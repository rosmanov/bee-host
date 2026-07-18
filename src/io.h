/**
 * Input/output helpers.
 *
 * Copyright © 2019-2025 Ruslan Osmanov <608192+rosmanov@users.noreply.github.com>
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
#ifndef __BEECTL_IO_H__
# define __BEECTL_IO_H__
#include "common.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>    /* uint32_t UINT32_MAX */
#include <stdio.h>     /* FILE */
#include <sys/types.h> /* size_t */
#include <time.h>

#ifdef _WIN32
#  include <process.h>  /* for getpid() */
#  define ELOG_PATH_SEP '\\'
#else
#  include <limits.h>
#  define ELOG_PATH_SEP '/'
#endif

#include "str.h"

/* Environment variable to override log file path */
#define ELOG_ENV "BEECTL_DEBUG_LOG"
/* Default log file name (without path and extension) */
#define ELOG_DEFAULT_FILE "beectl_debug"
/* Include process ID in log filename to avoid clashes from multiple instances */
#define ELOG_INCLUDE_PID 1
/* Timestamp format for log entries */
#define ELOG_TS_FMT "%Y-%m-%d %H:%M:%S"

#ifndef NDEBUG

/* Close the log file at exit */
void elog_close (void);
FILE *elog_stream (void);

/* Pick log file path based on environment variable or temp directory */
static const char *elog_pick_path (char *out, size_t outsz);

void elog_log (const char *level, const char *file, int line, const char *func,
               const char *fmt, ...);
#endif /* !NDEBUG */

#ifdef NDEBUG
# define elog_debug(...) ((void) 0)
# define elog_debugw(...) ((void) 0)
# define elog_error(...) fprintf (stderr, __VA_ARGS__)
#else
/* In debug builds: write DEBUG to file; ERROR to file + mirror to stderr */
# define elog_debug(...)  elog_log("DEBUG", __FILE__, __LINE__, __func__, __VA_ARGS__)
  // If you truly need wide output, keep this; otherwise prefer UTF-8 narrow.
# define elog_error(...) elog_log ("ERROR", __FILE__, __LINE__, __func__, __VA_ARGS__)
#endif

/* Reads browser request from the standard input.

   Returns the text read. The number of bytes read is assigned to `size`.
   On error, NULL is returned, and the value of `size` is undefined.

   The returned string must be freed by the caller. */
char *read_browser_request (uint32_t *size);

/* Reads an entire file.

   Returns the text read from the file as a null-terminated string. The string
   length is saved into `len`.

   On error, NULL is returned, and the value of len is undefined. */
char *read_file_from_fd (int fd, size_t *len);


/* Reads an entire file.

   Returns the text read from the file as a null-terminated string. The string
   length is saved into `len`.

   On error, NULL is returned, and the value of len is undefined. */
char *read_file_from_stream (FILE *stream, size_t *len);

/* Creates and opens a temporary file.
   Returns file descriptor.
   On error, -1 is returned, and errno is set appropriately */
int open_tmp_file (char **out_path, str_t *tmp_dir, const char* ext, unsigned ext_len);

/* Removes file from filesystem */
bool remove_file (const char* filename);

/* Generates response for the browser */
char *make_response (int fd, uint32_t *size);

/* Sends response to the browser */
void send_file_response(const char *filepath);

#endif /* __BEECTL_IO_H__ */
