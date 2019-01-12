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
