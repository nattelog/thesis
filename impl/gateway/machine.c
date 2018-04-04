#include "uv.h"
#include "machine.h"
#include "net.h"
#include "log.h"
#include "err.h"

void __tcp_request_connecting(state_t* state, void* payload)
{
    log_verbose("__tcp_request_connecting:state=%p, payload=%p", state, payload);

    int r;
    net_tcp_context_t* context = net_get_context(state, payload);

    r = net_connect(context, "connect"); log_check_uv_r(r, "req_connect");
    log_check_uv_r(r, "net_connect");
}

void __tcp_request_writing(state_t* state, void* payload)
{
    log_verbose("__tcp_request_writing:state=%p, payload=%p", state, payload);

    int r;
    net_tcp_context_t* context = net_get_context(state, payload);

    r = net_write(context, "done");
    log_check_uv_r(r, "net_write");
}

void __tcp_request_reading(state_t* state, void* payload)
{
    log_verbose("__tcp_request_reading:state=%p, payload=%p", state, payload);

    net_tcp_context_t* context = net_get_context(state, payload);
    net_read(context, "done", NULL);
}

void __tcp_request_closing(state_t* state, void* payload)
{
    log_verbose("__tcp_request_closing:state=%p, payload=%p", state, payload);

    int r = 0;
    net_tcp_context_t* context = net_get_context(state, payload);

    r = net_disconnect(context, "done");
    log_check_uv_r(r, "net_disconnect");
}

state_t* machine_tcp_request(state_lookup_t* lookup, state_callback done)
{
    log_verbose("__machine_tcp_request:lookup=%p, done=%p", lookup, done);

    const state_initializer_t si[] = {
        { .name = "tcp_request_connecting", .callback = __tcp_request_connecting },
        { .name = "tcp_request_writing", .callback = __tcp_request_writing },
        { .name = "tcp_request_reading", .callback = __tcp_request_reading },
        { .name = "tcp_request_closing", .callback = __tcp_request_closing },
        { .name = "tcp_request_done", .callback = done }
    };
    const edge_initializer_t ei[] = {
        { .name = "connect", .from = "tcp_request_connecting", .to = "tcp_request_writing" },
        { .name = "done", .from = "tcp_request_writing", .to = "tcp_request_reading" },
        { .name = "done", .from = "tcp_request_reading", .to = "tcp_request_closing" },
        { .name = "done", .from = "tcp_request_closing", .to = "tcp_request_done" }
    };
    const int nsi = sizeof(si) / sizeof(si[0]);
    const int nei = sizeof(ei) / sizeof(ei[0]);

    return state_machine_build(si, nsi, ei, nei, lookup);
}

void __boot_process_verify_config(state_t* state, void* payload) {
    log_verbose("__boot_process_verify_config:state=%p, payload=%p", state, payload);

    int r;
    char straddr[128];
    int port;
    protocol_value_t* request;
    protocol_value_t* config_val;
    protocol_value_t* addr_val;
    protocol_value_t* port_val;
    protocol_value_t* tuple_val;
    machine_boot_context_t* context = (machine_boot_context_t*) payload;
    net_tcp_context_t* server_context = context->server_context;

    r = config_to_protocol_type(context->config, &config_val);
    log_check_r(r, "config_to_protocol_type");

    // build tuple with gateway server address
    net_hostname(server_context, (char*) &straddr, &port);
    protocol_build_string(&addr_val, straddr);
    protocol_build_int(&port_val, port);
    protocol_build_array(&tuple_val, 2, addr_val, port_val);

    r = protocol_build_request(&request, "verify_gateway", 2, config_val, tuple_val);
    log_check_r(r, "protocol_build_request");

    ((net_tcp_context_t*) context)->write_payload = request;
    state_run_next(state, "start", context);
}

void __boot_process_check_verification(state_t* state, void* payload) {
    log_verbose("__boot_process_check_verification:state=%p, payload=%p", state, payload);

    int r;
    net_tcp_context_t* context = (net_tcp_context_t*) payload;
    protocol_value_t* response = context->read_payload;

    protocol_free_build(context->write_payload);
    protocol_check_response_error(response);

    if (protocol_has_key(response, "result")) {
        protocol_value_t* result_obj;
        int result;

        r = protocol_get_key(response, &result_obj, "result");
        log_check_r(r, "protocol_get_key");

        result = protocol_get_bool(result_obj);

        if (result < 0) {
            log_check_r(result, "protocol_get_bool");
        }

        if (result == 1) {
            log_info("config verification ok!");
            protocol_free_parse(response);
            state_run_next(state, "done", context);
        }
        else {
            log_error("config verification not ok!");
            exit(1);
        }
    }
    else {
        log_check_r(EPTCL, "boot_process_check_verification");
    }
}

void __boot_process_get_devices(state_t* state, void* payload) {
    log_verbose("__boot_process_get_devices:state=%p, payload=%p", state, payload);

    int r;
    net_tcp_context_t* context = (net_tcp_context_t*) payload;
    protocol_value_t* request;

    r = protocol_build_request(&request, "hostnames", 0);
    log_check_r(r, "protocol_build_request");

    context->write_payload = request;
    state_run_next(state, "start", context);
}

void __boot_process_done(state_t* state, void* payload) {
    log_verbose("__boot_process_done:state=%p, payload=%p", state, payload);

    int r;
    machine_boot_context_t* context = (machine_boot_context_t*) payload;
    protocol_value_t* response = ((net_tcp_context_t*) context)->read_payload;
    protocol_value_t* request = ((net_tcp_context_t*) context)->write_payload;

    protocol_check_response_error(response);
    r = protocol_get_devices(response, context->devices, &context->devices_len);
    log_check_r(r, "protocol_get_devices");

    protocol_free_build(response);
    protocol_free_build(request);
}

void __boot_process_tcp_done(state_t* state, void* payload) {
    log_verbose("__boot_process_tcp_done:state=%p, payload=%p", state, payload);

    machine_boot_context_t* context = (machine_boot_context_t*) payload;
    static int count = 0;

    if (count++ == 0) {
        state_run_next(state, "verification_response", context);
    } else {
        state_run_next(state, "devices_response", context);
    }
}

/**
 * Create a boot state machine that initiates the gateway with its surrounding
 * test systems.
 */
state_t* machine_boot_process(
        machine_boot_context_t* context,
        uv_loop_t* loop,
        config_data_t* config,
        net_tcp_context_t* server_context)
{
    log_verbose(
            "machine_boot_process:context=%p, loop=%p, config=%p, server_context=%p",
            context,
            loop,
            config,
            server_context);

    int r;
    state_t* boot_process;
    state_lookup_t lookup;
    char* nameservice_address;
    int nameservice_port;

    const state_initializer_t si[] = {
        { .name = "boot_process_verify_config", .callback = __boot_process_verify_config },
        { .name = "boot_process_check_verification", .callback = __boot_process_check_verification },
        { .name = "boot_process_get_devices", .callback = __boot_process_get_devices },
        { .name = "boot_process_done", .callback = __boot_process_done }
    };
    const edge_initializer_t ei[] = {
        { .name = "start", .from = "boot_process_verify_config", .to = "tcp_request_connecting" },
        { .name = "verification_response", .from = "tcp_request_done", .to = "boot_process_check_verification" },
        { .name = "done", .from = "boot_process_check_verification", .to = "boot_process_get_devices" },
        { .name = "start", .from = "boot_process_get_devices", .to = "tcp_request_connecting" },
        { .name = "devices_response", .from = "tcp_request_done", .to = "boot_process_done" }
    };
    const int nsi = sizeof(si) / sizeof(si[0]);
    const int nei = sizeof(ei) / sizeof(ei[0]);

    lookup_init(&lookup);
    machine_tcp_request(&lookup, __boot_process_tcp_done);
    boot_process = state_machine_build(si, nsi, ei, nei, &lookup);
    lookup_clear(&lookup);

    nameservice_address = (char*) config->nameservice_address;
    nameservice_port = config->nameservice_port;

    r = net_tcp_context_init((net_tcp_context_t*) context, loop, nameservice_address, nameservice_port);
    log_check_uv_r(r, "net_tcp_context_init");

    context->config = config;
    context->server_context = server_context;
    ((net_tcp_context_t*) context)->buf = malloc(NET_MAX_SIZE);

    return boot_process;
}

void __server_listening(state_t* state, void* payload)
{
    log_verbose("__server_listening:state=%p, payload=%p", state, payload);

    int r;
    net_tcp_context_t* context = net_get_context(state, payload);

    r = net_listen(context, "connect");
    log_check_uv_r(r, "net_listen");
}

void __server_connecting(state_t* state, void* payload)
{
    log_verbose("__server_connecting:state=%p, payload=%p", state, payload);

    int r;
    machine_server_context_t* server_context = (machine_server_context_t*) payload;
    machine_server_context_t* client_context = calloc(1, sizeof(machine_server_context_t));
    uv_tcp_t* server_handle = ((net_tcp_context_t*) server_context)->handle;
    uv_loop_t* loop = ((net_tcp_context_t*) server_context)->loop;
    uv_tcp_t* client_handle = calloc(1, sizeof(uv_tcp_t));

    r = uv_tcp_init(loop, client_handle);
    log_check_uv_r(r, "uv_tcp_init");
    r = uv_accept((uv_stream_t*) server_handle, (uv_stream_t*) client_handle);

    if (r) {
        log_error("could not accept connection (%d)", r);
        r = net_disconnect((net_tcp_context_t*) client_context, "disconnect");
        log_check_uv_r(r, "net_disconnect");
    }

    ((net_tcp_context_t*) client_context)->handle = client_handle;
    ((net_tcp_context_t*) client_context)->state = state;
    ((net_tcp_context_t*) client_context)->buf_len = NET_MAX_SIZE;
    ((net_tcp_context_t*) client_context)->buf = calloc(1, NET_MAX_SIZE);

    client_context->on_request = server_context->on_request;
    client_handle->data = client_context;
    r = net_read((net_tcp_context_t*) client_context, "process", "disconnect");
    log_check_uv_r(r, "net_read");
}

void __server_processing(state_t* state, void* payload)
{
    log_verbose("__server_processing:state=%p, payload=%p", state, payload);

    int r;
    machine_server_context_t* context = (machine_server_context_t*) payload;
    request_callback on_request = context->on_request;
    protocol_value_t* request = ((net_tcp_context_t*) context)->read_payload;
    protocol_value_t* response;

    ((net_tcp_context_t*) context)->state = state;

    if (request == NULL) {
       r = protocol_build_response_error(&response, "ProtocolError", "Could not parse response");
       log_check_r(r, "protocol_build_response_error");
    }
    else {
        response = (*on_request)(request);
    }

    protocol_free_parse(request);
    ((net_tcp_context_t*) context)->write_payload = response;
    net_write((net_tcp_context_t*) context, "done");
}

void __server_write_done(state_t* state, void* payload)
{
    log_verbose("__server_write_done:state=%p, payload=%p", state, payload);

    int r;
    net_tcp_context_t* context = net_get_context(state, payload);
    state_t* connecting_state = NULL;

    if (context->write_payload != NULL) {
        protocol_free_build(context->write_payload);
    }

    r = state_next(context->state, &connecting_state, "read");
    log_check_r(r, "state_next");

    // reset state without running the callback
    context->state = connecting_state;
}

void __server_disconnecting(state_t* state, void* payload)
{
    log_verbose("__server_disconnecting:state=%p, payload=%p", state, payload);

    int r;
    net_tcp_context_t* context = net_get_context(state, payload);

    r = net_disconnect(context, "clean");
    log_check_uv_r(r, "net_disconnect");
}

void __server_cleaning(state_t* state, void* payload)
{
    log_verbose("__server_cleaning:state=%p, payload=%p", state, payload);

    net_tcp_context_t* context = net_get_context(state, payload);

    free(context);
}

/**
 * Creates a tcp server state machine that listens for tcp connections, reads
 * their data and responds according to on_request.
 */
state_t* machine_tcp_server(
        machine_server_context_t* context,
        uv_loop_t* loop,
        request_callback on_request)
{
    log_verbose("machine_tcp_server:context=%p, loop=%p, on_request=%p", context, loop, on_request);

    int r;
    state_t* server;
    state_lookup_t lookup;

    const state_initializer_t si[] = {
        { .name = "server_listening", .callback = __server_listening },
        { .name = "server_connecting", .callback = __server_connecting },
        { .name = "server_processing", .callback = __server_processing },
        { .name = "server_write_done", .callback = __server_write_done },
        { .name = "server_disconnecting", .callback = __server_disconnecting },
        { .name = "server_cleaning", .callback = __server_cleaning },
    };
    const edge_initializer_t ei[] = {
        { .name = "connect", .from = "server_listening", .to = "server_connecting" },
        { .name = "process", .from = "server_connecting", .to = "server_processing" },
        { .name = "disconnect", .from = "server_connecting", .to = "server_disconnecting" },
        { .name = "done", .from = "server_processing", .to = "server_write_done" },
        { .name = "read", .from = "server_write_done", .to = "server_connecting" },
        { .name = "clean", .from = "server_disconnecting", .to = "server_cleaning" }
    };
    const int nsi = sizeof(si) / sizeof(si[0]);
    const int nei = sizeof(ei) / sizeof(ei[0]);

    lookup_init(&lookup);
    server = state_machine_build(si, nsi, ei, nei, &lookup);
    lookup_clear(&lookup);

    r = net_tcp_context_init((net_tcp_context_t*) context, loop, "0.0.0.0", SERVER_PORT);
    log_check_uv_r(r, "net_tcp_context_init");

    context->on_request = on_request;

    return server;
}
