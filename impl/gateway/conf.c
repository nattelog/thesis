#include <stdlib.h>
#include <regex.h>
#include "conf.h"
#include "json.h"
#include "json-builder.h"
#include "log.h"
#include "err.h"

/**
 * Initializes the config object with default values.
 */
void config_init(config_data_t* config)
{
    config->tp_size = 10;
    config->dispatcher = NULL;
    config->eventhandler = NULL;
    config->cpu = 0;
    config->io = 0;
    config->logserver_port = 0;
    config->nameservice_port = 0;
}

/**
 * Converts the config object to a protocol type object.
 */
int config_to_protocol_type(config_data_t* config, protocol_value_t** protocol)
{
    log_verbose("config_to_protocol_type:config=%p, protocol=%p", config, *protocol);

    char pre[128];

    if (config->dispatcher == NULL || config->eventhandler == NULL) {
        return ENULL;
    }

    *protocol = json_object_new(0);
    json_object_push(*protocol, "DISPATCHER", json_string_new(config->dispatcher));
    json_object_push(*protocol, "EVENT_HANDLER", json_string_new(config->eventhandler));
    json_object_push(*protocol, "CPU_INTENSITY", json_double_new(config->cpu));
    json_object_push(*protocol, "IO_INTENSITY", json_double_new(config->io));
    json_object_push(*protocol, "POOL_SIZE", json_integer_new(config->tp_size));

    /*
    sprintf((char*) &pre, "%s:%d", config->nameservice_address, config->nameservice_port);
    json_object_push(*protocol, "NAMESERVICE_ADDRESS", json_string_new((char*) &pre));

    sprintf((char*) &pre, "%s:%d", config->logserver_address, config->logserver_port);
    json_object_push(*protocol, "LOGSERVER_ADDRESS", json_string_new((char*) &pre));
    */

    return 0;
}

/**
 * Parses an address string on the format x.x.x.x and sets them in the
 * target parameters. Returns 1 if parsing was unsuccessful, 0 otherwise.
 */
int config_parse_address(char* addr_string, char* target_addr)
{
    regex_t regex;
    int r = 0;
    int nmatch = 2;
    regmatch_t matches[nmatch];
    char* str = "^([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)$";

    r = regcomp(&regex, str, REG_EXTENDED);

    if (r) {
        log_error("could not compile regex");
        return 1;
    }

    r = regexec(&regex, addr_string, nmatch, (regmatch_t*) &matches, 0);

    if (r) {
        return 1;
    }
    else {
        regmatch_t addr_match = matches[1];
        int addr_len = addr_match.rm_eo - addr_match.rm_so;

        memcpy(target_addr, addr_string, addr_len);
        target_addr[addr_len] = (char) 0;

        return 0;
    }
}
