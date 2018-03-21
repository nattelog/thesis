#ifndef __NET_h__
#define __NET_h__

#include <stdlib.h>
#include "uv.h"
#include "json.h"
#include "state.h"

typedef char* (*request_callback)(char* method, char* argv[], size_t argc);

typedef struct tcp_server_context_s tcp_server_context_t;
typedef struct tcp_client_context_s tcp_client_context_t;
typedef struct net_request_s net_request_t;

struct tcp_server_context_s {
    uv_loop_t* loop;
    char* address;
    int port;
    request_callback req_callback;
    state_t* listening_state;
    uv_tcp_t* server_handle;
};

struct tcp_client_context_s {
    state_t* state;
    request_callback req_callback;
    uv_tcp_t* client_handle;
    ssize_t nread;
    uv_buf_t* buf;
};

struct net_request_s {
    int parse_error;
    char* method;
    char** argv;
    size_t argc;
};

state_t* tcp_server_machine(
        uv_loop_t* loop,
        char* address,
        int port,
        request_callback on_request,
        tcp_server_context_t* server_context);

#endif
