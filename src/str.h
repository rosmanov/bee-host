/**
 * Native messaging host for Bee browser extension.
 * String utilities header.
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
#ifndef __BEECTL_STR_H__
# define __BEECTL_STR_H__
#include "common.h" /* unlikely */
#include <string.h> /* memchr, memcpy */
#include <stdbool.h>
#include <sys/types.h> /* size_t */
#include <stdlib.h> /* free */

#ifdef WINDOWS
# include <windows.h> /* LocalFree */
#endif

#ifndef HAVE_STRNDUP
char *strndup (const char *s, size_t n);
#endif

/* Checks if a string ends with a suffix */
bool ends_with (const char *str, const char *suffix);

/* Returns true, if path looks like an absolute path.
   path_size includes terminating null byte. */
forceinline bool
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

typedef struct _str_t {
  char *name;
  size_t size; /* Size of name in bytes */
} str_t;


/* Frees the memory allocated for the *inner* structure members
   (the pointer itself won't be freed) */
forceinline void
str_destroy (str_t *s)
{
  if (s)
    {
      if (s->name != NULL)
        free (s->name);
      s->name = NULL;
    }
}

#ifdef WINDOWS
wchar_t *convert_char_array_to_LPCWSTR (const char *str, int *wstr_len_in_chars);
wchar_t **convert_single_byte_to_multibyte_array (const char * const *args, const unsigned num_args);

forceinline void
free_argsw (wchar_t **argsw, const unsigned num_args)
{
  unsigned int i = 0;

  if (unlikely (argsw == NULL))
    return;

  for (i = 0; i < num_args; i++)
    {
      if (argsw[i] != NULL)
        LocalFree (argsw[i]);
    }
}

forceinline void
free_args (char **args, const unsigned num_args)
{
  unsigned int i = 0;

  if (unlikely (args == NULL))
    return;

  for (i = 0; i < num_args; i++)
    {
      if (args[i] != NULL)
        LocalFree (args[i]);
    }
}

#endif /* WINDOWS */

#endif /* __BEECTL_STR_H__ */
