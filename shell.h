/**
 * Native messaging host for Bee browser extension
 * Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>
 *
 * Shell-specific stuff.
 */
#ifndef BEECTL_SHELL_H
# define BEECTL_SHELL_H

/* Executes a shell command. Blocks until the command terminates.
   Returns exit code of the command.
   `args` - editor executable followed by optional command line arguments.
   `args_num` - the number of arguments in `args`. */
int beectl_shell_exec (const char * const *args, const unsigned args_num);

#endif /* BEECTL_SHELL_H */
