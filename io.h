/**
 * Input/output helpers.
 *
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 */
#ifndef __BEECTL_IO_H__
# define __BEECTL_IO_H__
#include <stdint.h> /* uint32_t UINT32_MAX */
#include <sys/types.h> /* size_t */
#include <stdio.h> /* FILE */
#include "common.h"

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

#endif /* __BEECTL_IO_H__ */
