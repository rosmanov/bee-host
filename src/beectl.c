/**
 * Implementation of native messaging host for Bee browser extension.
 *
 * Optional path to the text editor executable is passed to the standard input.
 * If the path is not provided, the script tries to find a popular text editor
 * such as gvim, macvim, gedit, kate etc.
 *
 * When a subprocess of the text editor finishes, the script sends the
 * updated text back to the browser extension.
 *
 * Copyright © 2019-2025 Ruslan Osmanov <608192+rosmanov@users.noreply.github.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "common.h"
#include "shell.h"
#include "str.h"
#include "io.h"
#include "basename.h"

#include <stdlib.h> /* getenv, malloc, realloc, free */
#include <string.h> /* strtok, strcmp, memcpy, printf */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h> /* uint32_t UINT32_MAX */
#include <inttypes.h>
#include <assert.h> /* static_assert, assert */
#include <sys/stat.h>
#include <fcntl.h> /* O_BINARY */

#include <uv.h>
#include "cjson/cJSON.h"

/* The number of extra bytes for pathname realloc()s.
   By allocating more bytes than required we are trying to reduce the number of
   memory allocation calls which are known to be slow */
#define REALLOC_PATHNAME_STEP 128

/* Used to coalesce multiple rapid file events into a single logical change. */
#define FILE_CHANGE_DEBOUNCE_DELAY_MS 100

/* Used to delay the watcher to avoid phantom editor open events. */
#define FILE_WATCH_INITIAL_DELAY_MS 300

uv_loop_t *loop;
uv_fs_event_t fs_event;
uv_timer_t debounce_timer;
uv_timer_t watch_start_timer;
bool debounce_timer_started = false;
char *tmp_file_path = NULL;
str_t tmp_file_dir = { 0 };
static uv_timespec_t last_mtime = {0};

static void
print_help ()
{
  printf ("%s.\n\n"
          "Version: %s\n"
          "Copyright: %s\n"
          "License: %s\n",
          PROJECT_DESCRIPTION,
          PROJECT_VERSION,
          PROJECT_COPYRIGHT,
          PROJECT_LICENSE);
}

/* Works like the `which` command on Unix-like systems.

   Returns absolute path to the executable, or NULL if executable is not found
   in any directories listed in the PATH environment variable.
   The returned string must be freed.

   executable_size is the number of bytes in executable including the
   terminating null byte. */
static char *
which (char *executable, size_t executable_size)
{
  char *dir = NULL;
  char *pathname = NULL;
  char *path = NULL;
  size_t pathname_size = 0;
  size_t dir_len = 0;
  size_t alloc_pathname_size = 256;
  const char *org_path;

  if (executable_size <= 1)
    return NULL;
  assert (executable != NULL);

  if (is_absolute_path (executable, executable_size))
    return strdup (executable);

  if (unlikely ((org_path = getenv ("PATH")) == NULL))
    {
      perror ("Environment variable PATH variable was not found");
      return NULL;
    }

  path = strdup (org_path);
  if (unlikely (path == NULL))
    {
      perror ("strdup");
      return NULL;
    }

  /* Pre-allocate memory for pathname */
  if (unlikely ((pathname = malloc (alloc_pathname_size)) == NULL))
    {
      perror ("malloc failed");
      free (path);
      return NULL;
    }

  for (dir = strtok (path, PATH_DELIMITER);
       dir != NULL;
       dir = strtok (NULL, PATH_DELIMITER))
    {
      dir_len = strlen (dir);

      pathname_size = dir_len + DIR_SEPARATOR_LEN + executable_size;

      /* Allocate more memory if needed */
      if (alloc_pathname_size < pathname_size)
        {
          pathname_size += REALLOC_PATHNAME_STEP;
          if (unlikely ((pathname = realloc (pathname, pathname_size)) == NULL))
            {
              perror ("realloc");
              free (path);
              return NULL;
            }
          alloc_pathname_size = pathname_size;
        }

      /* Build pathname */
      memset (pathname, 0, pathname_size);
      memcpy (pathname, dir, dir_len);
      static_assert (DIR_SEPARATOR_LEN == 1,
                    "DIR_SEPARATOR is expected to be 1");
      pathname[dir_len] = DIR_SEPARATOR;
      memcpy (pathname + dir_len + DIR_SEPARATOR_LEN,
              executable, executable_size);

      if (!access (pathname, F_OK))
        {
          free (path);
          return pathname;
        }
    }

  free (path);
  if (pathname != NULL)
    free (pathname);

  return NULL;
}


/* Reads the JSON value key "editor".
   `value` represents the root JSON object:
   {"editor":"...", ...} */
static char *
get_editor (const cJSON *obj)
{
  cJSON *editor = NULL;
  char *editor_text = NULL;
  char *editor_path = NULL;

  if (unlikely (obj == NULL) || !cJSON_IsObject (obj))
    return NULL;

  editor = cJSON_GetObjectItemCaseSensitive (obj, "editor");
  if (editor == NULL)
    return NULL;

  editor_text = cJSON_GetStringValue (editor);
  if (editor_text == NULL)
    return NULL;

  return which (editor_text, strlen (editor_text) + 1);
}


/* Reads the JSON value key "args", an array of command line arguments
   for executable specified via "editor" property.
   `value` represents the root JSON object: {"args":"...",...}
   `num_reserved_args` specified the number of the arguments to reserve in the
   resulting array.

   On success, returns an array of strings, where the last item in the
   resulting array is guaranteed to be NULL. Otherwise, returns NULL.
   */
static char **
get_editor_args (const cJSON *value,
                 unsigned *num_args,
                 const unsigned num_reserved_args,
                 char *editor)
{
  unsigned int x = 0;
  const cJSON *args_obj = NULL;
  cJSON *arg_obj = NULL;
  char **args = NULL;
  int length = 0;
  int args_array_len = 0;
  const char *error;
  const bool is_vim = ends_with (editor, "vim");
  const size_t num_extra_args = num_reserved_args +
    (size_t) is_vim +
    1 + /* editor */
    1 /* Terminating NULL */;

  *num_args = 0;

  if (unlikely (value == NULL) || !cJSON_IsObject (value))
    return NULL;

  args_obj = cJSON_GetObjectItemCaseSensitive (value, "args");
  if (args_obj == NULL)
    args_obj = cJSON_CreateObject();

  if (unlikely (args_obj == NULL))
    {
      error = cJSON_GetErrorPtr ();
      if (error != NULL)
        fprintf (stderr, "Failed creating JSON object: %s\n", error);
      return NULL;
    }

  args_array_len = cJSON_GetArraySize (args_obj);
  length = args_array_len + num_extra_args;
  args = malloc (length * sizeof (char *));
  if (unlikely (args == NULL))
    {
      perror ("malloc");
      *num_args = 0;
      return NULL;
    }

  memset (args, 0, length * sizeof (char *));
  *num_args = length;

  args[x++] = strdup (editor);

  cJSON_ArrayForEach (arg_obj, args_obj)
    {
      char *tmp = NULL;

      if (x > args_array_len)
        break;

      if (!cJSON_IsString (arg_obj))
        continue;

      tmp = cJSON_GetStringValue (arg_obj);
      if (tmp == NULL)
        continue;

      if (unlikely ((args[x++] = strdup (tmp)) == NULL))
        {
          perror ("strdup");
          continue;
        }
    }

  /* Foreground option for a Vim editor */
  if (is_vim)
    args[x++] = strndup ("-f", sizeof ("-f") - 1);

  /* Terminating NULL */
  args[length - 1] = NULL;

  return args;
}


/* Reads a JSON text property value
   `value` is supposed to represent the root JSON object:
   {"text":"...", ...}

   The length of `value` is written to `value_len`.

   The caller is responsible for freeing memory allocated for the returned
   string. */
static char *
get_text_prop (const cJSON *value, unsigned int *value_len, const char* key)
{
  char *text = NULL;
  cJSON *text_obj = NULL;

  if (unlikely (value == NULL || !cJSON_IsObject (value)))
    return NULL;

  text_obj = cJSON_GetObjectItemCaseSensitive (value, key);
  if (text_obj == NULL || !cJSON_IsString (text_obj))
    return NULL;

  text = cJSON_GetStringValue (text_obj);
  if (text == NULL)
    return NULL;

  *value_len = strlen (text);
  return strndup (text, *value_len);
}


static inline char *
get_text (const cJSON *value, unsigned int *value_len)
{
  return get_text_prop (value, value_len, "text");
}


static inline char *
get_ext (const cJSON *value, unsigned int *value_len)
{
  return get_text_prop (value, value_len, "ext");
}


/* Searches for default editor.
   Returns absolute path on success. Otherwise, NULL.
   The returned string must be freed by the caller. */
static char *
get_alternative_editor ()
{
  char *editor = NULL;
  const str_t fallback_editors[] = {
#ifdef WINDOWS
        { .name = "gedit.exe",        .size = sizeof ("gedit.exe")        },
        { .name = "sublime_text.exe", .size = sizeof ("sublime_text.exe") },
        { .name = "notepad++.exe",    .size = sizeof ("notepad++.exe")    },
        { .name = "notepad.exe",      .size = sizeof ("notepad.exe")      },
#else
        { .name = "gvim",     .size = sizeof ("gvim")     },
        { .name = "sublime",  .size = sizeof ("sublime")  },
        { .name = "gedit",    .size = sizeof ("gedit")    },
        { .name = "kate",     .size = sizeof ("kate")     },
        { .name = "mousepad", .size = sizeof ("mousepad") },
        { .name = "leafpad",  .size = sizeof ("leafpad")  },
#endif
        { .name = NULL, .size = 0 },
  };

  for (unsigned i = 0; fallback_editors[i].name != NULL; i++)
    {
      editor = which (fallback_editors[i].name, fallback_editors[i].size);
      if (editor != NULL)
        return editor;
    }

  return NULL;
}

static void
on_file_change_debounced (uv_timer_t *handle)
{
  elog_debug ("%s: debounced file change confirmed\n", __func__);
  debounce_timer_started = false;

  elog_debug ("%s: sending response to the browser\n", __func__);
  if (tmp_file_path != NULL)
    send_file_response (tmp_file_path);
}

static void
on_file_change (uv_fs_event_t *handle,
                const char *filename,
                int events,
                int status)
{
  if (filename == NULL || strcmp (filename, basename (tmp_file_path)) != 0)
    return;

  if (status < 0)
    {
      elog_error ("Watch error: %s\n", uv_strerror (status));
      return;
    }

  elog_debug ("Raw file event: %s (events: %d)\n", filename, events);

  if (debounce_timer_started)
    uv_timer_stop (&debounce_timer);

  uv_timer_start (&debounce_timer, on_file_change_debounced,
                  FILE_CHANGE_DEBOUNCE_DELAY_MS, 0);
  debounce_timer_started = true;
}

/* Editor process exit callback */
static void
on_editor_process_exit (uv_process_t *req,
                 int64_t exit_status,
                 int term_signal)
{
  elog_debug ("editor process exited with status %" PRId64 "\n", exit_status);
  uv_fs_event_stop (&fs_event);
  uv_close ((uv_handle_t *)req, NULL);
  uv_stop (loop);
}

static void
poll_tmp_file (uv_timer_t *handle)
{
  uv_fs_t stat_req;
  uv_timespec_t mtime;
  int rc = -1;

  rc = uv_fs_stat (loop, &stat_req, tmp_file_path, NULL);
  if (rc < 0)
    {
      elog_error ("uv_fs_stat failed: %s\n", uv_strerror (rc));
      uv_fs_req_cleanup (&stat_req);
      return;
    }

  mtime = stat_req.statbuf.st_mtim;
  uv_fs_req_cleanup (&stat_req);

  if (mtime.tv_sec != last_mtime.tv_sec || mtime.tv_nsec != last_mtime.tv_nsec)
    {
      last_mtime = mtime;
      elog_debug ("Polling detected file change: %s\n", tmp_file_path);
      send_file_response (tmp_file_path);
    }
}

static void
start_file_watch_cb (uv_timer_t *timer)
{
  int res = -1;

  /* Watch the directory of the temp file because many editors such as
   *vim and code don't modify the inode of the file; instead, they write the
   updated content to a temporary file, delete the original, rename the new
   file to the original name (inode is destroyed and not being watched). */

   /* uv_fs_event_start() is unreliable/broken on macOS when watching temporary
   directories/files (e.g., /tmp, /private/tmp etc.), or under sandboxed
   contexts. So we use polling there.

   If we ever move the temp file to a more stable location like
   ~/Library/Application\ Support/..., we can switch back to uv_fs_event_start(). */
#ifndef __APPLE__
  res = uv_fs_event_start (&fs_event, on_file_change, tmp_file_dir.name, 0);

  if (res < 0)
    {
      elog_error ("Failed to start fs_event: %s; "
                  "falling back to polling\n",
                  uv_strerror (res));
      uv_timer_start (&debounce_timer,
                      poll_tmp_file,
                      FILE_CHANGE_DEBOUNCE_DELAY_MS,
                      FILE_CHANGE_DEBOUNCE_DELAY_MS);
    }
#else
  elog_debug ("Using polling for file changes on macOS\n");
  uv_timer_start (&debounce_timer,
                  poll_tmp_file,
                  FILE_CHANGE_DEBOUNCE_DELAY_MS,
                  FILE_CHANGE_DEBOUNCE_DELAY_MS);
#endif

  elog_debug ("Started watching file: %s\n", tmp_file_path);
  uv_timer_stop (timer);
  uv_close ((uv_handle_t *) timer, NULL);
}

int
main (int argc, char *argv[])
{
  int fd = -1;
  int exit_code = EXIT_SUCCESS;
  int i = 0;
  int res = -1;
  char *editor = NULL;
  char **editor_args = NULL;
  unsigned editor_args_num = 0;
  char *text = NULL;
  unsigned text_len = 0;
  char *ext = NULL;
  unsigned ext_len = 0;
  char *json_text = NULL;
  uint32_t json_size = 0;
  cJSON *obj = NULL;
  const char *error = NULL;
  unsigned num_reserved_args = 1 /* tmp_file_path */;
  uv_process_t child_proc;
  uv_process_options_t proc_options = { 0 };

  for (i = 0; i < argc; ++i)
    {
      const char * const arg = argv[i];
      int arg_len;

      if (arg[0] != '-')
        continue;

      arg_len = strlen (arg);
      if (arg_len == 2)
        {
          if (arg[1] == '-') /* -- */
            break;
          if (arg[1] == 'h') /* -h */
            {
              print_help ();
              return exit_code;
            }
        }

      if (!strcmp(arg, "--help"))
        {
          print_help ();
          return exit_code;
        }
    }

  /* Set stdin to binary mode in order to avoid possible issues
   * with \r\n on Windows */
  SET_BINARY_MODE (STDIN_FILENO);
  SET_BINARY_MODE (STDOUT_FILENO);

  if ((json_text = read_browser_request (&json_size)) == NULL)
    return EXIT_FAILURE;

  obj = cJSON_Parse (json_text);
  if (unlikely (obj == NULL))
    {
      error = cJSON_GetErrorPtr ();
      if (error != NULL)
        elog_error ("Failed parsing browser request: %s\n", error);
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  assert (editor == NULL);
  editor = get_editor (obj);
  if (editor == NULL)
    {
      assert (editor == NULL);
      editor = get_alternative_editor ();
    }
  if (editor == NULL)
    {
      elog_error ("Editor not found\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  editor_args = get_editor_args (obj, &editor_args_num,
                                 num_reserved_args, editor);
  if (editor_args == NULL)
    {
      elog_error ("Couldn't get editor arguments\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  if ((text = get_text (obj, &text_len)) == NULL)
    {
      elog_error ("Failed to read 'text' value\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  ext = get_ext (obj, &ext_len);
  elog_debug ("'ext': (%s) (len = %u)\n", ext, ext_len);

  fd = open_tmp_file (&tmp_file_path, &tmp_file_dir, ext, ext_len);
  if (fd == -1)
    {
      elog_error ("Failed to open temporary file\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }
  elog_debug ("opened file (%s)\n", tmp_file_path);
  editor_args[editor_args_num - num_reserved_args - 1] = tmp_file_path;

  elog_debug ("writing %s (len = %u) to tmp file (fd = %d)\n",
           text, text_len, fd);
  if (write (fd, text, text_len) != text_len) {
      perror ("Temporary file is not writable");
      exit_code = EXIT_FAILURE;
      goto _ret;
  }
  if (unlikely (close (fd)))
    {
      perror ("close");
      goto _ret;
    }
  fd = -1;

  loop = uv_default_loop ();

  res = uv_fs_event_init (loop, &fs_event);
  if (unlikely (res < 0))
    {
      elog_error ("Failed to init fs_event: %s\n", uv_strerror (res));
      exit_code = EXIT_FAILURE;
      goto _ret;
    }
  res = uv_timer_init (loop, &debounce_timer);
  if (unlikely (res < 0))
    {
      elog_error ("Failed to init debounce timer: %s\n", uv_strerror (res));
      exit_code = EXIT_FAILURE;
      goto _ret;
    }
  res = uv_timer_init (loop, &watch_start_timer);
  if (unlikely (res < 0))
    {
      elog_error ("Failed to init watch_start_timer: %s\n", uv_strerror (res));
      exit_code = EXIT_FAILURE;
      goto _ret;
    }
  /* Delay file watching to avoid getting events on the newly created file
     which may happen if the editor touches or read-locks the file, triggers background
     processes taht open the file, create swap or backup files sometimes modifying the mtime. */
  uv_timer_start (&watch_start_timer, start_file_watch_cb, FILE_WATCH_INITIAL_DELAY_MS, 0);

  if (unlikely (editor_args_num == 0 || editor_args[0] == NULL))
    {
      elog_error ("Invalid editor arguments\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  proc_options.args = editor_args;
  proc_options.file = editor_args[0];
  proc_options.exit_cb = on_editor_process_exit;
  proc_options.flags = UV_PROCESS_WINDOWS_HIDE; /* Hide the terminal window on Windows. */
  proc_options.stdio_count = 0;
  proc_options.cwd = NULL;
  proc_options.env = NULL;

  elog_debug ("%s: spawning editor process\n", __func__);
  res = uv_spawn (loop, &child_proc, &proc_options);
  if (res < 0)
    {
      elog_error ("Failed to spawn editor process: %s\n", uv_strerror (res));
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  elog_debug ("%s: running event loop\n", __func__);
  uv_run (loop, UV_RUN_DEFAULT);

  elog_debug ("%s: stopping event loop\n", __func__);
  uv_fs_event_stop (&fs_event);
  if (uv_loop_alive (loop))
    uv_loop_close (loop);

  if (unlikely (tmp_file_path == NULL || access (tmp_file_path, F_OK) != 0))
    {
      elog_error ("Temporary file was not found after editor exited\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  elog_debug ("%s: sending response\n", __func__);
  send_file_response (tmp_file_path);

_ret:
  if (fd != -1) close (fd);
  if (tmp_file_path != NULL)
    remove_file (tmp_file_path);

  if (editor_args)
    {
      for (unsigned i = 0; i < editor_args_num; i++)
        {
          if (editor_args[i] != NULL)
            free (editor_args[i]);
        }
      free (editor_args);
    }
  if (editor != NULL) free (editor);
  if (json_text != NULL) free (json_text);
  if (obj != NULL) cJSON_Delete (obj);
  if (text != NULL) free (text);
  if (ext != NULL) free (ext);
  str_destroy (&tmp_file_dir);

  elog_debug ("%s exiting with exit_code = %d\n", __func__, exit_code);
  return exit_code;
}
