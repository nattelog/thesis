#include <stdarg.h>
#include <string.h>
#include "net.h"
#include "log.h"
#include "err.h"

/**
 * Casts payload and updates the state field.
 */
net_tcp_context_t* net_get_context(state_t* state, void* payload)
{
    net_tcp_context_t* context = (net_tcp_context_t*) payload;
    context->state = state;
    return context;
}

/**
 * Initializes the context. Allocates memory for the tcp handle and the address
 * struct. Returns a uv error code if somethings goes wrong.
 */
int net_tcp_context_init(
        net_tcp_context_t* context,
        uv_loop_t* loop,
        char* address,
        int port)
{
    int r = 0;
    uv_tcp_t* handle = malloc(sizeof(uv_tcp_t));
    struct sockaddr* addr = malloc(sizeof(struct sockaddr));

    r = uv_tcp_init(loop, handle);

    if (r) {
        return r;
    }

    r = uv_ip4_addr(address, port, (struct sockaddr_in*) addr);

    if (r) {
        return r;
    }

    handle->data = context;
    context->addr = addr;
    context->handle = handle;

    return 0;
}

/**
 * Returns the value associated with the "method" key in a json object. Returns
 * NULL if no "method" key was found.
 */
json_value* get_method(json_object_entry* values, unsigned int length)
{
    for (int i = 0; i < length; ++i) {
        if (strcmp(values[i].name, "method") == 0) {
            return values[0].value;
        }
    }

    return NULL;
}

/**
 * Returns the json_value object associated with key.
 */
json_value* net_json_get_key(json_value* value, char* key)
{
    if (value->type == json_object) {
        for (int i = 0; i < value->u.object.length; ++i) {
            json_object_entry current_key = value->u.object.values[i];

            if (strcmp(current_key.name, key) == 0) {
                return current_key.value;
            }
        }
    }

    return NULL;
}

/**
 * Parses the json string in buf and sets the "method" and "args" values in
 * request. Returns an error code if parsing fails.
 */
int net_parse_request(net_request_t* request, char* buf)
{
    json_value* json = json_parse((json_char*) buf, strlen(buf));
    json_value* method_val;
    json_value* args_val;
    char* method_str;

    if (json == NULL) {
        return EJSON;
    }

    if (json->type != json_object || json->u.object.length != 2) {
        json_value_free(json);
        return EJSON;
    }

    method_val = net_json_get_key(json, "method");
    args_val = net_json_get_key(json, "args");

    if (method_val == NULL || args_val == NULL) {
        json_value_free(json);
        return EPTCL;
    }

    if (method_val->type != json_string || args_val->type != json_array) {
        json_value_free(json);
        return EPTCL;
    }

    method_str = malloc(method_val->u.string.length);
    strcpy(method_str, method_val->u.string.ptr);
    request->method = method_str;
    request->argc = 0;

    return 0;
}

/**
 * Parses the json string in buf and depending on whether the response was
 * successful (the "result" key is set) or failed (the "error" key is set), the
 * associated fields are set in the response object. Returns an error code if
 * parsing goes wrong.
 */
int net_parse_response(net_response_t* response, char* buf)
{
    json_value* json = json_parse((json_char*) buf, strlen(buf));
    json_value* result_val;

    if (json == NULL) {
        return EJSON;
    }

    if (json->type != json_object || json->u.object.length != 1) {
        json_value_free(json);
        return EJSON;
    }

    result_val = net_json_get_key(json, "result");

    if (result_val != NULL) {
        char* str;

        if (result_val->type != json_string) {
            return EPTCL;
        }

        str = malloc(result_val->u.string.length);
        strcpy(str, result_val->u.string.ptr);
        json_value_free(json);

        return net_response_success_init(response, str);
    }

    result_val = net_json_get_key(json, "error");

    if (result_val != NULL) {
        json_value* name_val = net_json_get_key(result_val, "name");
        json_value* args_val = net_json_get_key(result_val, "args");
        json_value* msg_val;
        char* name_str;
        char* msg_str;

        if (name_val == NULL || args_val == NULL) {
            json_value_free(json);
            return EPTCL;
        }

        if (name_val->type != json_string || args_val->type != json_array) {
            return EPTCL;
        }

        name_str = malloc(name_val->u.string.length);
        strcpy(name_str, name_val->u.string.ptr);
        response->error_name = name_str;
        response->error_message = NULL;
        response->result = NULL;

        if (args_val->u.array.length == 0) {
            response->error_message = NULL;
        }
        else {
            msg_val = args_val->u.array.values[0];

            if (msg_val->type != json_string) {
                json_value_free(json);
                return EPTCL;
            }

            msg_str = malloc(msg_val->u.string.length);
            strcpy(msg_str, msg_val->u.string.ptr);
            response->error_message = msg_str;
        }

        json_value_free(json);
        return 0;
    }

    json_value_free(json);
    return EPTCL;
}

/**
 * Initializes payload with method, argc and the variable arguments are
 * allocated and copied into argv as char* pointers.
 */
int net_request_init(net_request_t* request, char* method, int argc, ...)
{
    if (argc > MAX_REQUEST_ARGS) {
        return EARGC;
    }

    va_list args;

    request->method = method;
    request->argc = argc;
    va_start(args, argc);

    for (int i = 0; i < argc; ++i) {
        char* arg = va_arg(args, char*);
        size_t argsize = strlen(arg);
        char* buf = malloc(argsize);

        strcpy(buf, arg);
        request->argv[i] = buf;
    }

    va_end(args);

    return 0;
}

/**
 * Initializes a successful response object with the value in result.
 */
int net_response_success_init(net_response_t* response, char* result)
{
    if (response == NULL || result == NULL) {
        return ENULL;
    }

    response->error_name = NULL;
    response->error_message = NULL;
    response->result = result;

    return 0;
}

/**
 * Initializes an errorneous response with the values in name and message.
 */
int net_response_error_init(net_response_t* response, char* name, char* message)
{
    if (response == NULL || name == NULL || message == NULL) {
        return ENULL;
    }

    response->error_name = name;
    response->error_message = message;
    response->result = NULL;

    return 0;
}

/**
 * Converts payload into a json string and copies it into buf.
 */
int net_request_to_json(net_request_t* request, char* buf)
{
    if (request == NULL) {
        return ENULL;
    }

    char* method = request->method;
    char** argv = (char**) request->argv;
    size_t argc = request->argc;

    if (method == NULL || (argc > 0 && argv == NULL)) {
        return ENULL;
    }

    strcpy(buf, "{\"method\":\"");
    strcat(buf, method);
    strcat(buf, "\", \"args\":[\"");

    for (int i = 0; i < argc; ++i) {
        strcat(buf, argv[i]);

        if (i < argc - 1) {
            strcat(buf, "\", \"");
        }
    }

    strcat(buf, "\"]}");

    return 0;
}

/**
 * Creates a json string in an error format and copies it into buf.
 */
void net_response_error_to_json(char* name, char* message, char* buf)
{
    strcpy(buf, "{\"error\":{\"name\":\"");
    strcat(buf, name);
    strcat(buf, "\",\"args\":[\"");
    strcat(buf, message);
    strcat(buf, "\"]}}");
}

/**
 * Creates a json string in a success format and copies it into buf.
 */
void net_response_success_to_json(char* result, char* buf)
{
    strcpy(buf, "{\"result\":");
    strcat(buf, result);
    strcat(buf, "}");
}

int net_response_to_json(net_response_t* response, char* buf)
{
    if (response->result != NULL) {
        net_response_success_to_json(response->result, buf);
        return 0;
    }
    else if (response->error_name != NULL && response->error_message != NULL) {
        net_response_error_to_json(response->error_name, response->error_message, buf);
        return 0;
    }

    return ENULL;
}

/**
 * Run when the tcp handle retrieves a connection request.
 */
void net_on_connection(uv_connect_t* req, int status)
{
    log_check_uv_r(status, "on_connection_cb");

    net_tcp_context_t* context = (net_tcp_context_t*) req->data;
    state_run_next(context->state, "connect", context);
    free(req);
}

/**
 * Called by the shutdown request. Closes the tcp connection.
 */
void net_on_close(uv_handle_t* handle)
{
    free(handle);
    log_debug("closed connection");
}

/**
 * Shutdown the tcp client connection.
 */
void net_on_shutdown(uv_shutdown_t* req, int status)
{
    uv_close((uv_handle_t*) req->handle, net_on_close);
    free(req);
}

/**
 * Allocate memory with the suggested size to retrieve incoming data from the
 * tcp client.
 */
void net_on_alloc(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    buf->base = malloc(size);
    buf->len = size;

    if (buf->base == NULL) {
        log_error("reading buffer was not allocated properly");
    }
}

/**
 * Called when a chunk of data has been read.
 */
void net_on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    int r = 0;
    net_tcp_context_t* context = (net_tcp_context_t*) handle->data;

    if (nread < 0) {
        uv_shutdown_t* shutdown_req = malloc(sizeof(uv_shutdown_t));

        if (nread != UV_EOF) {
            log_check_uv_r(nread, "tcp_on_reading");
        }

        free(context);
        r = uv_shutdown(shutdown_req, handle, net_on_shutdown);
        log_check_uv_r(r, "uv_shutdown");
    }
    else if (nread > 0) {
        log_debug("net:>>>> \"%s\"", buf->base);

        char* edge_name = context->data;

        uv_read_stop(handle);
        context->data = buf->base;
        state_run_next(context->state, edge_name, context);
    }

    free(buf->base);
}

/**
 * Read incoming data on the stream handle set in context. Go to the state
 * associated with edge_name when read is finished.
 */
int net_read(net_tcp_context_t* context, char* edge_name)
{
    context->data = edge_name;

    return uv_read_start((uv_stream_t*) context->handle, net_on_alloc, net_on_read);
}

void net_on_write(uv_write_t* req, int status)
{
    log_check_uv_r(status, "net_on_write");

    net_tcp_context_t* context = (net_tcp_context_t*) req->data;
    char* edge_name = context->data;
    state_run_next(context->state, edge_name, context);
}

/**
 * Write buf to the stream handle set in context. Go to the state associated
 * with edge_name when write is finished.
 */
int net_write(net_tcp_context_t* context, char* buf, char* edge_name)
{
    uv_write_t* write_req = malloc(sizeof(write_req));
    uv_buf_t bufs[] = {
        { .base = buf, .len = strlen(buf) }
    };

    context->data = edge_name;
    write_req->data = context;

    return uv_write(write_req, (uv_stream_t*) context->handle, bufs, 1, net_on_write);
}
