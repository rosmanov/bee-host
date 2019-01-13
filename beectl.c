/**
 * Native messaging host for Bee browser extension.

 * Optional path to the text editor executable is passed to the standard input.
 * If the path is not provided, the script tries to find a popular text editor
 * such as gvim, macvim, gedit, kate etc.
 *
 * When a subprocess of the text editor finishes, the script sends the
 * updated text back to the browser extension.
 *
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 */
#include "common.h"
#include "beectl.h"
#include "shell.h"
#include "str.h"
#include "io.h"

#include "cJSON.h"


/* Path to the system temporary directory */
static str_t sys_temp_dir = { .name = NULL, .size = 0 };


/* Returns true, if path looks like an absolute path.
   path_size includes terminating null byte. */
static inline bool
is_absolute_path (const char *path, size_t path_size)
{
  if (unlikely (path == NULL))
    return false;

#ifdef WINDOWS
  /* Something like "D:\path\to\file" */
  return (path_size > 3 && *(path + 1) == ':' && *(path + 2) == DIR_SEPARATOR);
#else
  return (*path == DIR_SEPARATOR);
#endif
}


static bool
ends_with (const char *str, const char *suffix)
{
  size_t str_len;
  size_t suffix_len;

  if (unlikely (str == NULL || suffix == NULL))
    return false;

  str_len = strlen (str);
  suffix_len = strlen (suffix);

  if (suffix_len > str_len)
    return false;

  return !strncmp (str + str_len - suffix_len,
                  suffix, suffix_len);
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
    return executable;

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
    return NULL;

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


/* Reads the JSON value key "text".
   `value` is supposed to represent the root JSON object:
   {"text":"...", ...}

   The length of `value` is written to `value_len`.

   The caller is responsible for freeing memory allocated for the returned
   string. */
static char *
get_text (const cJSON *value, unsigned int *value_len)
{
  char *text = NULL;
  cJSON *text_obj = NULL;

  if (unlikely (value == NULL || !cJSON_IsObject (value)))
    return NULL;

  text_obj = cJSON_GetObjectItemCaseSensitive (value, "text");
  if (text_obj == NULL || !cJSON_IsString (text_obj))
    return NULL;

  text = cJSON_GetStringValue (text_obj);
  if (text == NULL)
    return NULL;

  *value_len = strlen (text);
  return strndup (text, *value_len);
}


/* Searches for default editor.
   Returns absolute path on success. Otherwise, NULL.
   The returned string must be freed by the caller. */
static char *
get_alternative_editor ()
{
  char *editor;
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
        return strdup (editor);
    }

  return NULL;
}


#ifdef WINDOWS
/* Converts wide-character string to multibyte character string.
   The caller is responsible for freeing the result. */
static char *
wide_char_to_multibyte (const wchar_t *in, size_t in_len, size_t *out_len)
{
  int r = 0;
  int size = 0;
  char *result = NULL;

  size = WideCharToMultiByte (CP_UTF8, 0, in, in_len, NULL, 0, NULL, NULL);
  if (size == 0)
    {
      perror ("Failed to determine length of a multibyte string");
      return NULL;
    }

  result = malloc (size);
  if (result == NULL)
    {
      perror ("Failed to allocate memory for a multibyte string");
      return NULL;
    }

  r = WideCharToMultiByte (CP_UTF8, 0, in, in_len, result, size, NULL, NULL);
  if (r == 0)
    {
      free (result);
      perror ("WideCharToMultiByte");
      return NULL;
    }

  assert (result ? r == size : 1);
  assert (result ? strlen (result) == size - 1 : 1);

  result[size - 1] = '\0';

  return result;
}
#endif


/* Returns the system temporary directory */
static const str_t *
get_sys_temp_dir ()
{
  if (sys_temp_dir.name != NULL)
    return &sys_temp_dir;

#ifdef WINDOWS
    {
      wchar_t w_tmp[MAX_PATH];
      char *tmp;
      size_t len = GetTempPathW (MAX_PATH, w_tmp);
      assert (0 < len);

      sys_temp_dir.name = wide_char_to_multibyte (w_tmp, len, &sys_temp_dir.size);

      return &sys_temp_dir;
    }
#else
    {
      char* s = getenv ("TMPDIR");
      if (s && *s)
        {
          int len = strlen (s);

          if (s[len - 1] == DIR_SEPARATOR)
            {
              sys_temp_dir.name = strndup (s, len - 1);
              sys_temp_dir.size = len - 1;
            }
          else
            {
              sys_temp_dir.name = strndup (s, len);
              sys_temp_dir.size = len;
            }

          return &sys_temp_dir;
      }
    }

  /* Fallback */
  sys_temp_dir.name = strdup ("/tmp");
  sys_temp_dir.size = sizeof ("/tmp");

  return &sys_temp_dir;
#endif
}


/* Creates and opens a temporary file.
   Returns file descriptor.
   On error, -1 is returned, and errno is set appropriately */
static inline int
open_tmp_file (char **out_path)
{
  int fd = -1;
  const str_t *tmp_dir = NULL;
  size_t tmp_file_template_size = 0;
  char *tmp_file_template = NULL;

  tmp_dir = get_sys_temp_dir ();

  tmp_file_template_size = (tmp_dir->size - 1) +
    DIR_SEPARATOR_LEN + sizeof (TMP_FILENAME_TEMPLATE);

  tmp_file_template = malloc (tmp_file_template_size);
  if (unlikely (tmp_file_template == NULL))
    {
      perror ("malloc");
      return -1;
    }

  snprintf (tmp_file_template, tmp_file_template_size,
            "%s%c" TMP_FILENAME_TEMPLATE,
            tmp_dir->name, DIR_SEPARATOR);

#ifdef WINDOWS
  if (_mktemp_s (tmp_file_template, tmp_file_template_size) != 0)
  {
    free (tmp_file_template);
    return -1;
  }

  fd = _open (tmp_file_template,
      _O_RDWR | _O_CREAT | _O_APPEND | _O_EXCL,
      _S_IWRITE | _S_IREAD);
#else
  fd = mkstemp (tmp_file_template);
#endif

  if (fd == -1)
    {
      if (tmp_file_template)
        free (tmp_file_template);
      return fd;
    }

  *out_path = tmp_file_template;

  return fd;
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

  response = cJSON_Print (json_response);
  if (response == NULL)
    {
      if ((error = cJSON_GetErrorPtr ()) != NULL)
        fprintf (stderr, "Failed converting JSON to string: %s\n", error);
    }

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

  if ((editor = get_editor (obj)) == NULL &&
      (editor = get_alternative_editor ()) == NULL)
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

  fd = open_tmp_file (&tmp_file_path);
  if (fd == -1)
    {
      fprintf (stderr, "Failed to open temporary file\n");
      exit_code = EXIT_FAILURE;
      goto _ret;
    }
  editor_args[editor_args_num - num_reserved_args - 1] = tmp_file_path;

  if (write (fd, text, text_len) != text_len) {
      perror ("Temporary file is not writable");
      exit_code = EXIT_FAILURE;
      goto _ret;
  }

  beectl_shell_exec ((const char * const*) editor_args, editor_args_num);

  free (json_text);
  json_text = make_response (fd, &json_size);
  json_size-- /* length = size_in_bytes - 1 */;

  if (unlikely (write (STDOUT_FILENO, (char *) &json_size, sizeof (uint32_t)) != sizeof (uint32_t)))
    {
      perror ("write");
      goto _ret;
    }

  if (unlikely (write (STDOUT_FILENO, json_text, json_size) != json_size))
    perror ("write");

_ret:
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
  if (fd != -1) close (fd);
  if (json_text != NULL) free (json_text);
  if (obj != NULL) cJSON_Delete (obj);
  if (text != NULL) free (text);
  if (sys_temp_dir.name != NULL) free (sys_temp_dir.name);

  return exit_code;
}
