#ifndef __CONF_h__
#define __CONF_h__

#include "protocol.h"

typedef struct config_data_s config_data_t;

struct config_data_s {
    int tp_size;
    char* dispatcher;
    char* eventhandler;
    double cpu;
    double io;
    char test_manager_address[128];
    int logserver_port;
    int nameservice_port;
};

void config_init(config_data_t* config);

int config_to_protocol_type(config_data_t* config, protocol_value_t** protocol);

int config_parse_address(char* addr_string, char* target_addr);

#endif
