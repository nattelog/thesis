#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "event_handler.h"
#include "log.h"

int __is_prime(long p)
{
    // this log slows execution down a lot when verbose is on
    // log_verbose("__is_prime:p=%d", p);

    for (long k = 2; k < p; ++k) {
        if (p % k == 0) {
            return 0;
        }
    }

    return 1;
}

long __nth_prime(long n)
{
    log_verbose("__nth_prime:n=%d", n);

    long i = 0;
    long j = 1;

    while (i <= n) {
        if (__is_prime(j++)) {
            ++i;
        }
    }

    return j - 1;
}

/**
 * Perform CPU intensive calculation by calculating the nth prime given an
 * intensity value between 0 and 1.
 */
void event_handler_do_cpu(double intensity)
{
    log_verbose("event_handle_do_cpu:intensity=%f", intensity);

    long n;
    long j;

    if (intensity < 0 || intensity > 1) {
        log_error("__do_cpu:intensity must be between 0 and 1");
        exit(1);
    }

    n = (long) pow(2, 12) * intensity;
    j = __nth_prime(n);

    log_verbose("%dth prime is %d", n, j);
}

void __do_io_sync(double intensity)
{
    log_verbose("__do_io_sync:intensity=%f", intensity);

    long n;
    FILE* fd;

    n = event_handler_calc_io_rounds(intensity);

    for (int i = 0; i < n; ++i) {
        fd = fopen(EVENT_HANDLER_IO_FILE, "a");

        fprintf(fd, "%s", "DOING IO\n");
        fclose(fd);
    }

    unlink(EVENT_HANDLER_IO_FILE);
}

/**
 * Returns the number of rounds the file will be written to given an intensity
 * value between 0 and 1.
 */
long event_handler_calc_io_rounds(double intensity)
{
    log_verbose("event_handler_calc_io_rounds:intensity=%f", intensity);

    if (intensity < 0 || intensity > 1) {
        log_error("event_handler_calc_io_rounds:intensity must be between 0 and 1");
        exit(1);
    }

    return (long) pow(2, 14) * intensity;
}

/**
 * Perfoms CPU and I/O intensive tasks while blocking the rest of the
 * execution.
 */
void event_handler_serial(double cpu_intensity, double io_intensity)
{
    log_verbose("event_handler_serial:cpu_intensity=%f, io_intensity=%f", cpu_intensity, io_intensity);

    event_handler_do_cpu(cpu_intensity);
    __do_io_sync(io_intensity);
}

void event_handler_preemptive_init()
{
    log_verbose("event_handler_preemptive_init");

    pool = thpool_init(EVENT_HANDLER_POOL_SIZE);
}

void __do_preemptive_work(void* args)
{
    log_debug("__do_preemptive_work:args=%p");

    net_tcp_context_sync_t* device = (net_tcp_context_sync_t*) args;
    config_data_t* config = device->config;

    event_handler_do_cpu(config->cpu);

    // pthread_mutex_lock(&event_handler_io_mutex);
    __do_io_sync(config->io);
    // pthread_mutex_unlock(&event_handler_io_mutex);

    log_event_done((char*) device->event);

    pthread_mutex_lock(&device->mutex);
    device->is_processed = 1;
    pthread_mutex_unlock(&device->mutex);
}

void event_handler_preemptive(net_tcp_context_sync_t* device)
{
    log_debug("event_handler_preemptive:device=%p", device);

    device->is_processed = 0;
    thpool_add_work(pool, (void*) __do_preemptive_work, (void*) device);
}
