#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "log.h"
#include "uv.h"
#include "err.h"

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
    int r;

    r = uv_ip4_addr(address, port, &remote_addr); // the remote host

    if (r) {
        return r;
    }

    r = uv_udp_init(loop, &udp_handle);

    if (r) {
        return r;
    }

    struct sockaddr_in local_addr;
    r = uv_ip4_addr("0.0.0.0", 0, &local_addr); // the local host

    if (r) {
        return r;
    }

    r = uv_udp_bind(&udp_handle, (struct sockaddr*) &local_addr, 0);

    if (r) {
        return r;
    }

    return 0;
}

/**
 * Returns the current timestamp in ms.
 */
int get_timestamp()
{
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return ((int) tval.tv_sec * 1000) + ((int) tval.tv_usec / 1000);
}

/**
 * Formats a message and copies it to buf. The resulting format is
 * <level>:<timestamp>:<message>.
 */
void log_format(char* buf, const char* level, const char* format, va_list args)
{
    char pre[1024];

    sprintf(pre, "%s:%d:%s", level, get_timestamp(), format);
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
    uv_udp_send_t* send_req = malloc(sizeof(uv_udp_send_t));
    uv_buf_t bufs[] = {
        { .base = message, .len = strlen(message) }
    };
    send_req->data = message;

    r = uv_udp_send(send_req, &udp_handle, bufs, 1, (struct sockaddr*) &remote_addr, on_send_cb);
    log_check_uv_r(r, "uv_udp_send");
}

/**
 * Generic log function. Writes level and format to a log server and possibly
 * stdout. buf is allocated on the heap here, and is only freed when there is a
 * confirmation that the message has been sent.
 */
void log_write(const char* level, const char* format, ...)
{
    char* buf = malloc(1024);
    va_list args;

    va_start(args, format);
    log_format(buf, level, format, args);
    va_end(args);
    printf("%s\n", buf); // todo: add guard so this is not run in test

    if (uv_is_writable((uv_stream_t*) &udp_handle)) { // todo: will this be a bottleneck?
        log_send(buf);
    }
}
