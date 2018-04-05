#ifndef __MACHINE_h__
#define __MACHINE_h__

#include "state.h"
#include "net.h"
#include "conf.h"
#include "protocol.h"
#include "fs.h"

#define MACHINE_MAX_DEVICES 500

typedef protocol_value_t* (*request_callback)(protocol_value_t* request);

typedef struct machine_boot_context_s machine_boot_context_t;
typedef struct machine_server_context_s machine_server_context_t;
typedef struct machine_coop_context_s machine_coop_context_t;

struct machine_boot_context_s {
    net_tcp_context_t tcp;
    config_data_t* config;
    net_tcp_context_t* server_context;
    size_t devices_len;
    struct sockaddr_storage* devices[MACHINE_MAX_DEVICES];
};

struct machine_server_context_s {
    net_tcp_context_t tcp;
    request_callback on_request;
};

struct machine_coop_context_s {
    net_tcp_context_t tcp;
    fs_context_t fs;
    config_data_t* config;
    int req_count;
    long io_count;
    long io_rounds;
    char event[128];
};

state_t* machine_tcp_request(state_lookup_t* lookup, state_callback done);

state_t* machine_boot_process(
        machine_boot_context_t* context,
        uv_loop_t* loop,
        config_data_t* config,
        net_tcp_context_t* server_context);

state_t* machine_tcp_server(
        machine_server_context_t* context,
        uv_loop_t* loop,
        request_callback on_request);

state_t* machine_cooperative_dispatch();

#endif
