/* eXtended Login Shell
 * (c) 2011 Michał Siejak
 *
 * See COPYING file for license details.
 */

#ifndef __XLSH_LIBXLSH_H
#define __XLSH_LIBXLSH_H

#define XLSH_VERSION_API    2
#define XLSH_VERSION_STRING "0.2.2"

enum xlsh_ecode {
 XLSH_EOK       = 0x00,
 XLSH_EFATAL    = 0x01,
 XLSH_EDONE     = 0x02,
 XLSH_EARG      = 0x03,
 XLSH_ENOTFOUND = 0x04,
 XLSH_EFOUND    = 0x05,
 XLSH_ERROR     = 0xFF,
};

static const int XLSH_NORMAL    = 0x00;
static const int XLSH_OVERWRITE = 0x01;
static const int XLSH_DETACH    = 0x02;

size_t libxlsh_strnlen(const char* s, size_t maxlen);
void   libxlsh_proc_sigmask(void);
pid_t  libxlsh_proc_exec(const char* cmdline, int flags);
pid_t  libxlsh_pid_read(const char* filename);
int    libxlsh_pid_lock(const char* filename, pid_t pid, int flags);
int    libxlsh_file_read(const char* filename, char** buffer, size_t* bufsize);

#endif
