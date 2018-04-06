#ifndef __EVENT_HANDLER_h__
#define __EVENT_HANDLER_h__

#include <pthread.h>
#include "thpool.h"
#include "net.h"

#define EVENT_HANDLER_IO_FILE "EVENT_HANDLER_IO_FILE"
#define EVENT_HANDLER_IO_CONTENT "EVENT_HANDLER_IO_CONTENT"
#define EVENT_HANDLER_POOL_SIZE 10

static pthread_mutex_t event_handler_io_mutex = PTHREAD_MUTEX_INITIALIZER;
static threadpool pool;

void event_handler_do_cpu(double intensity);

long event_handler_calc_io_rounds(double intensity);

void event_handler_serial(double cpu_intensity, double io_intensity);

void event_handler_preemptive_init();

void event_handler_preemptive(net_tcp_context_sync_t* device);

#endif
