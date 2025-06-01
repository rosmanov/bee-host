#ifndef HAVE_BASENAME
#include <string.h>
#include <stdlib.h>

char *
portable_basename (const char *path)
{
  const char *base;

  if (!path || !*path)
    return strdup (".");

  base = strrchr (path, '/');

#ifdef _WIN32
  const char *backslash = strrchr (path, '\\');
  if (!base || (backslash && backslash > base))
    base = backslash;
#endif

  return strdup (base ? base + 1 : path);
}
#endif /* HAVE_BASENAME */
