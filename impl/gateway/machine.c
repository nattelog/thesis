#include "uv.h"
#include "machine.h"
#include "net.h"
#include "log.h"
#include "err.h"

void tcp_request_connecting(state_t* state, void* payload)
{
    log_debug("tcp_request_connecting");

    int r;
    net_tcp_context_t* context = net_get_context(state, payload);

    r = net_connect(context, "connect");
    log_check_uv_r(r, "req_connect");
}

void tcp_request_writing(state_t* state, void* payload)
{
    log_debug("tcp_request_writing");

    int r = 0;
    net_tcp_context_t* context = net_get_context(state, payload);
    protocol_value_t* request = context->request;
    char* json_buf = malloc(256);

    r = protocol_to_json(request, json_buf);
    log_check_r(r, "protocol_to_json");

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
    protocol_value_t* response;

    r = protocol_parse(&response, buf);
    log_check_r(r, "protocol_parse");

    free(buf);
    context->response = response;
    state_run_next(state, "done", context);
}

void tcp_request_closing(state_t* state, void* payload)
{
    log_debug("tcp_request_closing");

    int r = 0;
    net_tcp_context_t* context = net_get_context(state, payload);

    r = net_disconnect(context, "done");
    log_check_uv_r(r, "net_disconnect");
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

void boot_process_verify_config(state_t* state, void* payload) {
    log_debug("boot_process_verify_config");

    int r;
    //char straddr[32];
    //char strport[32];
    //int port;
    protocol_value_t* request;
    protocol_value_t* config_val;
    machine_boot_context_t* context = (machine_boot_context_t*) payload;

    r = config_to_protocol_type(context->config, &config_val);
    log_check_r(r, "config_to_protocol_type");

    /*
    net_hostname((net_tcp_context_t*) context, (char*) &straddr, &port);
    sprintf(strport, "%d", port);
    */

    r = protocol_build_request(&request, "verify_gateway", 1, config_val); //, &straddr, &strport);
    log_check_r(r, "protocol_build_request");

    ((net_tcp_context_t*) context)->request = request;
    state_run_next(state, "start", context);
}

void boot_process_check_verification(state_t* state, void* payload) {
    log_debug("boot_process_check_verification");

    int r;
    net_tcp_context_t* context = (net_tcp_context_t*) payload;
    protocol_value_t* response = context->response;

    protocol_free_parse(context->request);
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

void boot_process_get_devices(state_t* state, void* payload) {
    log_debug("boot_process_get_devices");

    int r;
    net_tcp_context_t* context = (net_tcp_context_t*) payload;
    protocol_value_t* request;

    r = protocol_build_request(&request, "hostnames", 0);
    log_check_r(r, "protocol_build_request");

    context->request = request;
    state_run_next(state, "start", context);
}

void boot_process_done(state_t* state, void* payload) {
    log_debug("boot_process_done");

    int r;
    net_tcp_context_t* context = (net_tcp_context_t*) payload;
    protocol_value_t* response = context->response;

    protocol_check_response_error(response);
}

void boot_process_tcp_done(state_t* state, void* payload) {
    log_debug("boot_process_tcp_done");

    machine_boot_context_t* context = (machine_boot_context_t*) payload;

    if (context->request_count == 0) {
        context->request_count++;
        state_run_next(state, "verification_response", context);
    } else {
        state_run_next(state, "devices_response", context);
    }
}

state_t* machine_boot_process()
{
    state_t* boot_process;
    state_lookup_t lookup;
    const state_initializer_t si[] = {
        { .name = "boot_process_verify_config", .callback = boot_process_verify_config },
        { .name = "boot_process_check_verification", .callback = boot_process_check_verification },
        { .name = "boot_process_get_devices", .callback = boot_process_get_devices },
        { .name = "boot_process_done", .callback = boot_process_done }
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
    machine_tcp_request(&lookup, boot_process_tcp_done);
    boot_process = state_machine_build(si, nsi, ei, nei, &lookup);
    lookup_clear(&lookup);

    return boot_process;
}
