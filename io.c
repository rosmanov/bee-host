/**
 * Input/output helpers.
 *
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 */
#include "common.h"
#include "io.h"
#include "str.h"

#include <stdlib.h> /*  malloc free memset */
#include <stdio.h> /* perror */
#include <string.h> /* memset */
#include <assert.h>
#include <fcntl.h> /* setmode, _O_CREAT/_O_CREAT/_O_EXCL/_O_APPEND */
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WINDOWS
# include <wchar.h>
# include <io.h> /* _access, read, _mktemp_s, _open */
# include <process.h> /* _execl */
# include <windows.h>
#else
# include <unistd.h>
#endif

#define TMP_FILENAME_TEMPLATE "chrome_bee_XXXXXXXX"


char *
read_browser_request (uint32_t *size)
{
  char *text = NULL;

  /* First 4 bytes is the message type */
  if (read (STDIN_FILENO, (char *) size, 4) <= 0)
    {
      perror ("Failed to read request size");
      return text;
    }

  if (unlikely ((text = malloc (*size)) == NULL))
    {
      perror("Failed to allocate memory for the text");
      return text;
    }

  if (read (STDIN_FILENO, text, *size) != *size)
    {
      perror ("Failed to read request body");
      free (text);
      return NULL;
    }

  return text;
}


char *
read_file_from_fd (int fd, size_t *len)
{
  size_t bytes_read;
  char *text = NULL;
  *len = lseek (fd, 0, SEEK_END);

  if (unlikely (*len == -1))
    {
      perror ("lseek");
      return NULL;
    }

  if (unlikely (lseek (fd, 0, SEEK_SET) == -1))
    {
      perror ("lseek");
      return NULL;
    }

  text = malloc (*len + 1);
  if (unlikely (text == NULL))
    {
      perror ("malloc");
      return NULL;
    }
  memset (text, 0, *len + 1);

  if (read (fd, text, *len) == -1)
    {
      perror ("read");
      free (text);
      return NULL;
    }

  text[*len] = '\0';

  return text;
}


char *
read_file_from_stream (FILE *stream, size_t *len)
{
  char *text = NULL;
  size_t bytes_read = 0;

  if (unlikely (fseek (stream, 0, SEEK_END) != 0))
    {
      perror ("fseek");
      return NULL;
    }

  *len = ftell (stream);
  if (unlikely (*len == -1))
    {
      perror ("ftell");
      return NULL;
    }

  /* Reserve space for terminating null byte */
  text = malloc (*len + 1);
  if (unlikely (text == NULL))
    {
      perror ("malloc");
      return NULL;
    }

  fseek (stream, 0, SEEK_SET);
  bytes_read = fread (text, 1, *len, stream);
  if (unlikely (bytes_read != *len))
    {
      free (text);
      return NULL;
    }
  text[*len] = '\0';

  return text;
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
str_t *
get_sys_temp_dir (str_t *sys_temp_dir)
{
  if (unlikely (sys_temp_dir == NULL))
    return NULL;

#ifdef WINDOWS
    {
      wchar_t w_tmp[MAX_PATH];
      char *tmp;
      size_t len = GetTempPathW (MAX_PATH, w_tmp);
      assert (0 < len);

      sys_temp_dir->name = wide_char_to_multibyte (w_tmp, len, &sys_temp_dir->size);

      return sys_temp_dir;
    }
#else
    {
      char* s = getenv ("TMPDIR");
      if (s && *s)
        {
          int len = strlen (s);

          if (s[len - 1] == DIR_SEPARATOR)
            {
              sys_temp_dir->name = strndup (s, len - 1);
              sys_temp_dir->size = len - 1;
            }
          else
            {
              sys_temp_dir->name = strndup (s, len);
              sys_temp_dir->size = len;
            }

          return sys_temp_dir;
      }
    }

  /* Fallback */
  sys_temp_dir->name = strdup ("/tmp");
  sys_temp_dir->size = sizeof ("/tmp");

  return sys_temp_dir;
#endif
}


int
open_tmp_file (char **out_path)
{
  int fd = -1;
  str_t tmp_dir = { 0 };
  size_t tmp_file_template_size = 0;
  char *tmp_file_template = NULL;

  if (unlikely (get_sys_temp_dir (&tmp_dir) == NULL))
    return -1;

  tmp_file_template_size = (tmp_dir.size - 1) +
    DIR_SEPARATOR_LEN + sizeof (TMP_FILENAME_TEMPLATE);

  tmp_file_template = malloc (tmp_file_template_size);
  if (unlikely (tmp_file_template == NULL))
    {
      perror ("malloc");
      goto _ret;
    }

  snprintf (tmp_file_template, tmp_file_template_size,
            "%s%c" TMP_FILENAME_TEMPLATE,
            tmp_dir.name, DIR_SEPARATOR);

#ifdef WINDOWS
  if (_mktemp_s (tmp_file_template, tmp_file_template_size) != 0)
    goto _ret;

  fd = _open (tmp_file_template,
      _O_RDWR | _O_CREAT | _O_APPEND | _O_EXCL,
      _S_IWRITE | _S_IREAD);
#else
  fd = mkstemp (tmp_file_template);
#endif

  *out_path = tmp_file_template;

_ret:
  str_destroy (&tmp_dir);

  if (fd == -1)
    {
      if (tmp_file_template != NULL)
        free (tmp_file_template);
    }

  return fd;
}

