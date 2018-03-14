#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "uv.h"
#include "state.h"

typedef struct {
    uv_tcp_t* client;
    uv_connect_t* connect_req;
    uv_write_t* write_req;
    uv_shutdown_t* shutdown_req;
    char* data;
} context_t;

typedef struct {
    ssize_t n;
    const uv_buf_t* buf;
} chunk_context_t;

void check_r(int r, char* msg) {
    if (r != 0) {
        log_error("%s: [%s(%d): %s]\n", msg, uv_err_name((r)), (int) r, uv_strerror((r)));
        exit(1);
    }
}

/*
void destroy_context(context_t* context) {
    free(context->client);
    free(context->connect_req);
    free(context->write_req);
    free(context->shutdown_req);
    free(context);
}

void on_close(uv_shutdown_t* req, int status) {
    check_r(status, "on_connect");
}

void alloc_cb(uv_handle_t* handle, size_t n, uv_buf_t* buf) {
    buf->base = malloc(n);
    buf->len = n;
}

void read_cb(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    int r = 0;
    context_t* context = client->data;

    if (nread < 0) {
        if (nread != UV_EOF) {
            check_r(nread, "read_cb");
        }

        free(buf->base);
        context->shutdown_req = malloc(sizeof(uv_shutdown_t));

        r = uv_shutdown(context->shutdown_req, client, on_close);
        check_r(r, "uv_shutdown");
    }

    log_info("read data: %s", buf->base);
}

void on_write(uv_write_t* req, int status) {
    check_r(status, "on_write");

    log_info("wrote data");
}

void on_connect(uv_connect_t* req, int status) {
    check_r(status, "on_connect");

    log_info("Connected %d", status);

    int r = 0;
    context_t* context = req->data;
    context->write_req = malloc(sizeof(uv_write_t));
    context->write_req->data = context;
    context->data = "{\"method\":\"blabla\", \"args\":[]}\n";
    int n = strlen(context->data);

    uv_buf_t bufs[] = {
        { .base = context->data, .len = n }
    };

    r = uv_read_start((uv_stream_t*) context->client, alloc_cb, read_cb);
    check_r(r, "uv_read_start");

    r = uv_write(context->write_req, (uv_stream_t*) context->client, bufs, 1, on_write);
    check_r(r, "uv_write");
}
*/

void on_connection(uv_stream_t* server, int status)
{
    check_r(status, "on_connection");
    state_next((state_t*) server->data, "connect", server);
}

void on_start(state_t* state, void* payload)
{
    log_info("on_start::%s", state->name);

    int r = 0;
    uv_loop_t* loop = uv_default_loop();
    struct sockaddr_in addr;
    uv_tcp_t server;

    r = uv_tcp_init(loop, &server);
    check_r(r, "uv_tcp_init");

    server.data = state;

    r = uv_ip4_addr("0.0.0.0", 5000, &addr);
    check_r(r, "uv_ip4_addr");

    r = uv_tcp_bind(&server, &addr, 0);
    check_r(r, "uv_tcp_bind");

    r = uv_listen((uv_stream_t*) &server, 0, on_connection);
    check_r(r, "uv_listen");

    log_info("listening on 0.0.0.0:5000");

    uv_run(loop, UV_RUN_DEFAULT);
}

void on_close(uv_handle_t* client)
{
    free(client);
    log_info("closed connection");
}

void on_shutdown(uv_shutdown_t* req, int status)
{
    uv_close((uv_handle_t*) req->handle, on_close);
    free(req);
}

void on_alloc(uv_handle_t* client, size_t n, uv_buf_t* buf)
{
    buf->base = malloc(n);
    buf->len = n;
}

void on_chunk(uv_handle_t* client, ssize_t n, const uv_buf_t* buf)
{
    chunk_context_t* context = malloc(sizeof(chunk_context_t));
    context->n = n;
    context->buf = buf;

    state_next((state_t*) client->data, "chunk", context);
}

void on_connecting(state_t* state, void* payload)
{
    log_info("on_connecting::%s", state->name);

    int r = 0;

    log_info("accepting connection");

    uv_tcp_t* server = (uv_tcp_t*) payload;
    uv_tcp_t* client = malloc(sizeof(uv_tcp_t));
    r = uv_tcp_init(server->loop, client);
    check_r(r, "uv_tcp_init");

    r = uv_accept((uv_stream_t*) server, (uv_stream_t*) client);

    if (r) {
        uv_shutdown_t* shutdown_req = malloc(sizeof(uv_shutdown_t));

        log_error("trying to accept connection %d", r);

        r = uv_shutdown(shutdown_req, (uv_stream_t*) client, on_shutdown);
        check_r(r, "uv_shutdown");
    } else {
        client->data = state;
        r = uv_read_start((uv_stream_t*) client, on_alloc, on_chunk);
        check_r(r, "uv_read_start");
    }
}

void on_read(state_t* state, void* payload)
{
    log_info("on_read::%s", state->name);

    chunk_context_t* context = (chunk_context_t*) payload;
    ssize_t n = context->n;
    const uv_buf_t* buf = context->buf;

    if (n < 0) {
        if (n != UV_EOF) {
            check_r(n, "on_read");
        }
    }

    log_info("%s", buf->base);
}

void on_processing(state_t* state, void* payload)
{
    log_info("on_processing::%s", state->name);
}

void on_writing(state_t* state, void* payload)
{
    log_info("on_writing::%s", state->name);
}

int main() {
    /*
    int r = 0;
    uv_loop_t* loop = uv_default_loop();
    struct sockaddr_in addr;
    context_t* context = malloc(sizeof(context_t));

    context->client = malloc(sizeof(uv_tcp_t));
    context->client->data = context;
    context->connect_req = malloc(sizeof(uv_connect_t));
    context->connect_req->data = context;

    r = uv_tcp_init(loop, context->client);
    check_r(r, "uv_tcp_init");

    r = uv_ip4_addr("0.0.0.0", 5000, &addr);
    check_r(r, "uv_ip4_addr");

    r = uv_tcp_connect(context->connect_req, context->client, &addr, on_connect);
    uv_run(loop, UV_RUN_DEFAULT);
    */

    const state_initializer_t si[] = {
        { .name = "start", .callback = on_start },
        { .name = "connecting", .callback = on_connecting },
        { .name = "reading", .callback = on_read },
        { .name = "processing", .callback = on_processing },
        { .name = "writing", .callback = on_writing }
    };
    const edge_initializer_t ei[] = {
        { .name = "connect", .from = "start", .to = "connecting" },
        { .name = "chunk", .from = "connecting", .to = "reading" },
        { .name = "chunk", .from = "reading", .to = "reading" },
        { .name = "done", .from = "reading", .to = "processing" },
        { .name = "done", .from = "processing", .to = "writing" }
    };
    const int nsi = sizeof(si) / sizeof(si[0]);
    const int nei = sizeof(ei) / sizeof(ei[0]);

    state_t* state = state_machine_build(si, nsi, ei, nei);
    state_print(state);

    state_machine_run(state, NULL);

    return 0;
}
