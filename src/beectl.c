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
 * Copyright © 2019,2020 Ruslan Osmanov <rrosmanov@gmail.com>
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

#include <stdlib.h> /* getenv, malloc, realloc, free */
#include <string.h> /* strtok, memcpy */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h> /* uint32_t UINT32_MAX */
#include <assert.h> /* static_assert, assert */
#include <sys/stat.h>
#include <fcntl.h> /* O_BINARY */
#ifndef WINODWS
# include <unistd.h>
#endif

#include "cjson/cJSON.h"

/* The number of extra bytes for pathname realloc()s.
   By allocating more bytes than required we are trying to reduce the number of
   memory allocation calls which are known to be slow */
#define REALLOC_PATHNAME_STEP 128

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
      static_assert (DIR_SEPARATOR_LEN == sizeof (char),
                    "DIR_SEPARATOR is expected to be a char");
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

  return which (editor_text, strlen (editor_text) + sizeof (""));
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

  args[x++] = editor;

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

static char *
make_response (int fd, uint32_t *size)
{
  size_t text_len = 0;
  char *text = read_file_from_fd (fd, &text_len);
  char *response = NULL;
  const char *error = NULL;
  cJSON *json_response = NULL;
  cJSON *json_text = NULL;

  if (unlikely (text == NULL))
    return NULL;

  json_text = cJSON_CreateStringReference (text);
  if (unlikely (json_text == NULL))
    return NULL;

  json_response = cJSON_CreateObject();
  if (unlikely (json_response == NULL))
    {
      cJSON_Delete (json_text);
      return NULL;
    }

  cJSON_AddItemReferenceToObject (json_response, "text", json_text);

  response = cJSON_PrintUnformatted (json_response);
  if (response == NULL)
    {
      if ((error = cJSON_GetErrorPtr ()) != NULL)
        fprintf (stderr, "Failed converting JSON to string: %s\n", error);
      goto _ret;
    }

  *size = strlen (response) + sizeof ("");

_ret:
  free (text);
  cJSON_Delete (json_text);
  cJSON_Delete (json_response);

  return response;
}


int
main (void)
{
  int fd = -1;
  int exit_code = EXIT_SUCCESS;
  char *editor = NULL;
  char **editor_args = NULL;
  unsigned editor_args_num = 0;
  char *text = NULL;
  unsigned text_len = 0;
  char *ext = NULL;
  unsigned ext_len = 0;
  char *json_text = NULL;
  char *tmp_file_path = NULL;
  uint32_t json_size = 0;
  cJSON *obj = NULL;
  const char *error = NULL;
  unsigned num_reserved_args = 1 /* tmp_file_path */;

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
        fprintf (stderr, "Failed parsing browser request: %s\n", error);
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
      fprintf (stderr, "Editor not found\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  editor_args = get_editor_args (obj, &editor_args_num,
                                 num_reserved_args, editor);
  if (editor_args == NULL)
    {
      fprintf (stderr, "Couldn't get editor arguments\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  if ((text = get_text (obj, &text_len)) == NULL)
    {
      fprintf (stderr, "Failed to read 'text' value\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }

  ext = get_ext (obj, &ext_len);

  fd = open_tmp_file (&tmp_file_path, ext, ext_len);
  if (fd == -1)
    {
      fprintf (stderr, "Failed to open temporary file\n");
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

  beectl_shell_exec ((const char * const*) editor_args, editor_args_num);

  elog_debug ("%s: making response\n", __func__);

  fd = open (tmp_file_path, O_RDWR | O_APPEND | O_EXCL,
             S_IWRITE | S_IREAD);
  if (unlikely (fd == -1))
    {
      perror("open");
      goto _ret;
    }

  free (json_text);
  json_text = make_response (fd, &json_size);
  json_size-- /* length = size_in_bytes - 1 */;

  elog_debug ("writing response size\n");
  if (unlikely (write (STDOUT_FILENO, (char *) &json_size, sizeof (uint32_t)) != sizeof (uint32_t)))
    {
      perror ("write");
      goto _ret;
    }

  elog_debug ("writing response body %s\n", json_text);
  if (unlikely (write (STDOUT_FILENO, json_text, json_size) != json_size))
    perror ("write");

_ret:
  if (fd != -1) close (fd);
  if (tmp_file_path != NULL)
    remove_file (tmp_file_path);

  if (editor != NULL && (!editor_args || editor != editor_args[0]))
    free (editor);
  if (editor_args)
    {
      for (unsigned i = 0; i < editor_args_num; i++)
        {
          if (editor_args[i] != NULL)
            free (editor_args[i]);
        }
      free (editor_args);
    }
  if (json_text != NULL) free (json_text);
  if (obj != NULL) cJSON_Delete (obj);
  if (text != NULL) free (text);
  if (ext != NULL) free (ext);

  elog_debug ("%s exiting with exit_code = %d\n", __func__, exit_code);
  return exit_code;
}
