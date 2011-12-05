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
#include <pwd.h>

#include <config.h>
#include <libxlsh.h>

static sigset_t xlsh_default_sigmask;

// Compatibility functions
size_t libxlsh_strnlen(const char* s, size_t maxlen)
{
  size_t len = 0;
  while(*s++ && ++len < maxlen);
  return len;
}

// Process functions
void libxlsh_proc_sigmask(void)
{
  sigprocmask(0, NULL, &xlsh_default_sigmask);
}

pid_t libxlsh_proc_exec(const char* cmdline, int flags)
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

// Pid read/write functions
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

// File IO functions
int libxlsh_file_read(const char* filename, char** buffer, size_t* bufsize)
{
  FILE*  file;
  size_t filesize, bytes_read;

  if(!(file = fopen(filename, "r")))
    return XLSH_ENOTFOUND;

  fseek(file, 0, SEEK_END);
  filesize = ftell(file);
  fseek(file, 0, SEEK_SET);

  *buffer = malloc(filesize + 1);
  if(bufsize) *bufsize = filesize + 1;
  if(!*buffer) {
    fclose(file);
    return XLSH_ERROR;
  }
  bytes_read = fread(*buffer, 1, filesize, file);
  fclose(file);

  if(bytes_read != filesize) {
    free(*buffer);
    return XLSH_ERROR;
  }
  return XLSH_EOK;
}

