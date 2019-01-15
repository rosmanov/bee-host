/**
 * Native messaging host for Bee browser extension
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 */
#ifndef __BEECTL_STR_H__
# define __BEECTL_STR_H__
#include "common.h" /* unlikely */
#include <string.h> /* memchr, memcpy */
#include <stdbool.h>
#include <sys/types.h> /* size_t */
#include <stdlib.h> /* free */

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
#endif /* __BEECTL_STR_H__ */
