#ifndef __LOG_h__
#define __LOG_h__

#include <stdarg.h>
#include "uv.h"

#ifndef LOGLEVEL
#define LOGLEVEL 4
#endif

#define LOGSTD

static int udp_sock = -1;
static struct sockaddr_in remote_addr;

void log_check_uv_r(int r, char* msg);

void log_check_r(int r, char* msg);

int log_init(uv_loop_t* loop, const char* address, const int port);

unsigned long long get_timestamp();

void log_format(char* buf, const char* level, const char* format, va_list args);

void log_write(const char* level, const char* format, ...);

void log_event_retrieved(char* event_id);

void log_event_dispatched(char* event_id);

void log_event_done(char* event_id);

#define log_error(M, ...) log_write("ERROR", M, ##__VA_ARGS__)
#define log_info(M, ...) log_write("INFO", M, ##__VA_ARGS__)
#define log_debug(M, ...) log_write("DEBUG", M, ##__VA_ARGS__)
#define log_verbose(M, ...) log_write("VERBOSE", M, ##__VA_ARGS__)

#if LOGLEVEL < 4
#undef log_verbose
#define log_verbose(M, ...)
#endif

#if LOGLEVEL < 3
#undef log_debug
#define log_debug(M, ...)
#endif

#if LOGLEVEL < 2
#undef log_info
#define log_info(M, ...)
#endif

#if LOGLEVEL < 1
#undef log_error
#define log_error(M, ...)
#endif

#endif
