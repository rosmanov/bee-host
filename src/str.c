/**
 * Native messaging host for Bee browser extension.
 * String utilities implementation.
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
#include <stdlib.h> /* malloc */
#include "str.h"

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
