#include <stdlib.h>
#include <unistd.h>
#include "log.h"
#include "state.h"
#include "net.h"
#include "conf.h"
#include "machine.h"
#include "dispatcher.h"
#include "event_handler.h"

static machine_server_context_t server_context;
static machine_boot_context_t boot_context;

void usage()
{
    char* usage_str = "    usage: ./gateway [<options>...]\n\n"
        "    OPTIONS\n"
        "        -h\n"
        "            Show this message.\n\n"
        "        -d <architecture>\n"
        "            The architecture of the event dispatcher. Can be one of the following:.\n"
        "                serial\n"
        "                preemptive\n"
        "                cooperative\n\n"
        "        -e <architecture>\n"
        "            The architecture of the event handler. Same alternatives as for the dispatcher.\n\n"
        "        -c <value>\n"
        "            The CPU intensity each event induce. Value between 0 and 1.\n\n"
        "        -i <value>\n"
        "            The I/O intensity each event induce. Value between 0 and 1.\n\n"
        "        -p <value>\n"
        "            The size of the thread pool. Defaults to 10.\n\n"
        "        -n <address>:<port>\n"
        "            The address and port of the nameservice.\n\n"
        "        -l <address>:<port>\n"
        "            The address and port of the log server.\n\n"
        "";

    printf("%s\n", usage_str);
}

void on_handle_close(uv_handle_t* handle)
{
    log_verbose("on_handle_close:handle=%p", handle);
}

void close_boot_process(uv_idle_t* handle)
{
    log_verbose("close_boot_process:handle=%p", handle);

    int r;
    uv_tcp_t* server_handle = server_context.tcp.handle;

    uv_close((uv_handle_t*) server_handle, on_handle_close);
    r = uv_idle_stop(handle);
    log_check_uv_r(r, "uv_idle_stop");
}

protocol_value_t* on_request(protocol_value_t* request)
{
    log_verbose("on_request:request=%p", request);

    int r;
    protocol_value_t* response = NULL;

    if (protocol_has_key(request, "method") && protocol_has_key(request, "args")) {
        protocol_value_t* method;
        protocol_value_t* result;
        char method_str[NET_MAX_SIZE] = { 0 };

        r = protocol_get_key(request, &method, "method");
        log_check_r(r, "protocol_get_key");

        r = protocol_get_string(method, (char*) &method_str);
        log_check_r(r, "protocol_get_string");

        if (strcmp(method_str, "get_timestamp") == 0) {
            r = protocol_build_int(&result, get_timestamp());
            log_check_r(r, "protocol_build_int");

            r = protocol_build_response_success(&response, result);
            log_check_r(r, "protocol_build_response_success");
        }
        else if (strcmp(method_str, "start_test") == 0) {
            uv_idle_t* handle = malloc(sizeof(uv_idle_t));
            uv_loop_t* loop = uv_default_loop();

            r = protocol_build_int(&result, 0);
            log_check_r(r, "protocol_build_int");

            r = uv_idle_init(loop, handle);
            log_check_uv_r(r, "uv_idle_init");

            r = uv_idle_start(handle, close_boot_process);
            log_check_uv_r(r, "uv_idle_start");

            r = protocol_build_response_success(&response, result);
            log_check_r(r, "protocol_build_response_success");
        }
        else {
            r = protocol_build_response_error(&response, "Error", "Unknown method");
            log_check_r(r, "protocol_build_response_error");
        }
    }
    else {
        r = protocol_build_response_error(&response, "Error", "Bad request protocol");
        log_check_r(r, "protocol_build_response_error");
    }

    return response;
}

void prepare_test(config_data_t* config, protocol_value_t** devices)
{
    log_verbose("prepare_test:config=%p, devices=%p", config, devices);

    int r;
    uv_loop_t* loop = uv_default_loop();
    state_t* boot_process;
    state_t* server;

    r = log_init(loop, config->logserver_address, config->logserver_port);
    log_check_uv_r(r, "log_init");

    boot_process = machine_boot_process(&boot_context, loop, config, (net_tcp_context_t*) &server_context);
    server = machine_tcp_server(&server_context, loop, on_request);

    state_machine_run(server, &server_context);
    state_machine_run(boot_process, &boot_context);
    uv_run(loop, UV_RUN_DEFAULT);

    // delete states

    r = protocol_get_key(boot_context.tcp.read_payload, devices, "result");
    log_check_r(r, "protocol_get_key");
}

void start_test(config_data_t* config, protocol_value_t* devices)
{
    log_verbose("start_test:config=%p, devices=%p", config, devices);

    char* dispatcher_type = config->dispatcher;

    if (strcmp(config->eventhandler, "preemptive") == 0) {
        char* pool_size[5];

        sprintf((char*) &pool_size, "%d", config->tp_size);
        setenv("UV_THREADPOOL_SIZE", (char*) pool_size, 1);
        event_handler_preemptive_init(config);
    }

    if (strcmp(dispatcher_type, "serial") == 0) {
        dispatcher_serial(config, devices);
    }
    else if (strcmp(dispatcher_type, "cooperative") == 0) {
        dispatcher_cooperative(config, devices);
    }
}

int main(int argc, char** argv)
{
    int r = 0;
    config_data_t config;
    int input_flag;
    protocol_value_t* devices;

    opterr = 0;
    config_init(&config);

    if (argc == 1) {
        usage();
        return 0;
    }

    while ((input_flag = getopt(argc, argv, "hd:e:c:i:p:l:n:")) != -1) {
        switch (input_flag) {
            case 'h':
                usage();
                return 0;
            case 'd':
                config.dispatcher = optarg;
                break;
            case 'e':
                config.eventhandler = optarg;
                break;
            case 'c':
                config.cpu = atof(optarg);
                break;
            case 'i':
                config.io = atof(optarg);
                break;
            case 'p':
                config.tp_size = atoi(optarg);
                break;
            case 'l':
                r = config_parse_address(optarg, (char*) &config.logserver_address, &config.logserver_port);
                if (r) {
                    log_error("could not parse logserver address");
                    return 1;
                }
                break;
            case 'n':
                r = config_parse_address(optarg, (char*) &config.nameservice_address, &config.nameservice_port);
                if (r) {
                    log_error("could not parse nameserver address");
                    return 1;
                }
                break;
            default:
                break;
        }
    }

    prepare_test(&config, &devices);
    start_test(&config, devices);

    return 0;
}
