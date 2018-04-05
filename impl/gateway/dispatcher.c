#include "uv.h"
#include "log.h"
#include "dispatcher.h"
#include "machine.h"
#include "state.h"
#include "event_handler.h"

/**
 * The serial dispatcher only process one device and one event at a time.
 */
void dispatcher_serial(config_data_t* config, protocol_value_t* devices)
{
    log_verbose("dispatcher_serial:devices=%p", devices);

    int i = 0;
    int r;
    protocol_value_t* status_request;
    protocol_value_t* get_event_request;
    struct sockaddr_storage* devices_addrs[MACHINE_MAX_DEVICES];
    size_t devices_len;
    net_tcp_context_sync_t* devices_context[MACHINE_MAX_DEVICES];

    r = protocol_get_devices(devices, devices_addrs, &devices_len);
    log_check_r(r, "protocol_get_devices");

    for (int j = 0; j < devices_len; ++j) {
        net_tcp_context_sync_t* context = malloc(sizeof(net_tcp_context_sync_t));
        struct sockaddr_storage* addr = devices_addrs[j];

        r = net_tcp_context_sync_init(context, addr);
        log_check_r(r, "net_tcp_context_sync_init");
        devices_context[j] = context;
    }

    r = protocol_build_request(&status_request, "status", 0);
    log_check_r(r, "protocol_build_request");

    r = protocol_build_request(&get_event_request, "next_event", 0);
    log_check_r(r, "protocol_build_request");

    while (1) {
        net_tcp_context_sync_t* device = devices_context[i];
        protocol_value_t* response;
        protocol_value_t* result;
        int status_ok;

        device->write_payload = status_request;
        r = net_call_sync(device);
        log_check_uv_r(r, "net_call_sync");

        response = device->read_payload;
        protocol_check_response_error(response);
        r = protocol_get_key(response, &result, "result");
        log_check_r(r, "protocol_get_key");

        status_ok = protocol_get_int(result);
        protocol_free_parse(device->read_payload);

        if (status_ok) {
            char event_id[128];

            device->write_payload = get_event_request;
            r = net_call_sync(device);
            log_check_uv_r(r, "net_call_sync");

            response = device->read_payload;
            protocol_check_response_error(response);
            r = protocol_get_key(response, &result, "result");
            log_check_r(r, "protocol_get_key");

            r = protocol_get_string(result, (char*) &event_id);
            log_check_r(r, "protocol_get_string");

            event_handler_serial(config->cpu, config->io);

            log_event_retrieved((char*) event_id);
            log_event_dispatched((char*) event_id);
            log_event_done((char*) event_id);
        }

        i = (i + 1) % devices_len;
    }

    protocol_free_build(status_request);
    protocol_free_build(get_event_request);
}

/**
 * The cooperative dispatcher utilizes the I/O-wait-time that occurs when a tcp
 * package is being transfered to process other devices and events.
 */
void dispatcher_cooperative(config_data_t* config, protocol_value_t* devices)
{
    log_verbose("dispatcher_cooperative:devices=%p", devices);

    int r;
    uv_loop_t* loop = uv_default_loop();
    state_t* coop_dispatch = machine_cooperative_dispatch();
    int devices_length = protocol_get_length(devices);

    if (devices_length < 0) {
        log_check_r(devices_length, "protocol_get_length");
    }

    for (int i = 0; i < devices_length; ++i) {
        machine_coop_context_t* context = malloc(sizeof(machine_coop_context_t));
        char addr[128];
        int port;

        r = protocol_get_device(devices, i, (char*) &addr, &port);
        log_check_r(r, "protocol_get_device");

        r = net_tcp_context_init((net_tcp_context_t*) context, loop, (char*) addr, port);
        log_check_uv_r(r, "net_tcp_context_init");

        context->req_count = 0;
        context->config = config;

        state_machine_run(coop_dispatch, context);
    }

    uv_run(loop, UV_RUN_DEFAULT);
}
