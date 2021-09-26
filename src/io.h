/**
 * Input/output helpers.
 *
 * Copyright © 2019,2020,2021 Ruslan Osmanov <rrosmanov@gmail.com>
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
#include <stdint.h> /* uint32_t UINT32_MAX */
#include <sys/types.h> /* size_t */
#include <stdio.h> /* FILE */
#include <stdbool.h>
#include "common.h"

#ifdef NDEBUG
# define elog_debug(...) ((void) 0)
# define elog_debugw(...) ((void) 0)
#else
# define elog_debug(...) fprintf (stderr, __VA_ARGS__)
# define elog_debugw(...) fwprintf (stderr, __VA_ARGS__)
#endif
#define elog_error(...) fprintf (stderr, __VA_ARGS__)

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
int open_tmp_file (char **out_path, const char* ext, unsigned ext_len);

/* Removes file from filesystem */
bool remove_file (const char* filename);

#endif /* __BEECTL_IO_H__ */
