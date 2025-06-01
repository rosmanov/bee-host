#ifndef __BEECTL_BASENAME_H__
#define __BEECTL_BASENAME_H__

#ifdef HAVE_BASENAME
# include <libgen.h>
#else
char *portable_basename(const char *path);
# define basename portable_basename
#endif

#endif /* __BEECTL_BASENAME_H__ */
