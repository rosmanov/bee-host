/**
 * Native messaging host for Bee browser extension. Shell utilities.
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
#include "common.h"

#include <stdio.h> /* popen */
#include <string.h> /* strdup */
#include <stdbool.h>

#ifdef WINDOWS
# include <windows.h>
# include <strsafe.h>
#else
# include <sys/types.h> /* pid_t */
# include <sys/wait.h>
# include <unistd.h>
#endif

#include "io.h"
#include "str.h"
#include "shell.h"

#ifdef WINDOWS

/* Concatenates command line arguments.
 The returned string must be freed by the caller. */
static LPWSTR
concat_args (const wchar_t * const *wargs, const unsigned num_args)
{
  LPWSTR res = NULL;
  size_t res_size = 512 * sizeof (wchar_t);
  size_t res_len = 0;
  LPCWSTR wspace = L" ";

  elog_debug ("%s malloc(%ld)\n", __func__, res_size);
  res = malloc (res_size);
  if (unlikely (res == NULL))
    {
      perror ("malloc");
      return res;
    }
  memset (res, 0, res_size);

  for (unsigned i = 0; i < num_args; i++)
    {
      size_t len = 0;
      LPCWSTR warg = wargs[i];

      if (warg == NULL)
        break;

      elog_debugw (L"%s: processing arg %d: %S\n", __func__, i, warg);

      if (unlikely (FAILED (StringCbLengthW (warg, 4096, &len))))
        {
          perror ("StringCbLengthW");
          break;
        }

      res_len += len + sizeof (wspace);

      if (unlikely (res_len > res_size))
        {
          res_size <<= 1;
          res = realloc (res, res_size);
          if (unlikely (res == NULL))
            {
              perror ("realloc");
              break;
            }
        }

      elog_debugw (L"%s: StringCchCatW %S\n", __func__, warg);
      StringCchCatW (res, res_size, warg);
      StringCchCatW (res, res_size, wspace);
    }

  elog_debugw (L"%s: processed %d num_args\n", __func__, num_args);
  elog_debugw (L"%s: res = %S\n", __func__, res);

  return res;
}

#define ARG_ESCAPED_SIZE 4096

/* Escapes a single command line argument. Returns NULL on error.
   The caller is responsible for freeing the resulting string.

   Note, the function iterates the characters byte by byte because it assumes
   the arguments are either in ASCII or UTF-8, and the characters in range
   U+0000 and U+007F (decimal 127) don't interfere with the characters of higher
   code points (the next range starts at U+0080 (decimal 128), but this
   function only works with ASCII characters. */
static char *
escape_arg (const char *arg)
{
#define CHECK_ARG_LENGTH(__delta) \
  do \
    { \
      if (unlikely ((arg_escaped_len + (__delta)) >= ARG_ESCAPED_SIZE)) \
        { \
          elog_error ("argument length exceeds %d\n", ARG_ESCAPED_SIZE); \
          return NULL; \
        } \
    } \
  while (0)

  const char *ch = arg;
  char arg_escaped[ARG_ESCAPED_SIZE] = { '\0' };
  unsigned arg_escaped_len = 0;
  unsigned delta = 0;

  if (unlikely (arg == NULL))
    return NULL;

  /* If there are no special characters in arg, no need to escape. */
  if (strpbrk (arg, " \t\n\v\"") == NULL)
    return strdup (arg);

  /* Append opening double quote */
  arg_escaped[arg_escaped_len++] = '"';

  for (ch = arg; *ch; ++ch)
    {
      unsigned num_backslashes = 0;

      CHECK_ARG_LENGTH (0);

      for (; *ch && *ch == '\\'; ++ch, ++num_backslashes);

      elog_debug ("[%s] ch = %c num_backslashes = %d\n", __func__, *ch, num_backslashes);

      switch (*ch)
        {
        case '\0':
          /* Escape all backslashes */
          if (num_backslashes)
            {
              delta = num_backslashes * 2;
              CHECK_ARG_LENGTH (delta);
              memset (&arg_escaped[arg_escaped_len], '\\', delta);
              arg_escaped_len += delta;
            }
          break;
        case '\"':
          /* Escape all backslashes and the following quotation mark */
          delta = num_backslashes * 2 + 1;
          CHECK_ARG_LENGTH (delta);
          memset (&arg_escaped[arg_escaped_len], '\\', delta);
          arg_escaped_len += delta;
          elog_debug ("[%s] [dquote] appending %c to %s [%d]\n", __func__, *ch, arg_escaped, arg_escaped_len);
          arg_escaped[arg_escaped_len++] = *ch;
          break;
        default:
          if (num_backslashes)
            {
              /* Backslashes are not special */
              CHECK_ARG_LENGTH (num_backslashes);
              memset (&arg_escaped[arg_escaped_len], '\\', num_backslashes);
              arg_escaped_len += num_backslashes;
            }
          elog_debug ("[%s] [default] appending %c to %s [%d]\n", __func__, *ch, arg_escaped, arg_escaped_len);
          arg_escaped[arg_escaped_len++] = *ch;
          break;
        }
    }

  /* Append closing double quote */
  elog_debug ("[%s] [end] appending closing dquote\n", __func__, *ch);
  arg_escaped[arg_escaped_len++] = '"';
  elog_debug ("[%s] [end] result: %s\n", __func__, arg_escaped);

  return strndup (arg_escaped, arg_escaped_len);

#undef CHECK_ARG_LENGTH
}

/* Escapes command line arguments.
   The caller should free the resulting array. Returns NULL on error. */
static char **
escape_args (const char * const *args,
             const unsigned args_num)
{
  bool success = true;
  char **escaped_args = LocalAlloc (LMEM_ZEROINIT, sizeof (char*) * args_num);
  char *escaped = NULL;

  elog_debug ("[%s] called with %d args\n", __func__, args_num);

  if (unlikely (escaped_args == NULL))
    {
      elog_error ("[%s] Failed allocating memory for escaped args\n", __func__);
      return NULL;
    }

  for (int i = 0; i < args_num; ++i)
    {
      elog_debug ("[%s] escaping arg %d\n", __func__, i);
      escaped = escape_arg (args[i]);

      if (unlikely (escaped == NULL && args[i] != NULL))
        {
          success = false;
          break;
        }

      elog_debug ("[%s] escaped_args[%d] = %s\n", __func__, i, escaped);
      escaped_args[i] = escaped;
    }

  if (unlikely (!success))
    {
      free_args (escaped_args, args_num);
      escaped_args = NULL;
    }

  return escaped_args;
}

#endif /* WINDOWS */

int
beectl_shell_exec (const char * const *args,
                   const unsigned args_num)
{
#ifdef WINDOWS
  SECURITY_ATTRIBUTES sa = { 0 };
  STARTUPINFOW si = { 0 };
  PROCESS_INFORMATION pi = { 0 };
  DWORD exit_code = -1;
  DWORD dw_error = 0;
  LPVOID lp_msg_buf = NULL;
  LPVOID lp_display_buf = NULL;
  const char* lpsz_function = __func__;
  bool terminated = false;
  LPWSTR cmd = NULL;
  wchar_t **wargs = NULL;
  char **args_escaped = NULL;

  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  SecureZeroMemory(&si, sizeof si);
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOWDEFAULT;

  args_escaped = escape_args (args, args_num);
  if (unlikely (args_escaped == NULL))
    {
      perror ("escape_args failed");
      goto _ret;
    }

  wargs = convert_single_byte_to_multibyte_array ((const char * const *)args_escaped, args_num);
  if (unlikely (wargs == NULL))
    {
      perror ("convert_single_byte_to_multibyte_array");
      goto _ret;
    }

  if ((cmd = concat_args ((const wchar_t * const *)wargs, args_num)) == NULL)
    {
      perror ("concat_args");
      goto _ret;
    }

  elog_debugw (L"cmd (after concat_args) = %S\n", cmd);
  /*
     Note, the fist argument of CreateProcessW should not be escaped (no quotes around required),
     but it should be properly quoted within `cmd`, e.g.:
     first arg = "C:\\Program Files\\notepad.exe"
     cmd = "\"C:\\Program Files\\notepad.exe\" arg1 arg2 \"arg with spaces\""
  */
  if (!CreateProcessW (NULL,
                       cmd,
                       NULL, /* Process handle not inheritable */
                       NULL, /* Thread handle not inheritable */
                       FALSE, /* Handle inheritance */
                       /*CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT*/0, /* Creation flags*/
                       NULL, /* Use parent's environment */
                       NULL, /* Use parent's starting directory */
                       &si,
                       &pi))
    {
      dw_error = GetLastError ();

      FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    dw_error,
                    MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lp_msg_buf,
                    0,
                    NULL);

      lp_display_buf = (LPVOID)LocalAlloc (LMEM_ZEROINIT,
                                        (lstrlen ((LPCTSTR)lp_msg_buf) + lstrlen ((LPCTSTR)lpsz_function) + 40) * sizeof(TCHAR));
      StringCchPrintf ((LPTSTR) lp_display_buf,
                      LocalSize(lp_display_buf) / sizeof (TCHAR),
                      TEXT ("%s failed with error %d: %s"),
                      lpsz_function, dw_error, lp_msg_buf);
      elog_error ("%s\n", lp_display_buf);

      LocalFree (lp_msg_buf);
      LocalFree (lp_display_buf);
      ExitProcess (dw_error);
      goto _ret;
    }
  elog_debugw (L"Created process for command %S\n", cmd);

  terminated = WaitForSingleObject (pi.hProcess, INFINITE);
  elog_debug ("WaitForSingleObject() returned\n");

  GetExitCodeProcess (pi.hProcess, &exit_code);
  elog_debug ("GetExitCodeProcess() returned %d\n", exit_code);
_ret:
  if (cmd != NULL) free (cmd);
  if (pi.hProcess) CloseHandle (pi.hProcess);
  if (pi.hThread) CloseHandle (pi.hThread);
  if (wargs != NULL) free_argsw (wargs, args_num);
  if (args_escaped != NULL) free_args (args_escaped, args_num);

  return exit_code;
#else /* POSIX */
  pid_t cpid;
  int status = -1;

  elog_debug ("%s (non-WINDOWS)\n", __func__);

  cpid = fork ();

  if (cpid == 0) /* Child */
    {
      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
      /* XXX somehow close inherited file descriptors
         opened in the parent process? */

      execv (args[0], (char * const*) args);
      perror ("execv"); /* execv returns only in case of an error */
    }
  else if (cpid > 0) /* Parent */
    {
      if ((wait (&status)) < 0)
        {
          perror ("wait");
          _exit (1);
        }
    }
  else
    {
      perror ("fork");
      _exit (1);
    }

  return status;
#endif
}
