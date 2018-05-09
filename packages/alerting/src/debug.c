#include <stdint.h>
#include <stdio.h>
#include "debug.h"

static uint8_t use_syslog = 1;
static uint8_t debug_level = LOG_INFO;

static const int log_class[] = {
    [L_CRIT] = LOG_CRIT,
    [L_ERR] = LOG_ERR,
    [L_WARNING] = LOG_WARNING,
    [L_NOTICE] = LOG_NOTICE,
    [L_INFO] = LOG_INFO,
    [L_DEBUG] = LOG_DEBUG
};


void log_message(int priority, const char *format, ...)
{
    va_list vl;

    if (priority > debug_level)
        return;

    va_start(vl, format);
    
    if (use_syslog)
        vsyslog(log_class[priority], format, vl);
    else
        vfprintf(stderr, format, vl);
    
    va_end(vl);
}

void set_debug_level(int level)
{
    debug_level = level;
}

