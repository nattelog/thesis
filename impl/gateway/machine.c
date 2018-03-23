#include "uv.h"
#include "machine.h"
#include "net.h"
#include "log.h"

void tcp_request_connecting(state_t* state, void* payload)
{
    log_debug("tcp_request_connecting");

    int r = 0;
    net_tcp_context_t* context = net_get_context(state, payload);
    uv_connect_t* connect_req = malloc(sizeof(uv_connect_t));

    context->state = state;
    connect_req->data = context;

    r = uv_tcp_connect(connect_req, context->handle, context->addr, net_on_connection);
    log_check_uv_r(r, "uv_tcp_connect");
}

void tcp_request_writing(state_t* state, void* payload)
{
    log_debug("tcp_request_writing");

    int r = 0;
    net_tcp_context_t* context = net_get_context(state, payload);
    net_request_t* net_payload = context->net_payload;
    char* json_buf = malloc(256);

    r = net_request_to_json(net_payload, json_buf);
    log_check_r(r, "net_request_payload_to_json");

    net_write(context, json_buf, "done");
}

void tcp_request_reading(state_t* state, void* payload)
{
    log_debug("tcp_request_reading");

    net_tcp_context_t* context = net_get_context(state, payload);
    net_read(context, "done");
}

void tcp_request_processing(state_t* state, void* payload)
{
    log_debug("tcp_request_processing");

    int r = 0;
    net_tcp_context_t* context = net_get_context(state, payload);
    char* buf = context->data;
    net_response_t response;

    r = net_parse_response(&response, buf);
    log_check_r(r, "net_parse_response");
}

void tcp_request_closing(state_t* state, void* payload)
{
    log_debug("tcp_request_closing");
}

state_t* machine_tcp_request(state_lookup_t* lookup, state_callback done)
{
    const state_initializer_t si[] = {
        { .name = "tcp_request_connecting", .callback = tcp_request_connecting },
        { .name = "tcp_request_writing", .callback = tcp_request_writing },
        { .name = "tcp_request_reading", .callback = tcp_request_reading },
        { .name = "tcp_request_processing", .callback = tcp_request_processing },
        { .name = "tcp_request_closing", .callback = tcp_request_closing },
        { .name = "tcp_request_done", .callback = done }
    };
    const edge_initializer_t ei[] = {
        { .name = "connect", .from = "tcp_request_connecting", .to = "tcp_request_writing" },
        { .name = "done", .from = "tcp_request_writing", .to = "tcp_request_reading" },
        { .name = "done", .from = "tcp_request_reading", .to = "tcp_request_processing" },
        { .name = "done", .from = "tcp_request_processing", .to = "tcp_request_closing" },
        { .name = "done", .from = "tcp_request_closing", .to = "tcp_request_done" }
    };
    const int nsi = sizeof(si) / sizeof(si[0]);
    const int nei = sizeof(ei) / sizeof(ei[0]);

    return state_machine_build(si, nsi, ei, nei, lookup);
}
