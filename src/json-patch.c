/**
 * Command line tool to add or replace a JSON property.
 *
 * Used in installation scripts.
 *
 * Copyright © 2019-2023 Ruslan Osmanov <608192+rosmanov@users.noreply.github.com>
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
#include "common.h"
#include "io.h"
#include <stdio.h> /* fprintf fopen fclose */
#include <string.h> /* strlen memcmp */
#include <stdlib.h> /* EXIT_SUCCESS EXIT_FAILURE */
#include <stdbool.h>

#include "cjson/cJSON.h"
#include "cjson/cJSON_Utils.h"

int
main (int argc, char const* argv[])
{
  int exit_code = EXIT_SUCCESS;
  char *text = NULL;
  char *obj_text = NULL;
  size_t text_len = 0;
  cJSON *input_obj = NULL;
  cJSON *obj = NULL;
  FILE *stream = NULL;
  const char *error = NULL;

  if (argc < 3)
    {
      fprintf (stderr, "Usage: %s input-file json-text\n", argv[0]);
      return EXIT_FAILURE;
    }

  input_obj = cJSON_Parse (argv[2]);
  if (unlikely (input_obj == NULL))
    {
      error = cJSON_GetErrorPtr ();
      if (likely (error != NULL))
        fprintf (stderr, "Failed parsing JSON: %s\n", error);
      goto _ret;
    }

  stream = fopen (argv[1], "r");
  if (unlikely (stream == NULL))
    {
      perror ("fopen");
      goto _ret;
    }

  text = read_file_from_stream (stream, &text_len);
  if (unlikely (text == NULL))
    goto _ret;

  obj = cJSON_Parse (text);
  if (unlikely (obj == NULL))
    {
      error = cJSON_GetErrorPtr ();
      if (likely (error != NULL))
        fprintf (stderr, "Failed parsing JSON: %s\n", error);
      goto _ret;
    }

  if (unlikely (cJSONUtils_MergePatch(obj, input_obj) == NULL))
    {
      perror ("Failed merging input JSON");
      goto _ret;
    }

  obj_text = cJSON_Print (obj);

  printf("%s\n", obj_text);
_ret:
  if (input_obj != NULL) cJSON_Delete (input_obj);
  if (stream != NULL && fclose (stream))
    perror ("fclose");
  if (text != NULL) free (text);
  if (obj != NULL) cJSON_Delete (obj);
  if (obj_text != NULL) free (obj_text);

  return exit_code;
}
