#ifndef __PROTOCOL_h__
#define __PROTOCOL_h__

#include "json.h"
#include "json-builder.h"

typedef json_value protocol_value_t;

int protocol_parse(protocol_value_t** protocol, char* buf, int len);

int protocol_is_object(protocol_value_t* protocol);

int protocol_is_array(protocol_value_t* protocol);

int protocol_is_string(protocol_value_t* protocol);

int protocol_is_bool(protocol_value_t* protocol);

int protocol_is_int(protocol_value_t* protocol);

int protocol_has_key(protocol_value_t* protocol, char* key);

int protocol_get_key(protocol_value_t* protocol, protocol_value_t** dest, char* key);

int protocol_get_length(protocol_value_t* protocol);

int protocol_get_at(protocol_value_t* protocol, protocol_value_t** dest, int index);

int protocol_get_string(protocol_value_t* protocol, char* buf);

int protocol_get_bool(protocol_value_t* protocol);

int protocol_get_int(protocol_value_t* protocol);

int protocol_build_string(protocol_value_t** protocol, char* value);

int protocol_build_int(protocol_value_t** protocol, long value);

int protocol_build_array(protocol_value_t** protocol, int argc, ...);

int protocol_build_request(protocol_value_t** protocol, char* method, int argc, ...);

int protocol_build_response_success(protocol_value_t** protocol, protocol_value_t* result);

int protocol_build_response_error(protocol_value_t** protocol, char* name, char* message);

int protocol_get_response_error(protocol_value_t* protocol, char* error_name, char* error_message);

void protocol_check_response_error(protocol_value_t* protocol);

int protocol_get_devices(protocol_value_t* protocol, struct sockaddr_storage** devices_list, size_t* devices_len);

size_t protocol_size(protocol_value_t* protocol);

int protocol_to_json(protocol_value_t* protocol, char* buf);

void protocol_free_parse(protocol_value_t* protocol);

void protocol_free_build(protocol_value_t* protocol);

#endif
