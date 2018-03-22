#ifndef __CONFIG_h__
#define __CONFIG_h__

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

void config_to_json(config_data_t* config, char* buf);

int config_parse_address(char* addr_string, char* target_addr, int* target_port);

#endif
