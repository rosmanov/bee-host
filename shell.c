/**
 * Native messaging host for Bee browser extension
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 *
 * Shell-specific stuff.
 */

# include "shell.h"
#include <stdio.h> /* popen */
#include <string.h> /* strdup */

#ifndef WINDOWS
#include <sys/types.h> /* pid_t */
# include <sys/wait.h>
#endif

#ifdef WINDOWS
/* Concatenates command line arguments.
 The returned string must be freed by the caller. */
static char *
concat_args (const char * const *args, const unsigned num_args)
{
  char *res;
  size_t res_size = 128;
  size_t res_len = 0;

  res = malloc (res_size);
  if (unlikely (res == NULL))
    {
      perror ("malloc");
      return res;
    }
  memset (res, 0, res_size);

  for (unsigned i = 0; i < num_args; i++)
    {
      size_t len = strlen(args[i]);

      if (unlikely ((res_len + len + 1) > res_size))
        {
          res_size *= 2;
          res = realloc (res, res_size);
          if (unlikely (res == NULL))
            {
              perror ("realloc");
              break;
            }
        }

      strcat (res, args[i]);
      res_len += len;
    }

  return res;
}
#endif /* WINDOWS */


/* XXX Report Windows errors using GetLastError() */
int
beectl_shell_exec (const char * const *args,
                   const unsigned args_num)
{
#ifdef WINDOWS
  SECURITY_ATTRIBUTES sa = { 0 };
  STARTUPINFOW si = { 0 };
  PROCESS_INFORMATION pi = { 0 };
  DWORD exit_code = -1;
  bool terminated = false;
  char *cmd = NULL;

  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  si.dwFlags = STARTF_USESHOWWINDOW;
  si.hStdOutput = NULL;
  si.hStdError = NULL;
  si.wShowWindow = SW_HIDE;

  if ((cmd = concat_args (args, args_num)) == NULL)
    return -1;

  if (!CreateProcessW (NULL, (LPWSTR) cmd, NULL, NULL, TRUE,
                       CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
    goto _ret;

  do
    {
      terminated = WaitForSingleObject (pi.hProcess, 50) == WAIT_OBJECT_0;
    }
  while (!terminated);

  GetExitCodeProcess (pi.hProcess, &exit_code);
_ret:
  if (cmd != NULL) free (cmd);
  if (pi.hProcess) CloseHandle (pi.hProcess);
  if (pi.hThread) CloseHandle (pi.hThread);

  return exit_code;
#else /* POSIX */
  pid_t cpid;
  int status = -1;

  cpid = fork ();

  if (cpid == 0) /* Child */
    {
      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
      /* XXX somehow close inherited file descriptors
         opened in the parent process? */

      execv (args[0], (char * const*) args);
      perror ("execv");
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
