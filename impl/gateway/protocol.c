#include <stdlib.h>
#include "log.h"
#include "err.h"
#include "protocol.h"
#include "machine.h"

/**
 * Parses the json string in buf and builds up a json structure in protocol.
 */
int protocol_parse(protocol_value_t** protocol, char* buf, int len)
{
    log_verbose("protocol_parse:protocol=%p, buf=\"%s\", len=%d", protocol, buf, len);

    json_settings settings = {};
    char strerr[1024] = { 0 };

    settings.value_extra = json_builder_extra;
    *protocol = json_parse_ex(&settings, buf, len, (char*) &strerr);

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

int protocol_build_int(protocol_value_t** protocol, long value)
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

/**
 * Retrieves the error name and the error message found in the errorenous
 * protocol. If protocol does not follow the error standard, return an error
 * code. Returns 0 otherwise.
 */
int protocol_get_response_error(protocol_value_t* protocol, char* error_name, char* error_message)
{
    log_verbose(
            "protocol_get_response_error:protocol=%p, error_name=%p, error_message=%p",
            protocol,
            error_name,
            error_message);

    if (protocol == NULL) {
        strcpy(error_name, "ProtocolError");
        strcpy(error_message, "protocol points to null");
        return 0;
    }

    if (protocol_has_key(protocol, "error")) {
        int r;
        protocol_value_t* error;
        protocol_value_t* name_str;
        protocol_value_t* args_arr;

        r = protocol_get_key(protocol, &error, "error");

        if (r) {
            return r;
        }

        r = protocol_get_key(error, &name_str, "name");

        if (r) {
            return r;
        }

        r = protocol_get_key(error, &args_arr, "args");

        if (r) {
            return r;
        }

        if (protocol_get_length(args_arr) > 0) {
            protocol_value_t* msg_str;

            r = protocol_get_at(args_arr, &msg_str, 0);

            if (r) {
                return r;
            }

            r = protocol_get_string(msg_str, error_message);

            if (r) {
                return r;
            }
        }

        r = protocol_get_string(name_str, error_name);

        if (r) {
            return r;
        }

        return 0;
    }

    return EPTCL;
}

/**
 * Checks whether protocol is an errorenous response or not. Exits if so, does
 * nothing otherwise.
 */
void protocol_check_response_error(protocol_value_t* protocol)
{
    log_verbose("protocol_check_response_error:protocol=%p", protocol);

    int r;
    char err_name[48];
    char err_msg[1024];

    r = protocol_get_response_error(protocol, (char*) &err_name, (char*) &err_msg);

    if (r == 0) {
        log_error("%s:%s", err_name, err_msg);
        exit(1);
    }
}

/**
 * Fills devices_list with devices described as tuples <addr, port> in
 * protocol. Returns an error code if something goes wrong.
 */
int protocol_get_devices(protocol_value_t* protocol, struct sockaddr_storage** devices_list, size_t* devices_len)
{
    log_verbose("protocol_get_devices:protocol=%p, devices=%p", protocol, devices_list);

    int r;
    int len;
    protocol_value_t* devices;

    if (!protocol_has_key(protocol, "result")) {
        return EPTCL;
    }

    r = protocol_get_key(protocol, &devices, "result");

    if (r) {
        return r;
    }

    len = protocol_get_length(devices);

    if (len < 0) {
        return len;
    }

    if (len > MACHINE_MAX_DEVICES) {
        log_error("protocol_get_devices:too many devices!");
        return EBNDS;
    }

    for (int i = 0; i < len; ++i) {
        protocol_value_t* device;
        protocol_value_t* addr;
        protocol_value_t* port;
        char addrstr[128];
        int portint;
        struct sockaddr_storage* saddr;

        r = protocol_get_at(devices, &device, i);

        if (r) {
            return r;
        }

        r = protocol_get_at(device, &addr, 0);

        if (r) {
            return r;
        }

        r = protocol_get_at(device, &port, 1);

        if (r) {
            return r;
        }

        r = protocol_get_string(addr, (char*) &addrstr);

        if (r) {
            return r;
        }

        portint = protocol_get_int(port);

        if (portint < 0) {
            return portint;
        }

        saddr = calloc(1, sizeof(struct sockaddr_storage));
        r = uv_ip4_addr((char*) &addrstr, portint, (struct sockaddr_in*) saddr);

        if (r) {
            return r;
        }

        devices_list[i] = saddr;
    }

    *devices_len = len;

    return 0;
}

size_t protocol_size(protocol_value_t* protocol)
{
    log_verbose("protocol_size:protocol=%p", protocol);

    return json_measure(protocol);
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
