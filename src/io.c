/**
 * Input/output helpers.
 *
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
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
#include "io.h"
#include "str.h"

#include <stdlib.h> /*  malloc free memset mkstemp mkstemps */
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

  result[size - 1] = '\0';
  *out_len = size;

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
open_tmp_file (char **out_path, const char* ext, unsigned ext_len)
{
  int fd = -1;
  str_t tmp_dir = { 0 };
  size_t tmp_file_template_size = 0;
  char *tmp_file_template = NULL;
  const unsigned suffix_len = ext_len ? 1 + ext_len : 0;

  if (unlikely (get_sys_temp_dir (&tmp_dir) == NULL))
    {
      perror ("get_sys_temp_dir");
      return -1;
    }

  tmp_file_template_size = (tmp_dir.size - 1) +
    DIR_SEPARATOR_LEN + sizeof (TMP_FILENAME_TEMPLATE) + suffix_len;

  tmp_file_template = malloc (tmp_file_template_size);
  if (unlikely (tmp_file_template == NULL))
    {
      perror ("malloc");
      goto _ret;
    }
  memset (tmp_file_template, 0, tmp_file_template_size);

  if (suffix_len)
    {
      snprintf (tmp_file_template, tmp_file_template_size,
                "%.*s%c" TMP_FILENAME_TEMPLATE ".%.*s",
                (int)tmp_dir.size, tmp_dir.name, DIR_SEPARATOR, ext_len, ext);

      fd = mkstemps (tmp_file_template, suffix_len);
      if (fd == -1)
        perror("mktemps");
    }
  else
    {
      snprintf (tmp_file_template, tmp_file_template_size,
                "%.*s%c" TMP_FILENAME_TEMPLATE,
                (int)tmp_dir.size, tmp_dir.name, DIR_SEPARATOR);

      fd = mkstemp (tmp_file_template);
      if (fd == -1)
        perror("mktemp");
    }

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


bool
remove_file (const char* filename)
{
  assert (filename);
  if (unlikely (filename == NULL))
    return false;

  if (unlikely (unlink (filename)))
    {
      perror ("unlink");
      return false;
    }

  return true;
}

