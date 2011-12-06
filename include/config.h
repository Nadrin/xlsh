/* eXtended Login Shell
 * (c) 2011 Michał Siejak
 *
 * See COPYING file for license details.
 */

#ifndef __XLSH_CONFIG_H
#define __XLSH_CONFIG_H

// Tweak parameters to suit your needs before compiling.

static const char XLSH_EXEC[]      = "./.xsession";
static const char XLSH_PROMPT[]    = "xlsh (%s)> ";
static const char XLSH_TMPDIR[]    = "/tmp";
static const char XLSH_SHELLS[]    = "/etc/shells";
static const char XLSH_ISSUE[]     = "/etc/issue";
static const char XLSH_PATH[]      = "/bin:/usr/bin:/usr/local/bin";
static const char XLSH_REBOOT[]    = "/sbin/shutdown -r now";
static const char XLSH_HALT[]      = "/sbin/shutdown -h now";
static const char XLSH_XRDB[]      = "/usr/bin/xrdb -remove";
static const char XLSH_XTTY[]      = "/dev/console";
static const char XLSH_XTTY_NAME[] = "X11";
static const char XLSH_DATEFMT[]   = "%Y-%m-%d";
static const char XLSH_TIMEFMT[]   = "%H:%M";
static const char XLSH_PAM_TTY[]   = "login";
static const char XLSH_PAM_X11[]   = "xlshd";

static const int XLSH_COMPLETION_SHOWROOT = 1;
static const int XLSH_COMPLETION_MINUID   = 1000;
static const int XLSH_COMPLETION_MAXUID   = 65534;

static const char XLSHD_TERM[]     = "/usr/bin/xterm -g 80x15+0+0 -fg #ffd7af -bg #1f1f1f -cr #b99b86 -fn fixed +sb -e /usr/local/sbin/xlsh";
static const char XLSHD_PIDFILE[]  = "/var/run/xlshd.pid";
static const char XLSHD_XSERVER[]  = "/usr/bin/X";
static const char XLSHD_XOPTIONS[] = "-nolisten tcp -noreset";
static const char XLSHD_XDISPLAY[] = ":0";
static const int XLSHD_XWAIT    = 1;
static const int XLSHD_XRETRY   = 2;

#endif
