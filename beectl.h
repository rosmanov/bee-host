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

#include "json.h"
#include "json-builder.h"

#if defined(_WIN32) || defined(WIN32)
# ifndef WINDOWS
#  define WINDOWS
# endif

# include <wchar.h>
# include <io.h> /* _access, read, _mktemp_s, _open */
# include <process.h> /* _execl */
# include <windows.h>

# define PATH_DELIMITER ";"
# define DIR_SEPARATOR '\\'
# define DIR_SEPARATOR_LEN sizeof(char)
# define access _access
# define read _read
# define setmode _setmode
# define execl _execl
# define lseek _lseek
# define F_OK 0 /* Test for existence */
# define open _open
# define TMP_FILE_MODE (_S_IWRITE | _S_IREAD)
# define SET_BINARY_MODE(fd) setmode ((fd), O_BINARY)
#else /* Unix */
# include <sys/types.h> /* pid_t */
# include <unistd.h> /* access, read */
# define PATH_DELIMITER ":"
# define DIR_SEPARATOR '/'
# define DIR_SEPARATOR_LEN sizeof (char)
# define TMP_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP)
# define SET_BINARY_MODE(fd) ((void)0)
#endif

# ifndef STDIN_FILENO
#  define STDIN_FILENO _fileno (stdin)
# endif
#ifndef STDERR_FILENO
# define STDERR_FILENO _fileno (stderr)
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO _fileno (stdout)
#endif

#if HAVE___BUILTIN_EXPECT
# define likely(x) __builtin_expect ((x), 1)
# define unlikely(x) __builtin_expect ((x), 0)
#else
# define likely(x) (x)
# define unlikely(x) (x)
#endif

#ifndef MAX_PATH
# define MAX_PATH 1024
#endif

#define TMP_FILENAME_TEMPLATE "chrome_bee_XXXXXXXX"

/* The number of extra bytes for pathname realloc()s.
   By allocating more bytes than required we are trying to reduce the number of
   memory allocation calls which are known to be slow */
#define REALLOC_PATHNAME_STEP 128
/* The step of automatic JSON buffer growth in bytes. */
#define REALLOC_JSON_STEP 256


typedef struct _str_t {
  char *name;
  size_t size; /* Size of name in bytes */
} str_t;

#endif /* __BEECTL_H__ */
/* vim: set ft=c: */
