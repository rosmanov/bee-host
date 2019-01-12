#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char const* argv[])
{
  char *args[] = { "/usr/bin/gvim", "-c", "set ft=markdown", NULL };

  execvp (args[0], (char * const*) (args+sizeof (char *)));
  return 0;
}
