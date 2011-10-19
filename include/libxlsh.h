/* eXtended Login Shell
 * (c) 2011 Michał Siejak
 *
 * See COPYING file for license details.
 */

#ifndef __XLSH_LIBXLSH_H
#define __XLSH_LIBXLSH_H

#define XLSH_VERSION_API    2
#define XLSH_VERSION_STRING "0.2.1"

#define XLSH_EOK       0x00
#define XLSH_EFATAL    0x01
#define XLSH_EDONE     0x02
#define XLSH_EARG      0x03
#define XLSH_ENOTFOUND 0x04
#define XLSH_EFOUND    0x05
#define XLSH_ERROR     0xFF

#define XLSH_NORMAL    0x00
#define XLSH_OVERWRITE 0x01
#define XLSH_DETACH    0x02

void  libxlsh_proc_sigmask(void);
pid_t libxlsh_proc_exec(const char* cmdline, int flags);
pid_t libxlsh_pid_read(const char* filename);
int   libxlsh_pid_lock(const char* filename, pid_t pid, int flags);
int   libxlsh_file_read(const char* filename, char** buffer, size_t* bufsize);

#endif
