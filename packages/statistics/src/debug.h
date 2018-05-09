#ifndef _DEBUG__H_
#define _DEBUG__H_

#include <syslog.h>
#include <stdarg.h>

enum {
    L_CRIT,
    L_ERR,
    L_WARNING,
    L_NOTICE,
    L_INFO,
    L_DEBUG
};

void log_message(int priority, const char *format, ...);
void set_debug_level(int level);

#endif
