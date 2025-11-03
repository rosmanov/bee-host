/**
 * Input/output helpers.
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
#include "io.h"
#include "str.h"
#include "mkstemps.h"

#include <stdlib.h> /*  malloc free memset mkstemp mkstemps */
#include <stdio.h> /* perror */
#include <string.h> /* memset */
#include <errno.h>
#include <assert.h>
#include <fcntl.h> /* setmode, _O_CREAT/_O_CREAT/_O_EXCL/_O_APPEND */
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WINDOWS
# include <wchar.h>
# include <io.h> /* _access, read, _mktemp_s, _open */
# include <process.h> /* _execl */
#endif

#include "cjson/cJSON.h"

#define TMP_FILENAME_TEMPLATE "chrome_bee_XXXXXXXX"

/* Reads exactly `count` bytes from the file descriptor `fd` into `buf`.
   Returns the number of bytes read, or -1 on error.
   If the end of file is reached before reading `count` bytes, returns the number of bytes read. */
static ssize_t
safe_read (int fd, void *buf, size_t count)
{
  size_t n = 0;

  while (n < count)
    {
      ssize_t r = read (fd, (char *)buf + n, count - n);
      if (r <= 0)
        return r;
      n += r;
    }

  return n;
}

char *
read_browser_request (uint32_t *size)
{
  char *text = NULL;

  /* First 4 bytes is the message type */
  if (safe_read (STDIN_FILENO, size, 4) != 4)
    {
      elog_error ("Failed to read request size\n");
      return text;
    }

  if (unlikely ((text = malloc (*size)) == NULL))
    {
      perror("Failed to allocate memory for the text");
      return text;
    }

  if (safe_read (STDIN_FILENO, text, *size) != *size)
    {
      elog_error ("Failed to read request body\n");
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
      elog_error ("Failed to lseek to 0: %s\n", strerror (errno));
      return NULL;
    }

  text = malloc (*len + 1);
  if (unlikely (text == NULL))
    {
      elog_error ("Failed to allocate memory for read buffer: %s\n", strerror (errno));
      return NULL;
    }
  memset (text, 0, *len + 1);

  if (safe_read (fd, text, *len) != *len)
    {
      elog_error ("Failed to read %zu bytes file from fd: %s\n",
                  *len, strerror (errno));
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
  long fsize;

  if (unlikely (fseek (stream, 0, SEEK_END) != 0))
    {
      perror ("fseek");
      return NULL;
    }

  /* Read into a long to avoid overflow issues with large files */
  fsize = ftell (stream);
  if (unlikely (fsize == -1L))
    {
      elog_error ("ftell failed: %s\n", strerror (errno));
      return NULL;
    }
  *len = (size_t)fsize;

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
static str_t *
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
              sys_temp_dir->size = len; /* len - 1 (last char) + 1 (terminating 0 byte)*/
            }
          else
            {
              sys_temp_dir->name = strndup (s, len);
              sys_temp_dir->size = len + 1 /* + 1 (terminating 0 byte)*/;
            }
          elog_debug ("%s: name: (%s)\n", __func__, sys_temp_dir->name);
          elog_debug ("%s: size: (%lu)\n", __func__, sys_temp_dir->size);

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
open_tmp_file (char **out_path, str_t *tmp_dir, const char* ext, unsigned ext_len)
{
  int fd = -1;
  size_t tmp_file_template_size = 0;
  char *tmp_file_template = NULL;
  const unsigned suffix_len = ext_len ? 1 + ext_len : 0;

  if (unlikely (tmp_dir == NULL))
    {
      elog_error ("tmp_dir is NULL\n");
      return -1;
    }

  if (unlikely (get_sys_temp_dir (tmp_dir) == NULL))
    {
      elog_error ("get_sys_temp_dir() failed: %s\n", strerror (errno));
      return -1;
    }

  tmp_file_template_size = (tmp_dir->size - 1) +
    DIR_SEPARATOR_LEN + sizeof (TMP_FILENAME_TEMPLATE) + suffix_len;

  tmp_file_template = malloc (tmp_file_template_size);
  if (unlikely (tmp_file_template == NULL))
    {
      elog_error ("malloc failed: %s\n", strerror (errno));
      goto _ret;
    }
  memset (tmp_file_template, 0, tmp_file_template_size);

  if (suffix_len)
    {
      snprintf (tmp_file_template, tmp_file_template_size,
                "%.*s%c" TMP_FILENAME_TEMPLATE ".%.*s",
                (int)tmp_dir->size, tmp_dir->name, DIR_SEPARATOR, ext_len, ext);
      elog_debug ("%s: tmp_file_template = \"%s\" "
                  "tmp_file_template_size = %lu "
                  "suffix_len = (%u) "
                  "ext = (%s) "
                  "ext_len = %u\n",
                 __func__,
                 tmp_file_template,
                 tmp_file_template_size,
                 suffix_len,
                 ext,
                 ext_len);

      fd = mkstemps (tmp_file_template, suffix_len);
      if (fd == -1)
        {
          if (errno == EEXIST)
              elog_error ("mkstemps: Temporary file already exists: %s\n", tmp_file_template);
          else
              elog_error ("mkstemps: Failed to create temporary file: %s\n", strerror (errno));
          goto _ret;
        }
    }
  else
    {
      snprintf (tmp_file_template, tmp_file_template_size,
                "%.*s%c" TMP_FILENAME_TEMPLATE,
                (int)tmp_dir->size, tmp_dir->name, DIR_SEPARATOR);

      fd = mkstemp (tmp_file_template);
      if (fd == -1)
        {
          if (errno == EEXIST)
              elog_error ("mkstemp: Temporary file already exists: %s\n", tmp_file_template);
          else
              elog_error ("mkstemp: Failed to create temporary file: %s\n", strerror (errno));
          goto _ret;
        }
    }

  elog_debug ("%s: tmp_file_template = \"%s\" "
              "tmp_file_template_size = %lu\n",
              __func__, tmp_file_template, tmp_file_template_size);
  *out_path = tmp_file_template;

_ret:
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

char *
make_response (int fd, uint32_t *size)
{
  size_t text_len = 0;
  char *text = NULL;
  char *response = NULL;
  const char *error = NULL;
  cJSON *json_response = NULL;
  cJSON *json_text = NULL;

  text = read_file_from_fd (fd, &text_len);
  if (unlikely (text == NULL))
    goto _ret;

  json_text = cJSON_CreateStringReference (text);
  if (unlikely (json_text == NULL))
    goto _ret;

  json_response = cJSON_CreateObject();
  if (unlikely (json_response == NULL))
    {
      cJSON_Delete (json_text);
      json_text = NULL;
      goto _ret;
    }

  if (unlikely (!cJSON_AddItemReferenceToObject (json_response, "text", json_text)))
    {
      error = cJSON_GetErrorPtr ();
      if (error != NULL)
        elog_error ("Failed adding 'text' to JSON response: %s\n", error);
      cJSON_Delete (json_text);
      json_text = NULL;
      goto _ret;
    }
  /* From this point, json_text is owned by json_response */

  response = cJSON_PrintUnformatted (json_response);
  if (unlikely (response == NULL))
    {
      if ((error = cJSON_GetErrorPtr ()) != NULL)
        elog_error ("Failed converting JSON to string: %s\n", error);
      goto _ret;
    }

  *size = strlen (response) + 1;

_ret:
  if (text != NULL) free (text);
  if (json_response != NULL) cJSON_Delete (json_response);

  return response;
}

void
send_file_response (const char *filepath)
{
  int fd = -1;
  char *response = NULL;
  uint32_t json_size = 0;

  fd = open (filepath, O_RDWR | O_APPEND | O_EXCL, S_IWRITE | S_IREAD);
  if (fd == -1)
    {
      elog_error ("%s: Failed to open file %s: %s\n",
                 __func__, filepath, strerror (errno));
      goto _ret;
    }

  elog_debug ("%s: making response\n", __func__);

  response = make_response (fd, &json_size);
  close(fd);

  if (response == NULL)
    {
      elog_debug ("Failed to create response\n");
      goto _ret;
    }

  json_size--; /* exclude trailing \0 */

  elog_debug ("writing response size\n");
  if (unlikely (write (STDOUT_FILENO, &json_size, sizeof (uint32_t)) != sizeof (uint32_t)))
    {
      elog_error ("Failed to write response size: %s\n", strerror (errno));
      goto _ret;
    }

  elog_debug ("writing response body (length %u)\n", json_size);
  if (unlikely (write (STDOUT_FILENO, response, json_size) != json_size))
    elog_error ("Failed to write response body: %s\n", strerror (errno));

_ret:
  if (response != NULL) free (response);
}
