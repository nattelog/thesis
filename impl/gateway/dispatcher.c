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
    log_verbose("dispatcher_serial:config=%p, devices=%p", config, devices);

    int i = 0;
    int r;
    size_t devices_len;
    net_tcp_context_sync_t* devices_context[MACHINE_MAX_DEVICES];
    char* ts_addr = (char*) config->test_manager_address;
    protocol_value_t* status_request;
    protocol_value_t* get_event_request;

    r = protocol_build_request(&status_request, "status", 0);
    log_check_r(r, "dispatcher_serial:protocol_build_request");

    r = protocol_build_request(&get_event_request, "next_event", 0);
    log_check_r(r, "dispatcher_serial:protocol_build_request");

    devices_len = protocol_get_length(devices);

    for (size_t j = 0; j < devices_len; ++j) {
        protocol_value_t* port_value;
        int device_port;
        net_tcp_context_sync_t* context = malloc(sizeof(net_tcp_context_sync_t));

        r = protocol_get_at(devices, &port_value, j);
        log_check_r(r, "dispatcher_serial:protocol_get_at");

        device_port = protocol_get_int(port_value);
        if (device_port < 0) {
            log_check_r(device_port, "dispatcher_serial:protocol_get_int");
        }

        r = net_tcp_context_sync_init(context, ts_addr, device_port, config);
        log_check_r(r, "net_tcp_context_sync_init");

        devices_context[j] = context;
    }

    while (1) {
        net_tcp_context_sync_t* device = devices_context[i];
        protocol_value_t* response;
        protocol_value_t* result;
        int status_ok;

        i = (i + 1) % devices_len;

        pthread_mutex_lock(&device->mutex);
        if (!device->is_processed) {
            pthread_mutex_unlock(&device->mutex);
            continue;
        }
        pthread_mutex_unlock(&device->mutex);

        device->write_payload = status_request;
        r = net_call_sync(device);
        log_check_uv_r(r, "dispatcher_serial:net_call_sync");

        response = device->read_payload;
        protocol_check_response_error(response);
        r = protocol_get_key(response, &result, "result");
        log_check_r(r, "dispatcher_serial:protocol_get_key");

        status_ok = protocol_get_int(result);
        protocol_free_parse(device->read_payload);

        if (status_ok) {
            device->write_payload = get_event_request;
            r = net_call_sync(device);
            log_check_uv_r(r, "dispatcher_serial:net_call_sync");

            response = device->read_payload;
            protocol_check_response_error(response);
            r = protocol_get_key(response, &result, "result");
            log_check_r(r, "dispatcher_serial:protocol_get_key");

            r = protocol_get_string(result, (char*) &device->event);
            log_check_r(r, "dispatcher_serial:protocol_get_string");

            log_event_retrieved((char*) device->event);
            log_event_dispatched((char*) device->event);

            if (strcmp(config->eventhandler, "serial") == 0) {
                event_handler_serial(config->cpu, config->io);
                log_event_done((char*) device->event);
            }
            else if (strcmp(config->eventhandler, "preemptive") == 0) {
                pthread_mutex_lock(&event_handler_lock);
                while (event_handler_queue_size > EVENT_HANDLER_MAX_QUEUE_SIZE) {
                    log_debug("dispatcher_serial:waiting for free thread");
                    pthread_cond_wait(&event_handler_cond, &event_handler_lock);
                }
                event_handler_preemptive(device);
                pthread_mutex_unlock(&event_handler_lock);
            }
            else {
                log_error("dispatcher_serial:no support for eventhandler \"%s\"", config->eventhandler);
                exit(1);
            }

        }
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
    log_verbose("dispatcher_cooperative:config=%p, devices=%p", config, devices);

    int r;
    uv_loop_t* loop = uv_default_loop();
    state_t* coop_dispatch = machine_cooperative_dispatch();
    char* ts_addr = (char*) config->test_manager_address;
    int devices_length = protocol_get_length(devices);

    if (devices_length < 0) {
        log_check_r(devices_length, "protocol_get_length");
    }

    for (int i = 0; i < devices_length; ++i) {
        machine_coop_context_t* context = malloc(sizeof(machine_coop_context_t));
        protocol_value_t* port_value;
        int device_port;

        r = protocol_get_at(devices, &port_value, i);
        log_check_r(r, "dispatcher_cooperative:protocol_get_at");

        device_port = protocol_get_int(port_value);
        if (device_port < 0) {
            log_check_r(device_port, "dispatcher_cooperative:protocol_get_int");
        }

        r = net_tcp_context_init((net_tcp_context_t*) context, loop, ts_addr, device_port);
        log_check_uv_r(r, "dispatcher_cooperative:net_tcp_context_init");

        context->req_count = 0;
        context->config = config;

        state_machine_run(coop_dispatch, context);
    }

    uv_run(loop, UV_RUN_DEFAULT);
}
