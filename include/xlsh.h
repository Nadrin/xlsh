/* eXtended Login Shell
 * (c) 2011 Michał Siejak
 *
 * See COPYING file for license details.
 */

#ifndef __XLSH_H
#define __XLSH_H

typedef enum xlsh_config_id_e {
  XLSH_ID_EXEC = 0,
  XLSH_ID_REBOOT,
  XLSH_ID_HALT,
  XLSH_ID_ISSUE,
  XLSH_ID_DATEFMT,
  XLSH_ID_TIMEFMT,
  XLSH_ID_TMPDIR,
  XLSH_ID_PATH,
  XLSH_ID_DISPLAY,
} xlsh_config_id_t;

typedef struct xlsh_config_item_s {
  const char* name;
  const char* defvalue;
  char* value;
} xlsh_config_item_t;

typedef int (*xlsh_func_t)(int argc, char** argv);

typedef struct xlsh_command_s {
  const char* name;
  const char* helptext;
  xlsh_func_t callback;
} xlsh_command_t;

typedef struct xlsh_system_s {
  struct utsname un;
  char date[100];
  char time[100];
  char ttyname[256];
  char ttypath[256];
  char hostname[256];
  char domainname[256];
} xlsh_system_t;

void xlsh_config_init(char* exec_arg);
void xlsh_config_free(void);
void xlsh_config_set(xlsh_config_item_t* item, const char* value);

int xlsh_cmd_loop(void);

char* xlsh_session_getpass(char* buffer, size_t bufsize);
char* xlsh_session_getstring(char* buffer, size_t bufsize);

int   xlsh_session_open(const char* service, const char* user,
												pam_handle_t** handle);
int xlsh_session_close(pam_handle_t* handle);
int xlsh_session_exec(pam_handle_t* handle, const char* session, const char* arg0);

int xlsh_session_tty(const char* user, const char* shell);
int xlsh_session_x(const char* user, const char* shell);

int xlsh_sys_getinfo(xlsh_system_t* sysinfo);
int xlsh_sys_issue(const char* issuefile);

int xlsh_func_exit(int argc, char** argv);
int xlsh_func_help(int argc, char** argv);
int xlsh_func_version(int argc, char** argv);
int xlsh_func_login(int argc, char** argv);
int xlsh_func_reboot(int argc, char** argv);
int xlsh_func_shutdown(int argc, char** argv);

#endif
