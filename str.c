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
