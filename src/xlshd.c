/* eXtended Login Shell (X daemon)
 * (c) 2011 Michał Siejak
 *
 * See COPYING file for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <config.h>
#include <libxlsh.h>

static void xlshd_usage(const char* argv0)
{
  fprintf(stderr, "eXtended Login Shell X daemon, version %s\n", XLSH_VERSION_STRING);
  fprintf(stderr, "usage: %s [-f|-h] [display]\n", argv0);
  exit(EXIT_FAILURE);
}

static void xlshd_sig_daemonize(int signum)
{
  switch(signum) {
  case SIGALRM: exit(EXIT_FAILURE);
  case SIGCHLD: exit(EXIT_FAILURE);
  case SIGUSR1: exit(EXIT_SUCCESS);
  }
}

static volatile sig_atomic_t xlshd_quit = 0;
static void xlshd_sig_quit(int signum)
{
  xlshd_quit = 1;
}

void xlshd_daemonize(const char* argv0)
{
  pid_t pid;
  struct sigaction sighandler;
  struct sigaction def_sigchld, def_sigusr1, def_sigalrm;

  memset(&sighandler, 0, sizeof(struct sigaction));
  sighandler.sa_handler = xlshd_sig_daemonize;

  sigaction(SIGCHLD, &sighandler, &def_sigchld);
  sigaction(SIGUSR1, &sighandler, &def_sigusr1);
  sigaction(SIGALRM, &sighandler, &def_sigalrm);

  pid = fork();
  if(pid < 0) {
    fprintf(stderr, "%s: Could not daemonize: %s\n", argv0, strerror(errno));
    exit(EXIT_FAILURE);
  }
  if(pid > 0) {
    alarm(2);
    pause();
    exit(EXIT_FAILURE);
  }

  sigaction(SIGCHLD, &def_sigchld, NULL);
  sigaction(SIGUSR1, &def_sigusr1, NULL);
  sigaction(SIGALRM, &def_sigalrm, NULL);

  umask(S_IWGRP | S_IWOTH);
  chdir("/");
  if(setsid() < 0)
    exit(EXIT_FAILURE);

  kill(getppid(), SIGUSR1);
}

int main(int argc, char** argv)
{
  int  opt_index          = 1;
  int  opt_nodaemon       = 0;

  const char* opt_display = XLSHD_XDISPLAY;

  char buffer[PATH_MAX];
  pid_t lock_pid     = 0;
  pid_t xserver_pid  = 0;
  pid_t xterm_pid    = 0;
  pid_t xsession_pid = 0;

  sigset_t sigmask;
  struct sigaction sighandler;
  int waitflag, retval;

  if(argc > opt_index && strcmp(argv[opt_index], "-h")==0)
    xlshd_usage(argv[0]);
  if(argc > opt_index && strcmp(argv[opt_index], "-f")==0) {
    opt_nodaemon = 1;
    opt_index++;
  }
  if(argc > opt_index)
    opt_display = argv[opt_index];

  if(geteuid() != 0) {
    fprintf(stderr, "%s: You need to have root privileges\n", argv[0]);
    return EXIT_FAILURE;
  }

  lock_pid = libxlsh_pid_lock(XLSHD_PIDFILE, getpid(), 0);
  if(lock_pid == -1) {
    fprintf(stderr, "%s: Cannot create lockfile: %s\n",
            argv[0], XLSHD_PIDFILE);
    return EXIT_FAILURE;
  } else if(lock_pid > 0) {
    fprintf(stderr, "%s: Lockfile %s is in use by %d, aborted\n",
            argv[0], XLSHD_PIDFILE, lock_pid);
    return EXIT_FAILURE;
  }

  if(!opt_nodaemon) {
    xlshd_daemonize(argv[0]);
    lock_pid = libxlsh_pid_lock(XLSHD_PIDFILE, getpid(), XLSH_OVERWRITE);
    if(lock_pid == -1) {
      fprintf(stderr, "%s: Cannot write lockfile: %s\n",
              argv[0], XLSHD_PIDFILE);
      return EXIT_FAILURE;
    }
  }

  stdin  = freopen("/dev/null", "r", stdin);
  stdout = freopen("/dev/null", "w", stdout);
  stderr = freopen("/dev/null", "w", stderr);

  libxlsh_proc_sigmask();

  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGHUP);
  sigaddset(&sigmask, SIGUSR1);
  sigaddset(&sigmask, SIGUSR2);
  sigprocmask(SIG_BLOCK, &sigmask, NULL);

  memset(&sighandler, 0, sizeof(struct sigaction));
  sighandler.sa_handler = xlshd_sig_quit;
  sigaction(SIGTERM, &sighandler, NULL);
  sigaction(SIGINT, &sighandler, NULL);

  retval = EXIT_SUCCESS;
  while(!xlshd_quit) {
    xserver_pid  = 0;
    xsession_pid = 0;
    xterm_pid      = 0;

    snprintf(buffer, PATH_MAX, "%s %s %s", XLSHD_XSERVER, XLSHD_XOPTIONS, opt_display);
    if((xserver_pid = libxlsh_proc_exec(buffer, 0)) < 0) {
      retval = EXIT_FAILURE;
      break;
    }

    setenv("DISPLAY", opt_display, 1);
    sleep(XLSHD_XWAIT);

    snprintf(buffer, PATH_MAX, "%s", XLSHD_TERM);
    if((xterm_pid = libxlsh_proc_exec(buffer, 0)) < 0) {
      kill(xserver_pid, SIGTERM);
      retval = EXIT_FAILURE;
      break;
    }

    waitflag = 1;
    waitpid(xterm_pid, &waitflag, 0);
    if(xlshd_quit) break;

    snprintf(buffer, PATH_MAX, "%s/.xlsh-%d.pid", XLSH_TMPDIR, xterm_pid);
    xsession_pid = libxlsh_pid_read(buffer);
    if(xsession_pid > 0) {
      while(!kill(xsession_pid, 0) && !xlshd_quit)
        sleep(XLSHD_XWAIT);
      unlink(buffer);
    }

    if(xlshd_quit) break;

    waitflag = -1;
    kill(xserver_pid, SIGTERM);
    wait(&waitflag);
    sleep(XLSHD_XRETRY);
  }

  if(xsession_pid > 0) {
    kill(xsession_pid, SIGTERM);
    sleep(XLSHD_XWAIT);
  }
  if(xterm_pid > 0)
    kill(xterm_pid, SIGTERM);
  if(xserver_pid > 0) {
    waitflag = -1;
    kill(xserver_pid, SIGTERM);
    wait(&waitflag);
  }

  unlink(XLSHD_PIDFILE);
  return retval;
}
