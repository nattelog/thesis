#ifndef __MACHINE_h__
#define __MACHINE_h__

#include "state.h"
#include "net.h"
#include "conf.h"
#include "protocol.h"

typedef protocol_value_t* (*request_callback)(protocol_value_t* request);

typedef struct machine_boot_context_s machine_boot_context_t;

struct machine_boot_context_s {
    net_tcp_context_t tcp;
    config_data_t* config;
    protocol_value_t* devices;
};

state_t* machine_tcp_request(state_lookup_t* lookup, state_callback done);

state_t* machine_boot_process();

#endif
