/**
 * Declarations common to all sources.
 *
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 */
#ifndef __BEECTL_COMMON_H__
# define __BEECTL_COMMON_H__

#if defined(_WIN32) || defined(WIN32)
# ifndef WINDOWS
#  define WINDOWS
# endif
#endif

#ifndef STDIN_FILENO
# define STDIN_FILENO _fileno (stdin)
#endif
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

#ifdef _MSC_VER
# define forceinline __forceinline
#elif defined(__GNUC__)
# define forceinline inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
# if __has_attribute(__always_inline__)
#   define forceinline inline __attribute__((__always_inline__))
# else
#   define forceinline inline
# endif
#else
# define forceinline inline
#endif

#ifndef MAX_PATH
# define MAX_PATH 1024
#endif

#ifdef WINDOWS
# define PATH_DELIMITER ";"
# define DIR_SEPARATOR '\\'
# define access _access
# define read _read
# define setmode _setmode
# define lseek _lseek
# define F_OK 0 /* Test for existence */
# define open _open
# define TMP_FILE_MODE (_S_IWRITE | _S_IREAD)
# define SET_BINARY_MODE(fd) setmode ((fd), O_BINARY)
#else /* Unix */
# define PATH_DELIMITER ":"
# define DIR_SEPARATOR '/'
# define TMP_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP)
# define SET_BINARY_MODE(fd) ((void) 0)
#endif /* WINDOWS */
#define DIR_SEPARATOR_LEN sizeof (char)

#endif /* __BEECTL_COMMON_H__ */
