/**
 * Declarations common to all sources.
 *
 * Copyright © 2019,2020 Ruslan Osmanov <rrosmanov@gmail.com>
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
# define unlink _unlink
# define setmode _setmode
# define lseek _lseek
# define F_OK 0 /* Test for existence */
# define open _open
# define TMP_FILE_MODE (_S_IWRITE | _S_IREAD)
# define SET_BINARY_MODE(fd) setmode ((fd), O_BINARY)
# define O_RDWR _O_RDWR
# define O_APPEND _O_APPEND
# define O_EXCL _O_EXCL
# define S_IWRITE _S_IWRITE
# define S_IREAD _S_IREAD
#else /* Unix */
# define PATH_DELIMITER ":"
# define DIR_SEPARATOR '/'
# define TMP_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP)
# define SET_BINARY_MODE(fd) ((void) 0)
#endif /* WINDOWS */
#define DIR_SEPARATOR_LEN sizeof (char)

#endif /* __BEECTL_COMMON_H__ */
