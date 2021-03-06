#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "log.h"
#include "uv.h"
#include "err.h"

int udp_sock = -1;
struct sockaddr_in remote_addr;

/**
 * Helper function for checking return values from uv functions.
 */
void log_check_uv_r(int r, char* msg)
{
    if (r != 0) {
        log_error("%s: [%s(%d): %s]", msg, uv_err_name(r), r, uv_strerror(r));
        exit(1);
    }
}

void log_check_r(int r, char* msg)
{
    if (r != 0) {
        log_error("%s: [(%d): %s]", msg, r, gw_strerror(r));
        exit(1);
    }
}

/**
 * Initiates the global udp variables. address and port refers to the remote
 * log server. Returns an uv error code if something goes wrong.
 */
int log_init(uv_loop_t* loop, const char* address, const int port)
{
    log_verbose("log_init:loop=%p, address=\"%s\", port=%d", loop, address, port);

    int r;

    r = uv_ip4_addr(address, port, &remote_addr); // the remote host

    if (r) {
        return r;
    }

    struct sockaddr_in local_addr;
    r = uv_ip4_addr("0.0.0.0", 0, &local_addr); // the local host

    if (r) {
        return r;
    }

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (udp_sock < 0) {
        return udp_sock;
    }

    return 0;
}

/**
 * Returns the current timestamp in ms.
 */
unsigned long long get_timestamp()
{
    unsigned long long s;
    unsigned long long ns;
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    s = (unsigned long long) (spec.tv_sec) * 1000;
    ns = (unsigned long long) round(spec.tv_nsec / 1.0e6);

    return s + ns;
}

/**
 * Formats a message and copies it to buf. The resulting format is
 * <level>:<timestamp>:<message>.
 */
void log_format(char* buf, const char* level, const char* format, va_list args)
{
    char pre[1024];

    sprintf(pre, "%s:%llu:%s", level, get_timestamp(), format);
    vsprintf(buf, pre, args);
}

/**
 * Called when the udp message has been sent. Frees the udb request and the
 * message.
 */
void on_send_cb(uv_udp_send_t* req, int status)
{
    log_check_uv_r(status, "on_send_cb");
    free(req->data);
    free(req);
}

/**
 * Send message to the log server.
 */
void log_send(char* message)
{
    int r = 0;

    r = sendto(udp_sock, message, strlen(message), 0, (struct sockaddr*) &remote_addr, sizeof(remote_addr));

    if (r == -1) {
        printf("ERROR:log_send:sendto:-1");
        exit(1);
    }
}

/**
 * Generic log function. Writes level and format to a log server and possibly
 * stdout. buf is allocated on the heap here, and is only freed when there is a
 * confirmation that the message has been sent.
 */
void log_write(const char* level, const char* format, ...)
{
    char buf[16384];
    va_list args;

    va_start(args, format);
    log_format((char*) buf, level, format, args);
    va_end(args);

#ifdef LOGSTD
    printf("%s\n", buf); // todo: add guard so this is not run in test
#endif

    if (strcmp(level, "INFO") == 0 && udp_sock > 0) {
        log_send(buf);
    }
}

void log_event_retrieved(char* event_id)
{
    log_info("gateway:EVENT_LIFECYCLE_RETRIEVED:%s", event_id);
}

void log_event_dispatched(char* event_id)
{
    log_info("gateway:EVENT_LIFECYCLE_DISPATCHED:%s", event_id);
}

void log_event_done(char* event_id)
{
    log_info("gateway:EVENT_LIFECYCLE_DONE:%s", event_id);
}
