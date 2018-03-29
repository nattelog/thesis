#ifndef __NET_h__
#define __NET_h__

#include <stdlib.h>
#include "uv.h"
#include "json.h"
#include "state.h"
#include "protocol.h"

#define MAX_REQUEST_ARGS 8

typedef struct net_tcp_context_s net_tcp_context_t;
typedef struct net_request_s net_request_t;
typedef struct net_response_s net_response_t;

struct net_tcp_context_s {
    state_t* state;
    uv_loop_t* loop;
    uv_tcp_t* handle;
    struct sockaddr* addr;
    protocol_value_t* read_payload;
    protocol_value_t* write_payload;
    void* data;
};

int net_tcp_context_init(
        net_tcp_context_t* context,
        uv_loop_t* loop,
        char* address,
        int port);

net_tcp_context_t* net_get_context(state_t* state, void* payload);

int net_connect(net_tcp_context_t* context, char* edge_name);

int net_disconnect(net_tcp_context_t* context, char* edge_name);

int net_listen(net_tcp_context_t* context, char* edge_name);

int net_read(net_tcp_context_t* context, char* edge_name);

int net_write(net_tcp_context_t* context, char* buf, char* edge_name);

int net_hostname(net_tcp_context_t* context, char* addr, int* port);

#endif
