#include <stdarg.h>
#include <string.h>
#include "net.h"
#include "log.h"
#include "err.h"

/**
 * Initializes the context. Allocates memory for the address struct. Returns an
 * uv error code if something goes wrong.
 */
int net_tcp_context_init(
        net_tcp_context_t* context,
        uv_loop_t* loop,
        char* address,
        int port)
{
    int r;
    struct sockaddr* addr = malloc(sizeof(struct sockaddr));

    r = uv_ip4_addr(address, port, (struct sockaddr_in*) addr);

    if (r) {
        return r;
    }

    context->addr = addr;
    context->loop = loop;

    return 0;
}

/**
 * Casts payload and updates the state field.
 */
net_tcp_context_t* net_get_context(state_t* state, void* payload)
{
    net_tcp_context_t* context = (net_tcp_context_t*) payload;
    context->state = state;
    return context;
}

/**
 * Run when the tcp handle retrieves a connection request.
 */
void __net_on_connection(uv_connect_t* req, int status)
{
    log_check_uv_r(status, "on_connection_cb");

    net_tcp_context_t* context = (net_tcp_context_t*) req->data;
    char* edge_name = context->data;

    free(req);
    state_run_next(context->state, edge_name, context);
}

int net_connect(net_tcp_context_t* context, char* edge_name)
{
    log_debug("net_connect");

    int r;
    uv_tcp_t* handle = malloc(sizeof(uv_tcp_t));
    uv_loop_t* loop = context->loop;
    uv_connect_t* connect_req = malloc(sizeof(uv_connect_t));

    r = uv_tcp_init(loop, handle);

    if (r) {
        return r;
    }

    context->handle = handle;
    connect_req->data = context;
    handle->data = context;
    context->data = edge_name;

    return uv_tcp_connect(connect_req, handle, context->addr, __net_on_connection);
}

/**
 * Called by the shutdown request. Closes the tcp connection.
 */
void __net_on_close(uv_handle_t* handle)
{
    log_verbose("net_on_close:handle=%p", handle);

    net_tcp_context_t* context = (net_tcp_context_t*) handle->data;
    state_t* state = context->state;
    char* edge_name = context->data;

    free(handle);
    state_run_next(state, edge_name, context);
}

void __net_on_shutdown(uv_shutdown_t* req, int status)
{
    log_verbose("net_on_shutdown:req=%p, status=%d", req, status);
    log_check_uv_r(status, "net_on_disconnect");

    net_tcp_context_t* context = (net_tcp_context_t*) req->data;
    uv_tcp_t* handle = context->handle;

    handle->data = context;
    free(req);
    uv_close((uv_handle_t*) handle, __net_on_close);
}

int net_disconnect(net_tcp_context_t* context, char* edge_name)
{
    log_verbose("net_disconnect:context=%p, edge_name=\"%s\"", context, edge_name);

    uv_shutdown_t* shutdown_req = malloc(sizeof(uv_shutdown_t));

    context->data = edge_name;
    shutdown_req->data = context;

    return uv_shutdown(shutdown_req, (uv_stream_t*) context->handle, __net_on_shutdown);
}

/**
 * Called by net_listen on the event of an incoming connection.
 */
void __net_on_incoming_connection(uv_stream_t* handle, int status)
{
    log_verbose("__net_on_incoming_connection:handle=%p, status=%d", handle, status);
    log_check_uv_r(status, "__net_on_incoming_connection");

    net_tcp_context_t* context = (net_tcp_context_t*) handle->data;
    state_t* state = context->state;
    char* edge_name = context->data;

    state_run_next(state, edge_name, context);
}

/**
 * Starts listening to tcp connections on context->addr. Goes to edge_name on
 * connection. Returns an uv error code if something goes wrong.
 */
int net_listen(net_tcp_context_t* context, char* edge_name)
{
    log_verbose("net_listen:context=%p, edge_name=\"%s\"", context, edge_name);

    int r;
    uv_tcp_t* handle = malloc(sizeof(uv_tcp_t));
    uv_loop_t* loop = context->loop;
    struct sockaddr* addr = context->addr;

    r = uv_tcp_init(loop, handle);

    if (r) {
        return r;
    }
    r = uv_tcp_bind(handle, addr, 0);

    if (r) {
        return r;
    }

    context->data = edge_name;
    context->handle = handle;
    handle->data = context;
    r = uv_listen((uv_stream_t*) handle, 10, __net_on_incoming_connection);

    return r;
}

/**
 * Allocate memory with the suggested size to retrieve incoming data from the
 * tcp client.
 */
void __net_on_alloc(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    log_verbose("net_on_alloc:handle=%p, size=%d, buf=%p", handle, size, buf);

    buf->base = malloc(size);
    buf->len = size;

    if (buf->base == NULL) {
        log_error("reading buffer was not allocated properly");
    }
}

/**
 * Called when a chunk of data has been read. The chunk buffer is pointed to by
 * context->data.
 */
void __net_on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    log_verbose("net_on_read:handle=%p, nread=%d, buf=%p", handle, nread, buf);

    int r;
    net_tcp_context_t* context = (net_tcp_context_t*) handle->data;
    char* edge_name = context->data;

    if (nread < 0) {
        if (nread != UV_EOF) {
            log_check_uv_r(nread, "tcp_on_reading");
        }

        free(buf->base);
    }
    else if (nread > 0) {
        log_debug("net:>>>> \"%s\"", buf->base);

        protocol_value_t* read_payload;

        r = uv_read_stop(handle);
        log_check_uv_r(r, "uv_read_stop");

        r = protocol_parse(&read_payload, buf->base);

        if (r) {
            log_error("could not parse read data (%d)", r);
            context->read_payload = NULL;
        }
        else {
            context->read_payload = read_payload;
        }

        free(buf->base);
        state_run_next(context->state, edge_name, context);
    }
}

/**
 * Read incoming data on the stream handle set in context. Go to the state
 * associated with edge_name when read is finished.
 */
int net_read(net_tcp_context_t* context, char* edge_name)
{
    log_verbose("net_read: context=%p, edge_name=\"%s\"", context, edge_name);

    context->data = edge_name;

    return uv_read_start((uv_stream_t*) context->handle, __net_on_alloc, __net_on_read);
}

void __net_on_write(uv_write_t* req, int status)
{
    log_check_uv_r(status, "net_on_write");

    net_tcp_context_t* context = (net_tcp_context_t*) req->data;
    char* edge_name = context->data;

    free(context->buf);
    state_run_next(context->state, edge_name, context);
}

/**
 * Writes protocol to the tcp handle specified in context. Proceeds to the
 * state associated with edge_name when writing is finished.
 */
int net_write(net_tcp_context_t* context, protocol_value_t* protocol, char* edge_name)
{
    log_verbose("net_write:context=%p, protocol=%p, edge_name=\"%s\"", context, protocol, edge_name);

    uv_write_t* write_req = malloc(sizeof(write_req));
    char pre[1024];
    char* buf = malloc(1024);

    protocol_to_json(protocol, (char*) &pre);
    sprintf(buf, "%s\n", &pre);

    uv_buf_t bufs[] = {
        { .base = buf, .len = strlen(buf) }
    };

    context->data = edge_name;
    context->buf = buf;
    write_req->data = context;

    return uv_write(write_req, (uv_stream_t*) context->handle, bufs, 1, __net_on_write);
}

int net_hostname(net_tcp_context_t* context, char* addr, int* port)
{
    int r;
    struct sockaddr s;
    struct sockaddr_in* sin;
    int addr_len;

    r = uv_tcp_getsockname(context->handle, &s, &addr_len);

    if (r) {
        return r;
    }

    sin = (struct sockaddr_in*) &s;
    strcpy(addr, inet_ntoa(sin->sin_addr));
    *port = (int) htons(sin->sin_port);

    return 0;
}
