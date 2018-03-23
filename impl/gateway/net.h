#ifndef __NET_h__
#define __NET_h__

#include <stdlib.h>
#include "uv.h"
#include "json.h"
#include "state.h"

#define MAX_PAYLOAD_ARGS 8

typedef char* (*request_callback)(char* method, char** argv, size_t argc);

typedef struct net_tcp_context_s net_tcp_context_t;
typedef struct net_request_s net_request_t;
typedef struct net_response_s net_response_t;

struct net_tcp_context_s {
    state_t* state;
    uv_tcp_t* handle;
    struct sockaddr* addr;
    void* net_payload; // should either be a request or response
    void* data;
};

struct net_request_s {
    int parse_error;
    char* method;
    char* argv[MAX_PAYLOAD_ARGS];
    size_t argc;
};

struct net_response_s {
    char* result;
    char* error_name;
    char* error_message;
};

int net_tcp_context_init(
        net_tcp_context_t* context,
        uv_loop_t* loop,
        char* address,
        int port);

net_tcp_context_t* net_get_context(state_t* state, void* payload);

int net_parse_response(net_response_t* response, char* buf);

int net_request_init(net_request_t* payload, char* method, int argc, ...);

int net_request_to_json(net_request_t* payload, char* buf);

void net_on_connection(uv_connect_t* req, int status);

int net_read(net_tcp_context_t* context, char* edge_name);

int net_write(net_tcp_context_t* context, char* buf, char* edge_name);

#endif
