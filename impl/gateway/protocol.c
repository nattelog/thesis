#include "log.h"
#include "err.h"
#include "protocol.h"

/**
 * Parses the json string in buf and builds up a json structure in protocol.
 */
int protocol_parse(protocol_value_t** protocol, char* buf)
{
    log_verbose("protocol_parse:protocol=%p, buf=\"%s\"", protocol, buf);

    json_settings settings = {};
    char strerr[128];

    settings.value_extra = json_builder_extra;
    *protocol = json_parse_ex(&settings, buf, strlen(buf), strerr);

    if (*protocol == NULL) {
        log_error("json error:%s", &strerr);
        return EJSON;
    }

    return 0;
}

int protocol_is_object(protocol_value_t* protocol)
{
    log_verbose("protocol_is_object:protocol=%p", protocol);

    return protocol->type == json_object;
}

int protocol_is_array(protocol_value_t* protocol)
{
    log_verbose("protocol_is_array:protocol=%p", protocol);

    return protocol->type == json_array;
}

int protocol_is_string(protocol_value_t* protocol)
{
    log_verbose("protocol_is_string:protocol=%p", protocol);

    return protocol->type == json_string;
}

int protocol_is_bool(protocol_value_t* protocol)
{
    log_verbose("protocol_is_bool:protocol=%p", protocol);

    return protocol->type == json_boolean;
}

int protocol_is_int(protocol_value_t* protocol)
{
    log_verbose("protocol_is_int:protocol=%p", protocol);

    return protocol->type == json_integer;
}

/**
 * Returns 1 if protocol is an object and has a key named key. Returns 0
 * otherwise.
 */
int protocol_has_key(protocol_value_t* protocol, char* key)
{
    log_verbose("protocol_has_key:protocol=%p, key=\"%s\"", protocol, key);

    if (protocol_is_object(protocol)) {
        for (int i = 0; i < protocol->u.object.length; ++i) {
            json_object_entry current_key = protocol->u.object.values[i];

            if (strcmp(current_key.name, key) == 0) {
                return 1;
            }
        }
    }

    return 0;
}

/**
 * Retrieves the value associated with key in protocol and points dest to that
 * value. Note that the value is not copied.
 */
int protocol_get_key(protocol_value_t* protocol, protocol_value_t** dest, char* key)
{
    log_verbose("protocol_get_key:protocol=%p, dest=%p, key=\"%s\"", protocol, dest, key);

    if (protocol_is_object(protocol)) {
        for (int i = 0; i < protocol->u.object.length; ++i) {
            json_object_entry current_key = protocol->u.object.values[i];

            if (strcmp(current_key.name, key) == 0) {
                *dest = current_key.value;
                return 0;
            }
        }
    }

    return ENFND;
}

/**
 * If protocol is an object: returns the number of keys. If an array: returns
 * the number of elemens in the array. If a string: returns the number of
 * characters.
 */
int protocol_get_length(protocol_value_t* protocol)
{
    log_verbose("protocol_get_length:protocol=%p", protocol);

    if (protocol_is_object(protocol)) {
        return protocol->u.object.length;
    }
    else if (protocol_is_array(protocol)) {
        return protocol->u.array.length;
    }
    else if (protocol_is_string(protocol)) {
        return protocol->u.string.length;
    }

    return EPTCL;
}

/**
 * If protocol is an array, points dest to the element at position index in the
 * array. Note that the element is not copied into dest.
 */
int protocol_get_at(protocol_value_t* protocol, protocol_value_t** dest, int index)
{
    log_verbose("protocol_get_at:protocol=%p, dest=%p, index=%d", protocol, dest, index);

    if (protocol_is_array(protocol)) {
        int len = protocol_get_length(protocol);

        if (index > len - 1) {
            return EBNDS;
        }

        *dest = protocol->u.array.values[index];
        return 0;
    }

    return EPTCL;
}

int protocol_get_string(protocol_value_t* protocol, char* buf)
{
    log_verbose("protocol_get_string:protocol=%p, buf=%s", protocol, buf);

    if (protocol_is_string(protocol)) {
        strcpy(buf, protocol->u.string.ptr);
        return 0;
    }

    return EPTCL;
}

int protocol_get_bool(protocol_value_t* protocol)
{
    log_verbose("protocol_get_bool:protocol=%p", protocol);

    if (protocol_is_bool(protocol)) {
        return protocol->u.boolean;
    }

    return EPTCL;
}

int protocol_get_int(protocol_value_t* protocol)
{
    log_verbose("protocol_get_int:protocol=%p", protocol);

    if (protocol_is_int(protocol)) {
        return protocol->u.integer;
    }

    return EPTCL;
}

int protocol_build_string(protocol_value_t** protocol, char* value)
{
    *protocol = json_string_new(value);
    return 0;
}

int protocol_build_int(protocol_value_t** protocol, int value)
{
    *protocol = json_integer_new(value);
    return 0;
}

int protocol_build_array(protocol_value_t** protocol, int argc, ...)
{
    va_list args;

    *protocol = json_array_new(argc);
    va_start(args, argc);

    for (int i = 0; i < argc; ++i) {
        protocol_value_t* arg = va_arg(args, protocol_value_t*);
        json_array_push(*protocol, arg);
    }

    va_end(args);
    return 0;
}

int protocol_build_request(protocol_value_t** protocol, char* method, int argc, ...)
{
    log_verbose("protocol_build_request:protocol=%p, method=\"%s\", args=%d", *protocol, method, argc);

    va_list args;
    json_value* method_val;
    json_value* args_val;

    *protocol = json_object_new(2);
    method_val = json_string_new(method);
    args_val = json_array_new(argc);
    va_start(args, argc);

    for (int i = 0; i < argc; ++i) {
        protocol_value_t* arg = va_arg(args, protocol_value_t*);
        json_array_push(args_val, arg);
    }

    json_object_push(*protocol, "method", method_val);
    json_object_push(*protocol, "args", args_val);
    va_end(args);
    return 0;
}

int protocol_build_response_success(protocol_value_t** protocol, protocol_value_t* result)
{
    log_verbose("protocol_build_response_success:protocol=%p, result=%p", *protocol, result);

    *protocol = json_object_new(1);
    json_object_push(*protocol, "result", result);
    return 0;
}

int protocol_build_response_error(protocol_value_t** protocol, char* name, char* message)
{
    log_verbose("protocol_build_response_error:protocol=%p, name=\"%s\", message=\"%s\"", *protocol, name, message);

    protocol_value_t* error_obj;
    protocol_value_t* name_str;
    protocol_value_t* args_array;
    protocol_value_t* message_str;

    *protocol = json_object_new(1);
    error_obj = json_object_new(2);
    name_str = json_string_new(name);
    args_array = json_array_new(1);
    message_str = json_string_new(message);

    json_object_push(error_obj, "name", name_str);
    json_array_push(args_array, message_str);
    json_object_push(error_obj, "args", args_array);
    json_object_push(*protocol, "error", error_obj);
    return 0;
}

int protocol_to_json(protocol_value_t* protocol, char* buf)
{
    log_verbose("protocol_to_json:protocol=%p, buf=%p", protocol, buf);

    json_serialize_opts opts = {};

    opts.mode = json_serialize_mode_packed;
    json_serialize_ex(buf, protocol, opts);
    return 0;
}

void protocol_free_parse(protocol_value_t* protocol)
{
    log_verbose("protocol_free_parse:protocol=%p", protocol);
    json_value_free(protocol);
}

void protocol_free_build(protocol_value_t* protocol)
{
    log_verbose("protocol_free_build:protocol=%p", protocol);
    json_builder_free(protocol);
}
