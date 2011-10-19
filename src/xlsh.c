/* eXtended Login Shell
 * (c) 2011 Michał Siejak
 *
 * See COPYING file for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <security/pam_appl.h>

#include <config.h>
#include <libxlsh.h>
#include <xlsh.h>

// Static data
static xlsh_config_item_t xlsh_config[] = {
  { "XLSH_EXEC", XLSH_EXEC, NULL },
  { "XLSH_REBOOT", XLSH_REBOOT, NULL },
  { "XLSH_HALT", XLSH_HALT, NULL },
  { "XLSH_ISSUE", XLSH_ISSUE, NULL },
  { "XLSH_DATEFMT", XLSH_DATEFMT, NULL },
  { "XLSH_TIMEFMT", XLSH_TIMEFMT, NULL },
  { "XLSH_TMPDIR", XLSH_TMPDIR, NULL },
  { "PATH", XLSH_PATH, NULL },
  { "DISPLAY", NULL, NULL },
  { NULL, NULL, NULL },
};

static xlsh_command_t xlsh_commands[] = {
  { "login", "   : Logins specified user into the system", xlsh_func_login },
  { "reboot", "  : Reboots the system", xlsh_func_reboot },
  { "shutdown", ": Halts the system", xlsh_func_shutdown },
  { "exit", "    : Exits (reloads) login shell", xlsh_func_exit },
  { "help", "    : Prints all available commands", xlsh_func_help },
  { "version", " : Prints copyright and version information", xlsh_func_version },
  { NULL, NULL, NULL },
};

static int xlsh_X = 0;

static void xlsh_usage(char* argv0)
{
  fprintf(stderr, "eXtended Login Shell, version %s\n", XLSH_VERSION_STRING);
  fprintf(stderr, "usage: %s [xsession]\n", argv0);
  exit(EXIT_FAILURE);
}

static void xlsh_version(void)
{
  printf("eXtended Login Shell, version %s\n", XLSH_VERSION_STRING);
  printf("(c) 2011 Michal Siejak, released under GPLv3\n");
}

// Command callbacks
int xlsh_func_exit(int argc, char** argv)
{
  return XLSH_EDONE;
}

int xlsh_func_help(int argc, char** argv)
{
  xlsh_command_t* ptr = xlsh_commands;

  printf("Command list:\n");
  do {
    printf(" * %s %s\n", ptr->name, ptr->helptext);
  } while((++ptr)->name);
  return XLSH_EOK;
}

int xlsh_func_version(int argc, char** argv)
{
  xlsh_version();
  return XLSH_EOK;
}

int xlsh_func_login(int argc, char** argv)
{
  char* arg_user  = argc>=2?argv[1]:NULL;
  char* arg_shell = argc>=3?argv[2]:NULL;

  if(!arg_user) {
    fprintf(stderr, "usage: %s <username> [shell/wm]\n", argv[0]);
    return XLSH_EARG;
  }

  if(xlsh_X) {
    if(!arg_shell)
      arg_shell = xlsh_config[XLSH_ID_EXEC].value;
    return xlsh_session_x(arg_user, arg_shell);
  }
  else
    return xlsh_session_tty(arg_user, arg_shell);
}

int xlsh_func_reboot(int argc, char** argv)
{
  pid_t xlshd_pid;

  printf("Initiating system reboot ...\n");
  if(libxlsh_proc_exec(XLSH_REBOOT, XLSH_DETACH) == -1) {
    fprintf(stderr, "Failed to execute: %s\n", XLSH_REBOOT);
    return XLSH_ERROR;
  }

  xlshd_pid = libxlsh_pid_read(XLSHD_PIDFILE);
  if(xlshd_pid > 0)
    kill(xlshd_pid, SIGTERM);

  pause();
  return XLSH_EDONE;
}

int xlsh_func_shutdown(int argc, char** argv)
{
  pid_t xlshd_pid;

  printf("Initiating system shutdown ...\n");
  if(libxlsh_proc_exec(XLSH_HALT, XLSH_DETACH) == -1) {
    fprintf(stderr, "Failed to execute: %s\n", XLSH_HALT);
    return XLSH_ERROR;
  }

  xlshd_pid = libxlsh_pid_read(XLSHD_PIDFILE);
  if(xlshd_pid > 0)
    kill(xlshd_pid, SIGTERM);

  pause();
  return XLSH_EDONE;
}

// Session management helpers
static int xlsh_session_conv(int num_msg, const struct pam_message** msg,
			     struct pam_response** resp, void* appdata_ptr)
{
  int i;
  char buffer[256];
  size_t resp_size = num_msg * sizeof(struct pam_response);
  
  *resp = malloc(resp_size);
  memset(*resp, 0, resp_size);
  
  for(i=0; i<num_msg; i++) {
    switch(msg[i]->msg_style) {
    case PAM_TEXT_INFO:
      printf("%s\n", msg[i]->msg);
      break;
    case PAM_ERROR_MSG:
      fprintf(stderr, "Error: %s\n", msg[i]->msg);
      break;
    case PAM_PROMPT_ECHO_ON:
      printf("%s ", msg[i]->msg);
      fflush(stdout);
      (*resp)[i].resp = strdup(xlsh_session_getstring(buffer, 256));
      break;
    case PAM_PROMPT_ECHO_OFF:
      printf("%s", msg[i]->msg);
      fflush(stdout);
      (*resp)[i].resp = strdup(xlsh_session_getpass(buffer, 256));
      break;
    }
  }
  return PAM_SUCCESS;
}

// Session management
char* xlsh_session_getshell(char* buffer, const char* shell, size_t bufsize)
{
  char  readbuf[bufsize];
  FILE* fsh = fopen(XLSH_SHELLS, "r");

  buffer[0] = 0;
  while(!feof(fsh)) {
    fgets(readbuf, bufsize, fsh);
    if(readbuf[0] != '/') continue;
    if(strstr(readbuf, shell)) {
      sscanf(readbuf, "%s", buffer);
      break;
    }
  }
  fclose(fsh);
  return buffer[0]?buffer:NULL;
}

char* xlsh_session_getpass(char* buffer, size_t bufsize)
{
  struct termios termflags;
  tcflag_t local_mode;
  size_t pass_len;

  tcgetattr(0, &termflags);
  local_mode = termflags.c_lflag;
  termflags.c_lflag &= ~ECHO;
  tcsetattr(0, TCSANOW, &termflags);

  fflush(stdin);
  fgets(buffer, bufsize, stdin);
  pass_len = strnlen(buffer, bufsize-1);
  if(buffer[pass_len-1] == '\n')
    buffer[pass_len-1] = 0;

  termflags.c_lflag = local_mode;
  tcsetattr(0, TCSANOW, &termflags);
  fputc('\n', stdout);
  return buffer;
}

char* xlsh_session_getstring(char* buffer, size_t bufsize)
{
  size_t string_len;
  
  fflush(stdin);
  fgets(buffer, bufsize, stdin);
  string_len = strnlen(buffer, bufsize-1);
  
  if(buffer[string_len-1] == '\n')
    buffer[string_len-1] = 0;
  return buffer;
}

int xlsh_session_open(const char* service, const char* user,
		      pam_handle_t** handle)
{
  struct pam_conv conv = { xlsh_session_conv, NULL };
  pam_handle_t* pam_handle;

  if(pam_start(service, user, &conv, &pam_handle) != PAM_SUCCESS)
    return XLSH_ERROR;
  
  if(xlsh_X)
    pam_set_item(pam_handle, PAM_TTY, XLSH_XTTY);
  else
    pam_set_item(pam_handle, PAM_TTY, ttyname(0));

  if(pam_authenticate(pam_handle, 0) != PAM_SUCCESS) {
    pam_end(pam_handle, 0);
    return XLSH_ERROR;
  }
  if(pam_acct_mgmt(pam_handle, 0) != PAM_SUCCESS) {
    pam_end(pam_handle, 0);
    return XLSH_ERROR;
  }
  if(pam_setcred(pam_handle, PAM_ESTABLISH_CRED) != PAM_SUCCESS) {
    pam_end(pam_handle, 0);
    return XLSH_ERROR;
  }
  if(pam_open_session(pam_handle, 0) != PAM_SUCCESS) {
    pam_setcred(pam_handle, PAM_DELETE_CRED);
    pam_end(pam_handle, 0);
    return XLSH_ERROR;
  }
  
  *handle = pam_handle;
  return XLSH_EOK;
}

int xlsh_session_close(pam_handle_t* handle)
{
  pam_close_session(handle, 0);
  if(pam_setcred(handle, PAM_DELETE_CRED) != PAM_SUCCESS) {
    pam_end(handle, 0);
    return XLSH_ERROR;
  }
  
  pam_end(handle, 0);
  return XLSH_EOK;
}

int xlsh_session_exec(pam_handle_t* handle, const char* session, const char* arg0)
{
  struct passwd* pwinfo;
  const char* pwname;
  char terminal[256];
  pid_t proc_shell;
  int   proc_wait = 0;

  const char* _arg0 = arg0;
  if(!arg0) _arg0 = session;

  pam_get_item(handle, PAM_USER, (const void**)&pwname);
  pwinfo = getpwnam(pwname);
  if(!pwinfo)
    return XLSH_ERROR;

  if((proc_shell = fork()) == 0) {
    chdir(pwinfo->pw_dir);
    
    if(initgroups(pwname, pwinfo->pw_gid) == -1)
      exit(EXIT_FAILURE);
    if(setgid(pwinfo->pw_gid) == -1)
      exit(EXIT_FAILURE);
    if(setuid(pwinfo->pw_uid) == -1)
      exit(EXIT_FAILURE);

    if(getenv("TERM"))
      strncpy(terminal, getenv("TERM"), 256);
    else
      terminal[0] = 0;
    
    clearenv();
    setenv("USER", pwinfo->pw_name, 1);
    setenv("LOGNAME", pwinfo->pw_name, 1);
    setenv("HOME", pwinfo->pw_dir, 1);
    setenv("PATH", xlsh_config[XLSH_ID_PATH].value, 1);
    
    if(xlsh_X) {
      setenv("DISPLAY", xlsh_config[XLSH_ID_DISPLAY].value, 1);
      if(libxlsh_proc_exec(XLSH_XRDB, 0) > 0)
	wait(&proc_wait);
    }
    else
      setenv("SHELL", session, 1);
		
    if(terminal[0])
      setenv("TERM", terminal, 1);

    execlp(session, _arg0, (char*)0);
    exit(EXIT_FAILURE);
  }
  else if(proc_shell == -1)
    return XLSH_ERROR;
  
  return XLSH_EOK;
}

int xlsh_session_tty(const char* user, const char* shell)
{
  pam_handle_t* pam_handle;
  struct passwd* pwinfo;
  int waitflag;
  
  char user_shell[PATH_MAX];
  char user_shell_name[PATH_MAX];
  
  if(xlsh_session_open(XLSH_PAM_TTY, user, &pam_handle) != XLSH_EOK) {
    fprintf(stderr, "Authorization failed\n");
    return XLSH_ERROR;
  }

  pwinfo = getpwnam(user);
  if(!pwinfo) {
    fprintf(stderr, "Failed to retrieve account information from system database\n");
    xlsh_session_close(pam_handle);
    return XLSH_ERROR;
  }

  if(!shell)
    strncpy(user_shell, pwinfo->pw_shell, PATH_MAX);
  else if(!xlsh_session_getshell(user_shell, shell, PATH_MAX)) {
    fprintf(stderr, "Invalid shell name: %s\n", shell);
    xlsh_session_close(pam_handle);
    return XLSH_ERROR;
  }

  sprintf(user_shell_name, "-%s", user_shell);
  if(xlsh_session_exec(pam_handle, user_shell, user_shell_name) != XLSH_EOK) {
    fprintf(stderr, "Cannot execute shell process: %s\n", user_shell);
    xlsh_session_close(pam_handle);
    return XLSH_ERROR;
  }

  waitflag = 0;
  wait(&waitflag);
  
  xlsh_session_close(pam_handle);
  return XLSH_EDONE;
}

int xlsh_session_x(const char* user, const char* shell)
{
  pid_t proc_session;
  int   proc_result;
  char  buffer[PATH_MAX];

  sigset_t sigset[2];
  int      waitflag;
  
  pam_handle_t* pam_handle;
  
  if((proc_session = fork()) == 0) {
    if(xlsh_session_open(XLSH_PAM_X11, user, &pam_handle) != XLSH_EOK) {
      fprintf(stderr, "Authorization failed\n");
      exit(EXIT_FAILURE);
    }

    setsid();
    if(xlsh_session_exec(pam_handle, shell, NULL) != XLSH_EOK) {
      fprintf(stderr, "Cannot execute shell process: %s\n", shell);
      exit(EXIT_FAILURE);
    }

    waitflag = 0;
    kill(getppid(), SIGUSR1);
    wait(&waitflag);
    
    xlsh_session_close(pam_handle);
    exit(EXIT_SUCCESS);
  }
  else {
    if(proc_session == -1) {
      fprintf(stderr, "Could not fork child process: %s\n", strerror(errno));
      return XLSH_ERROR;
    }

    sigemptyset(&sigset[0]);
    sigaddset(&sigset[0], SIGCHLD);
    sigaddset(&sigset[0], SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset[0], &sigset[1]);
    
    snprintf(buffer, PATH_MAX, "%s/.xlsh-%d.pid",
	     xlsh_config[XLSH_ID_TMPDIR].value, getppid());
    libxlsh_pid_lock(buffer, proc_session, 1);
    
    proc_result = sigwaitinfo(&sigset[0], NULL);
    sigprocmask(SIG_SETMASK, &sigset[1], NULL);

    switch(proc_result) {
    case SIGCHLD:
      unlink(buffer);
      return XLSH_ERROR;
    case SIGUSR1:
      return XLSH_EDONE;
    default:
      fprintf(stderr, "wait() syscall failed for session process: %s\n",
	      strerror(errno));
      unlink(buffer);
      return XLSH_ERROR;
    }
  }
    
  return XLSH_EOK;
}

// Configuration
void xlsh_config_init(char* exec_arg)
{     
  if(exec_arg)
    xlsh_config_set(&xlsh_config[XLSH_ID_EXEC], exec_arg);
  else
    xlsh_config_set(&xlsh_config[XLSH_ID_EXEC], NULL);

  xlsh_config_set(&xlsh_config[XLSH_ID_REBOOT], NULL);
  xlsh_config_set(&xlsh_config[XLSH_ID_HALT], NULL);
  xlsh_config_set(&xlsh_config[XLSH_ID_ISSUE], NULL);
  xlsh_config_set(&xlsh_config[XLSH_ID_TIMEFMT], NULL);
  xlsh_config_set(&xlsh_config[XLSH_ID_DATEFMT], NULL);
  xlsh_config_set(&xlsh_config[XLSH_ID_TMPDIR], NULL);
  xlsh_config_set(&xlsh_config[XLSH_ID_PATH], NULL);
  xlsh_config_set(&xlsh_config[XLSH_ID_DISPLAY], NULL);
}

void xlsh_config_free(void)
{
  xlsh_config_item_t* ptr = xlsh_config;
  do { free(ptr->value); } while((++ptr)->name);
}

void xlsh_config_set(xlsh_config_item_t* item, const char* value)
{
  free(item->value);
  if(value)
    item->value = strdup(value);
  else {
    const char* defvalue = getenv(item->name);
    if(!defvalue)
      defvalue = item->defvalue;
    item->value = defvalue?strdup(defvalue):NULL;
  }
}

// Command processing
static char* xlsh_cmd_readline(const char* prompt)
{
  char* line = NULL;
  for(;;) {
    free(line);
    line = readline(prompt);
    if(!line)
      return NULL;
    if(line[0]) {
      add_history(line);
      return line;
    }
  }
}

static char* xlsh_cmd_match_command(const char* text, int state)
{
  static size_t index, len;
  char* cmd_name;

  if(!state) {
    index = 0;
    len   = strlen(text);
  }	
  while((cmd_name = (char*)xlsh_commands[index++].name)) {
    if(strncmp(cmd_name, text, len) == 0)
      return strdup(cmd_name);
  }
  return NULL;
}

static char* xlsh_cmd_match_user(const char* text, int state)
{
  static size_t len;
  static struct passwd* pw_entry;
  int force_show = 0;

  if(!state) {
    setpwent();
    len = strlen(text);
  }
  while((pw_entry = getpwent())) {
#if XLSH_COMPLETION_SHOWROOT == 1
    force_show = pw_entry->pw_uid?0:1;
#endif
    if(!force_show &&
       (pw_entry->pw_uid < XLSH_COMPLETION_MINUID ||
	pw_entry->pw_uid > XLSH_COMPLETION_MAXUID))
      continue;
    if(strncmp(pw_entry->pw_name, text, len) == 0)
      return strdup(pw_entry->pw_name);
  }
  endpwent();
  return NULL;
}

static char** xlsh_cmd_complete(const char* text, int start, int end)
{
  rl_attempted_completion_over = 1;
  if(start == 0)
    return rl_completion_matches((char*)text,
				 xlsh_cmd_match_command);
  else
    return rl_completion_matches((char*)text,
				 xlsh_cmd_match_user);
}

int xlsh_cmd_loop(void)
{
  int  i, cmd_argc;
  char *cmd_argv[256];
  char *line, *argptr;

  char prompt[256];
  xlsh_system_t sysinfo;
  xlsh_command_t* command = NULL;
  
  int retvalue = XLSH_EOK;

  xlsh_sys_getinfo(&sysinfo);
  snprintf(prompt, 256, XLSH_PROMPT, sysinfo.ttyname);

  rl_attempted_completion_function = xlsh_cmd_complete;
  while((line = xlsh_cmd_readline(prompt))) {
    cmd_argc = 0;
    argptr   = strtok(line, " ");
    while(argptr && cmd_argc < 256) {
      cmd_argv[cmd_argc++] = strdup(argptr);
      argptr = strtok(NULL, " ");
    }

    command = xlsh_commands;
    do {
      if(strcmp(command->name, cmd_argv[0]) == 0)
	break;
    } while((++command)->name);

    if(command->name)
      retvalue = command->callback(cmd_argc, cmd_argv);
    else
      fprintf(stderr, "Unknown command: %s\n", cmd_argv[0]);
    
    for(i=0; i<cmd_argc; i++)
      free(cmd_argv[i]);
    free(line);

    if(retvalue == XLSH_EDONE || retvalue == XLSH_EFATAL)
      break;
  }
  
  if(retvalue == XLSH_EOK || retvalue == XLSH_EDONE)
    return EXIT_SUCCESS;
  return EXIT_FAILURE;
}

// System information
int xlsh_sys_getinfo(xlsh_system_t* sysinfo)
{
  struct tm *tminfo;
  time_t timeval;
  
  char* disp_name, *tty_name;
  char  tty_path[PATH_MAX];
  
  memset(sysinfo, 0, sizeof(xlsh_system_t));
  uname(&sysinfo->un);
  if(gethostname(sysinfo->hostname, sizeof(sysinfo->hostname)) != 0)
    strcpy(sysinfo->hostname, "localhost");
  if(getdomainname(sysinfo->domainname, sizeof(sysinfo->domainname)) != 0)
    strcpy(sysinfo->domainname, "localdomain");
  if(ttyname_r(0, tty_path, sizeof(tty_path)) != 0)
    strcpy(tty_path, XLSH_XTTY);
  strncpy(sysinfo->ttypath, tty_path + 5, sizeof(sysinfo->ttypath));
  
  if(xlsh_X) {
    disp_name = getenv("DISPLAY");
    if(disp_name[0] == ':')
      disp_name++;
    sprintf(tty_path, "%s/%s", XLSH_XTTY_NAME, disp_name);
    tty_name = tty_path;
  }
  else
    tty_name = tty_path + 5;
  strncpy(sysinfo->ttyname, tty_name, sizeof(sysinfo->ttyname));
  
  timeval = time(NULL);
  tminfo  = localtime(&timeval);
  if(tminfo) {
    strftime(sysinfo->time, sizeof(sysinfo->time),
	     xlsh_config[XLSH_ID_TIMEFMT].value, tminfo);
    strftime(sysinfo->date, sizeof(sysinfo->date),
	     xlsh_config[XLSH_ID_DATEFMT].value, tminfo);
  }
  else
    return XLSH_ERROR;
  
  return XLSH_EOK;
}

int xlsh_sys_issue(const char* issuefile)
{
  int errflag;
    
  char*  buffer;
  size_t buffer_size;
  char  *curptr, *nextptr, *endptr;
  
  const char* value;
  xlsh_system_t sysinfo;

  if(!issuefile)
    return XLSH_EOK;
  if((errflag = libxlsh_file_read(issuefile, &buffer, &buffer_size)) != XLSH_EOK)
    return errflag;

  xlsh_sys_getinfo(&sysinfo);
  
  curptr = buffer;
  endptr = buffer + buffer_size - 1;
  do {
    nextptr = strchr(curptr, '\\');
    if(nextptr) {
      *nextptr = 0;
      switch(*(++nextptr)) {
      case 's': value = sysinfo.un.sysname; break;
      case 'm': value = sysinfo.un.machine; break;
      case 'r': value = sysinfo.un.release; break;
      case 'v': value = sysinfo.un.version; break;
      case 't': value = sysinfo.time; break;
      case 'd': value = sysinfo.date; break;
      case 'l': value = sysinfo.ttypath; break;
      case 'n': value = sysinfo.hostname; break;
      case 'o': value = sysinfo.domainname; break;
      default:  value = NULL;
      }

      curptr += (printf("%s", curptr) + 2);
      if(value)
	printf("%s", value);
    }
    else
      curptr += printf("%s", curptr);
  } while(curptr < endptr);

  free(buffer);
  return XLSH_EOK;
}

// Main program
int main(int argc, char** argv)
{
  char*    opt_exec = NULL;
  int      retvalue;
  sigset_t sigmask;
  
  if(argc > 1) {
    if(strcmp(argv[1], "-h") == 0)
      xlsh_usage(argv[0]);
    opt_exec = argv[1];
  }

  if(geteuid() != 0) {
    fprintf(stderr, "%s: You need to have root privileges\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  libxlsh_proc_sigmask();
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGHUP);
  sigprocmask(SIG_BLOCK, &sigmask, NULL);

  if(getenv("DISPLAY"))
    xlsh_X = 1;
  
  xlsh_config_init(opt_exec);
  xlsh_sys_issue(xlsh_config[XLSH_ID_ISSUE].value);
  
  retvalue = xlsh_cmd_loop();
  
  xlsh_config_free();
  return retvalue;
}
