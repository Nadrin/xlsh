/* eXtended Login Shell
 * (c) 2011 Michał Siejak
 *
 * See COPYING file for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "libxlsh.h"

static sigset_t xlsh_default_sigmask;

void libxlsh_sigmask(void)
{
  sigprocmask(0, NULL, &xlsh_default_sigmask);
}

pid_t libxlsh_exec(const char* cmdline, int flags)
{
  pid_t pid;
  int i;
  char* buffer;
  char* argv[256];

  pid = fork();
  if(pid < 0) return -1;
  if(pid > 0) return pid;

  buffer  = strdup(cmdline);
  i=0;
  argv[0] = strtok(buffer, " ");
  do {
    argv[++i] = strtok(NULL, " ");
  } while(argv[i] && i<256);

  if(flags & XLSH_DETACH)
    setsid();
  sigprocmask(SIG_SETMASK, &xlsh_default_sigmask, NULL);
  execv(argv[0], argv);  
  exit(EXIT_FAILURE);
}

pid_t libxlsh_pid_read(const char* filename)
{
  pid_t result  = 0;
  FILE* pidfile = fopen(filename, "r");
  if(!pidfile)
    return -1;
  if(fscanf(pidfile, "%d", &result) != 1) {
    fclose(pidfile);
    return -1;
  }
  fclose(pidfile);
  return result;
}

int libxlsh_pid_lock(const char* filename, pid_t pid, int flags)
{
  struct stat statbuf;
  FILE* pidfile;

  if((flags & XLSH_OVERWRITE) && stat(filename, &statbuf) == 0)
    return XLSH_EFOUND;
  
  pidfile = fopen(filename, "w");
  if(!pidfile)
    return XLSH_ERROR;
  fprintf(pidfile, "%d", pid);
  fclose(pidfile);
  return XLSH_EOK;
}
