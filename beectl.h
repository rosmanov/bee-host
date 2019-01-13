/**
 * Native messaging host for Bee browser extension
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 */
#ifndef __BEECTL_H__
# define __BEECTL_H__

#include <stdlib.h> /* getenv, malloc, realloc, free, system, mkstemps */
#include <string.h> /* strtok, memcpy, strcmp, strcpy_s */
#include <stdio.h> /* _fileno */
#include <stdbool.h>
#include <stdint.h> /* uint32_t UINT32_MAX */
#include <assert.h> /* static_assert, assert */
#include <sys/stat.h>
#include <fcntl.h> /* setmode, _O_CREAT/_O_CREAT/_O_EXCL/_O_APPEND */

#ifdef WINDOWS
# include <wchar.h>
# include <io.h> /* _access, read, _mktemp_s, _open */
# include <process.h> /* _execl */
# include <windows.h>
#else /* Unix */
# include <sys/types.h> /* pid_t */
# include <unistd.h> /* access, read */
#endif

#define TMP_FILENAME_TEMPLATE "chrome_bee_XXXXXXXX"

/* The number of extra bytes for pathname realloc()s.
   By allocating more bytes than required we are trying to reduce the number of
   memory allocation calls which are known to be slow */
#define REALLOC_PATHNAME_STEP 128

typedef struct _str_t {
  char *name;
  size_t size; /* Size of name in bytes */
} str_t;

#endif /* __BEECTL_H__ */
/* vim: set ft=c: */
