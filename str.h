/**
 * Native messaging host for Bee browser extension
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 */
#ifndef __BEECTL_STR_H__
# define __BEECTL_STR_H__
#include <string.h> /* memchr, memcpy */

#ifndef HAVE_STRNDUP
char *strndup (const char *s, size_t n);
#endif

#endif /* __BEECTL_STR_H__ */
