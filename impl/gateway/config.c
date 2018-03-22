#include <stdlib.h>
#include <regex.h>
#include "config.h"
#include "log.h"

/**
 * Initializes the config object with default values.
 */
void config_init(config_data_t* config)
{
    config->dispatcher = NULL;
    config->eventhandler = NULL;
    config->cpu = 0;
    config->io = 0;
    config->logserver_port = 0;
    config->nameservice_port = 0;
}

/**
 * Writes the config object to a json format and puts it in buf.
 */
void config_to_json(config_data_t* config, char* buf)
{
    char pre[128];

    strcpy(buf, "{\"DISPATCHER\":\"");

    if (config->dispatcher == NULL) {
        strcat(buf, "");
    }
    else {
        strcat(buf, config->dispatcher);
    }

    strcat(buf, "\", \"EVENT_HANDLER\":\"");

    if (config->eventhandler == NULL) {
        strcat(buf, "");
    }
    else {
        strcat(buf, config->eventhandler);
    }

    strcat(buf, "\", \"CPU_INTENSITY\":");
    sprintf(&pre, "%f", config->cpu);
    strcat(buf, &pre);

    strcat(buf, ", \"IO_INTENSITY\":");
    sprintf(&pre, "%f", config->io);
    strcat(buf, &pre);

    strcat(buf, ", \"LOGSERVER_ADDRESS\":\"");
    sprintf(&pre, "%s:%d", config->logserver_address, config->logserver_port);
    strcat(buf, &pre);

    strcat(buf, ", \"NAMESERVICE_ADDRESS\":\"");
    sprintf(&pre, "%s:%d", config->nameservice_address, config->nameservice_port);
    strcat(buf, &pre);

    strcat(buf, "\"}");
}

/**
 * Parses an address string on the format <address>:<port> and sets them in the
 * target parameters. Returns 1 if parsing was unsuccessful, 0 otherwise.
 */
int config_parse_address(char* addr_string, char* target_addr, int* target_port)
{
    regex_t regex;
    int r = 0;
    int nmatch = 3;
    regmatch_t matches[nmatch];
    char* str = "^([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+):([0-9]+)$";

    r = regcomp(&regex, str, REG_EXTENDED);

    if (r) {
        log_error("could not compile regex");
        return 1;
    }

    r = regexec(&regex, addr_string, nmatch, &matches, 0);

    if (r) {
        return 1;
    }
    else {
        regmatch_t addr_match = matches[1];
        regmatch_t port_match = matches[2];
        int addr_len = addr_match.rm_eo - addr_match.rm_so;
        int port_len = port_match.rm_eo - port_match.rm_so;
        char* port_str[24];

        memcpy(target_addr, addr_string, addr_len);
        memcpy(port_str, &addr_string[port_match.rm_so], port_len);
        *target_port = atoi(port_str);

        return 0;
    }
}
