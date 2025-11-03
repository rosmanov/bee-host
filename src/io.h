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
#include <stdint.h> /* uint32_t UINT32_MAX */
#include <sys/types.h> /* size_t */
#include <stdio.h> /* FILE */
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include "common.h"
#include "str.h"

#ifdef _WIN32
#  include <windows.h>
#  define ELOG_PATH_SEP '\\'
#else
#  include <unistd.h>
#  include <limits.h>
#  define ELOG_PATH_SEP '/'
#endif

/* Environment variable to override log file path */
#define ELOG_ENV "BEECTL_DEBUG_LOG"
/* Default log file name (without path and extension) */
#define ELOG_DEFAULT_FILE "beectl_debug"
/* Include process ID in log filename to avoid clashes from multiple instances */
#define ELOG_INCLUDE_PID 1
/* Timestamp format for log entries */
#define ELOG_TS_FMT "%Y-%m-%d %H:%M:%S"

#ifndef NDEBUG
static FILE *elog_fp = NULL;

/* Close the log file at exit */
static void elog_close (void)
{
    if (elog_fp && elog_fp != stderr)
    {
        fclose (elog_fp);
    }
}

/* Pick log file path based on environment variable or temp directory */
static const char *
elog_pick_path (char *out, size_t outsz)
{
  const char *envp = getenv (ELOG_ENV);
  if (envp && *envp)
    {
      /* If env looks like a directory (ends with / or \), build a filename inside it */
      char last = envp[strlen(envp) - 1];
      if (last == '/' || last == '\\')
        {
#if ELOG_INCLUDE_PID
          snprintf (out, outsz, "%s%s_%ld.log", envp, ELOG_DEFAULT_FILE, (long) getpid ());
#else
          snprintf (out, outsz, "%s%s.log", envp, ELOG_DEFAULT_FILE);
#endif
          return out;
        }
      /* Otherwise treat as a full path */
      return envp;
    }

  /* No env override: choose a temp dir */
#ifdef _WIN32
  char tdir[MAX_PATH];
  DWORD n = GetTempPathA ((DWORD) sizeof tdir, tdir);
  if (!n || n >= sizeof tdir)
    {
      /* Fallback to current directory */
      snprintf (tdir, sizeof tdir, ".\\");
    }

  /* Ensure trailing separator */
  size_t L = strlen (tdir);
  if (L && tdir[L-1] != '\\' && tdir[L-1] != '/')
    strncat (tdir, "\\", sizeof tdir - L - 1);

# if ELOG_INCLUDE_PID
  snprintf (out, outsz, "%s%s_%ld.log", tdir, ELOG_DEFAULT_FILE, (long) GetCurrentProcessId ());
# else
  snprintf (out, outsz, "%s%s.log", tdir, ELOG_DEFAULT_FILE);
# endif
#else /* !_WIN32 */
  const char *tdir = getenv ("TMPDIR");
  if (!tdir || !*tdir)
    tdir = "/tmp/";

  /* Ensure trailing slash */
  char base[PATH_MAX];
  snprintf (base, sizeof base, "%s", tdir);
  size_t L = strlen (base);
  if (L && base[L-1] != '/')
    strncat (base, "/", sizeof base - L - 1);

# if ELOG_INCLUDE_PID
  snprintf (out, outsz, "%s%s_%ld.log", base, ELOG_DEFAULT_FILE, (long)getpid());
# else
  snprintf (out, outsz, "%s%s.log", base, ELOG_DEFAULT_FILE);
# endif
#endif /* _WIN32 */

  return out;
}

static FILE *
elog_stream (void)
{
  if (elog_fp)
    return elog_fp;

  char pathbuf[1024];
  const char *path = elog_pick_path (pathbuf, sizeof pathbuf);

  /* "a" avoids clobbering from repeated runs; line-buffer for timely writes */
  elog_fp = fopen (path, "a");
  if (!elog_fp)
    {
      elog_fp = stderr; /* last-ditch fallback */
      return elog_fp;
    }

  setvbuf (elog_fp, NULL, _IOLBF, 0);
  atexit (elog_close);

  /* Header to spot which file is used */
  time_t now = time (NULL);
  char ts[32];
  strftime (ts, sizeof ts, ELOG_TS_FMT, localtime (&now));
  fprintf (elog_fp, "[%s] DEBUG: log started at %s\n", ts, path);
  return elog_fp;
}

static void
elog_vlog (const char *level, const char *file, int line,
          const char *func, const char *fmt, va_list ap)
{
  FILE *fp = elog_stream ();
  time_t now = time (NULL);
  char ts[32];

  strftime (ts, sizeof ts, ELOG_TS_FMT, localtime (&now));

  /* Prefix with timestamp + level + origin */
  fprintf (fp, "[%s] %s %s:%d %s: ", ts, level, file, line, func);
  vfprintf (fp, fmt, ap);
  if (fp != stderr)
    fflush (fp);

  /* Mirror ERRORs to stderr too (useful outside the browser host) */
  if (fp != stderr && level[0] == 'E')
    {
      va_list ap2;

      va_copy (ap2, ap);
      fprintf (stderr, "[%s] %s %s:%d %s: ", ts, level, file, line, func);
      vfprintf (stderr, fmt, ap2);
      va_end (ap2);
      fflush (stderr);
    }
}

static void
elog_log (const char *level, const char *file, int line,
         const char *func, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  elog_vlog (level, file, line, func, fmt, ap);
  va_end (ap);
}
#endif /* !NDEBUG */

#ifdef NDEBUG
# define elog_debug(...) ((void) 0)
# define elog_debugw(...) ((void) 0)
# define elog_error(...) fprintf (stderr, __VA_ARGS__)
#else
/* In debug builds: write DEBUG to file; ERROR to file + mirror to stderr */
# define elog_debug(...)  elog_log("DEBUG", __FILE__, __LINE__, __func__, __VA_ARGS__)
  // If you truly need wide output, keep this; otherwise prefer UTF-8 narrow.
# define elog_debugw(...) do \
  { \
    FILE *fp = elog_stream (); \
    fwprintf (fp, L"[DEBUG] %hs:%d %hs: ", __FILE__, __LINE__, __func__); \
    fwprintf (fp, __VA_ARGS__); \
    if (fp != stderr) fflush (fp); \
  } while (0)
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
