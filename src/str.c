/**
 * Native messaging host for Bee browser extension.
 * String utilities.
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
#include <stdlib.h> /* malloc */
#include <stdbool.h>
#include <assert.h>

#include "str.h"

#ifdef WINDOWS
# include <windows.h>
#endif

#ifndef HAVE_STRNDUP
char *
strndup (const char *s, size_t n)
{
    char *p = memchr (s, '\0', n);

    if (p != NULL)
      n = p - s;

    p = malloc (n + 1);
    if (p != NULL)
      {
        memcpy (p, s, n);
        p[n] = '\0';
      }

    return p;
}
#endif


bool
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

#ifdef WINDOWS
wchar_t *
convert_char_array_to_LPCWSTR (const char *str, int *wstr_len_in_chars)
{
  wchar_t *wstr = NULL;

  /* Determine the size required for the output buffer */
  *wstr_len_in_chars = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);

  wstr = LocalAlloc (LMEM_ZEROINIT, *wstr_len_in_chars * sizeof (wchar_t));
  if (unlikely (wstr == NULL))
    return NULL;

  if (unlikely (!MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, *wstr_len_in_chars)))
    {
      perror ("MultiByteToWideChar");
      LocalFree (wstr);
      wstr = NULL;
    }

  return wstr;
}

wchar_t **convert_single_byte_to_multibyte_array (
        const char * const *args,
        const unsigned num_args)
{
  bool success = true;
  unsigned int i = 0;
  int warg_len = 0;
  wchar_t *warg = NULL;
  wchar_t **wargs = NULL;

  wargs = LocalAlloc(LMEM_ZEROINIT, sizeof (wchar_t*) * num_args);

  for (i = 0; i < num_args; i++)
    {
      if (args[i] == NULL)
        /* No error (considered a sentinel item) */
        break;

      warg = convert_char_array_to_LPCWSTR (args[i], &warg_len);
      if (unlikely (wargs == NULL))
        {
          success = false;
          break;
        }

      wargs[i] = warg;
    }

  if (unlikely (!success))
    {
      free_argsw (wargs, num_args);
      LocalFree (wargs);
      wargs = NULL;
    }

  assert (success || wargs == NULL);

  return wargs;
}

#endif /* WINDOWS */
