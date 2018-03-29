#ifndef __CONF_h__
#define __CONF_h__

#include "protocol.h"

typedef struct config_data_s config_data_t;

struct config_data_s {
    char* dispatcher;
    char* eventhandler;
    double cpu;
    double io;
    char logserver_address[48];
    int logserver_port;
    char nameservice_address[48];
    int nameservice_port;
};

void config_init(config_data_t* config);

int config_to_protocol_type(config_data_t* config, protocol_value_t** protocol);

int config_parse_address(char* addr_string, char* target_addr, int* target_port);

#endif
