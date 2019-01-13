/**
 * Input/output helpers.
 *
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 */
#include "io.h"

#include <stdlib.h> /*  malloc free memset */
#include <stdio.h> /* perror */
#include <string.h> /* memset */
#include <unistd.h>

char *
read_browser_request (uint32_t *size)
{
  char *text = NULL;

  /* First 4 bytes is the message type */
  if (read (STDIN_FILENO, (char *) size, 4) <= 0)
    {
      perror ("Failed to read request size");
      return text;
    }

  if (unlikely ((text = malloc (*size)) == NULL))
    {
      perror("Failed to allocate memory for the text");
      return text;
    }

  if (read (STDIN_FILENO, text, *size) != *size)
    {
      perror ("Failed to read request body");
      free (text);
      return NULL;
    }

  return text;
}


char *
read_file_from_fd (int fd, size_t *len)
{
  size_t bytes_read;
  char *text = NULL;
  *len = lseek (fd, 0, SEEK_END);

  if (unlikely (*len == -1))
    {
      perror ("lseek");
      return NULL;
    }

  if (unlikely (lseek (fd, 0, SEEK_SET) == -1))
    {
      perror ("lseek");
      return NULL;
    }

  text = malloc (*len + 1);
  if (unlikely (text == NULL))
    {
      perror ("malloc");
      return NULL;
    }
  memset (text, 0, *len + 1);

  if (read (fd, text, *len) == -1)
    {
      perror ("read");
      free (text);
      return NULL;
    }

  text[*len] = '\0';

  return text;
}


char *
read_file_from_stream (FILE *stream, size_t *len)
{
  char *text = NULL;
  size_t bytes_read = 0;

  if (unlikely (fseek (stream, 0, SEEK_END) != 0))
    {
      perror ("fseek");
      return NULL;
    }

  *len = ftell (stream);
  if (unlikely (*len == -1))
    {
      perror ("ftell");
      return NULL;
    }

  /* Reserve space for terminating null byte */
  text = malloc (*len + 1);
  if (unlikely (text == NULL))
    {
      perror ("malloc");
      return NULL;
    }

  printf ("reading %ld bytes\n", *len);
  fseek (stream, 0, SEEK_SET);
  bytes_read = fread (text, 1, *len, stream);
  printf ("read %ld bytes\n", bytes_read);
  if (unlikely (bytes_read != *len))
    {
      perror ("fread failed to read the entire file");
      free (text);
      return NULL;
    }
  text[*len] = '\0';

  return text;
}

