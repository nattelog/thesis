#include <string.h>
#include "net.h"
#include "log.h"

json_value* get_method(json_object_entry* values, unsigned int length)
{
    for (int i = 0; i < length; ++i) {
        if (strcmp(values[i].name, "method") == 0) {
            return values[0].value;
        }
    }

    return NULL;
}

/**
 * Validates the json_value* object and puts the "method" and "args" fields in
 * a net_request_t container for further use.
 */
void net_parse_request(json_value* value, net_request_t* request)
{
    if (value == NULL) {
        request->method = "could not parse json";
    }
    else if (value->type == json_object && value->u.object.length == 2) {
        json_value* method_value = get_method(value->u.object.values, value->u.object.length);

        if (method_value != NULL && method_value->type == json_string) {
            request->parse_error = 0;
            request->method = (char*) method_value->u.string.ptr;
            request->argv = NULL; // no support for args
            request->argc = 0;
            return;
        }
    }
    else {
        request->method = "corrupt json message: must be an object with \"method\" and \"args\" field";
    }

    request->parse_error = 1;
}

void net_response_error(char* name, char* message, char* buf)
{
    strcpy(buf, "{\"error\":{\"name\":\"");
    strcat(buf, name);
    strcat(buf, "\",\"args\":[\"");
    strcat(buf, message);
    strcat(buf, "\"]}}");
}

void net_response_success(char* result, char* buf)
{
    strcpy(buf, "{\"result\":");
    strcat(buf, result);
    strcat(buf, "}");
}

void on_connection_cb(uv_stream_t* server_handle, int status)
{
    log_check_uv_r(status, "on_connection_cb");

    tcp_server_context_t* context = (tcp_server_context_t*) server_handle->data;
    state_t* state = context->listening_state;

    state_run_next(state, "connect", context);
}

void tcp_on_listening(state_t* state, void* payload)
{
    log_debug("tcp_on_listening");

    int r = 0;
    tcp_server_context_t* context = (tcp_server_context_t*) payload;
    uv_tcp_t* server_handle = malloc(sizeof(uv_tcp_t));

    r = uv_tcp_init(context->loop, server_handle);
    log_check_uv_r(r, "uv_tcp_init");

    context->listening_state = state;
    context->server_handle = server_handle;
    server_handle->data = context;

    struct sockaddr_in addr;
    r = uv_ip4_addr(context->address, context->port, &addr);

    r = uv_tcp_bind(server_handle, &addr, 0);
    log_check_uv_r(r, "uv_tcp_bind");

    r = uv_listen((uv_stream_t*) server_handle, 0, on_connection_cb);
    log_check_uv_r(r, "uv_listen");

    // print the address of the server handle
    struct sockaddr s;
    struct sockaddr_in* sin;
    int addr_len;

    r = uv_tcp_getsockname(server_handle, &s, &addr_len);
    log_check_uv_r(r, "uv_tcp_getsockname");

    sin = (struct sockaddr_in*) &s;
    char* straddr = inet_ntoa(sin->sin_addr);
    unsigned short intport = htons(sin->sin_port);

    log_info("tcp server listening on %s:%d", straddr, intport);
}

void on_close_cb(uv_handle_t* client_handle)
{
    free(client_handle);
    log_debug("closed connection");
}

void on_shutdown_cb(uv_shutdown_t* req, int status)
{
    uv_close((uv_handle_t*) req->handle, on_close_cb);
    free(req);
}

void on_alloc_cb(uv_handle_t* client_handle, size_t size, uv_buf_t* buf)
{
    buf->base = malloc(size);
    buf->len = size;

    if (buf->base == NULL) {
        log_error("reading buffer was not allocated properly");
    }
}

void on_read_cb(uv_stream_t* client_handle, ssize_t nread, const uv_buf_t* buf)
{
    tcp_client_context_t* context = (tcp_client_context_t*) client_handle->data;
    context->nread = nread;
    context->buf = buf;

    state_run_next(context->state, "accept", context);
}

void tcp_on_connecting(state_t* state, void* payload)
{
    log_debug("tcp_on_connecting");

    tcp_server_context_t* server_context = (tcp_server_context_t*) payload;
    uv_loop_t* loop = server_context->loop;
    uv_tcp_t* server_handle = server_context->server_handle;
    int r = 0;
    uv_tcp_t* client_handle = malloc(sizeof(uv_tcp_t));

    r = uv_tcp_init(loop, client_handle);
    log_check_uv_r(r, "uv_tcp_init");

    r = uv_accept((uv_stream_t*) server_handle, (uv_stream_t*) client_handle);

    if (r) {
        uv_shutdown_t* shutdown_req = malloc(sizeof(uv_shutdown_t));

        log_error("trying to accept connection %d", r);
        r = uv_shutdown(shutdown_req, (uv_stream_t*) client_handle, on_shutdown_cb);
        log_check_uv_r(r, "uv_shutdown");
    } else {
        tcp_client_context_t* client_context = malloc(sizeof(tcp_client_context_t));

        client_context->state = state;
        client_context->req_callback = server_context->req_callback;
        client_context->client_handle = client_handle;
        client_handle->data = client_context;

        r = uv_read_start((uv_stream_t*) client_handle, on_alloc_cb, on_read_cb);
        log_check_uv_r(r, "uv_read_start");
    }
}

void on_write_cb(uv_write_t* req, int status)
{
    log_check_uv_r(status, "on_write_cb");
}

void tcp_on_reading(state_t* state, void* payload)
{
    log_debug("tcp_on_reading");

    tcp_client_context_t* context = (tcp_client_context_t*) payload;
    uv_tcp_t* client_handle = context->client_handle;
    ssize_t nread = context->nread;
    uv_buf_t* buf = context->buf;
    int r = 0;

    if (nread < 0) {
        uv_shutdown_t* shutdown_req = malloc(sizeof(uv_shutdown_t));

        if (nread != UV_EOF) {
            log_check_uv_r(nread, "tcp_on_reading");
        }

        free(context);
        r = uv_shutdown(shutdown_req, (uv_stream_t*) client_handle, on_shutdown_cb);
        log_check_uv_r(r, "uv_shutdown");
    }
    else if (nread > 0) {
        log_info(">>>> \"%s\"", buf->base);

        char result[512];
        json_value* j_val = json_parse(buf->base, nread);
        uv_write_t* write_req = malloc(sizeof(uv_write_t));
        request_callback req_callback = context->req_callback;
        net_request_t tcp_req = { .parse_error = 0, .method = NULL, .argv = NULL, .argc = 0 };

        net_parse_request(j_val, &tcp_req);

        if (tcp_req.parse_error == 1) {
            net_response_error("ParseError", tcp_req.method, &result);
        }
        else {
            char* str_result = (*req_callback)(tcp_req.method, tcp_req.argv, tcp_req.argc);
            net_response_success(str_result, &result);
        }

        if (j_val != NULL) {
            json_value_free(j_val);
        }

        log_info("<<<< \"%s\"", result);
        strcat(result, "\n");

        uv_buf_t write_buf[] = {
            { .base = result, .len = strlen(result) }
        };

        r = uv_write(write_req, (uv_stream_t*) client_handle, write_buf, 1, on_write_cb);
        log_check_uv_r(r, "uv_write");
    }

    free(buf->base);
}

state_t* tcp_server_machine(
        uv_loop_t* loop,
        char* address,
        int port,
        request_callback req_callback,
        tcp_server_context_t* context)
{
    const state_initializer_t si[] = {
        { .name = "tcp_server_listening", .callback = tcp_on_listening },
        { .name = "tcp_server_connecting", .callback = tcp_on_connecting },
        { .name = "tcp_server_reading", .callback = tcp_on_reading },
    };
    const edge_initializer_t ei[] = {
        { .name = "connect", .from = "tcp_server_listening", .to = "tcp_server_connecting" },
        { .name = "accept", .from = "tcp_server_connecting", .to = "tcp_server_reading" },
    };
    const int nsi = sizeof(si) / sizeof(si[0]);
    const int nei = sizeof(ei) / sizeof(ei[0]);

    state_lookup_t lookup;
    lookup_init(&lookup);
    state_t* state = state_machine_build(si, nsi, ei, nei, &lookup);
    lookup_clear(&lookup);

    context->loop = loop;
    context->address = address;
    context->port = port;
    context->req_callback = req_callback;
    context->listening_state = NULL;

    return state;
}
