#ifndef __NET_h__
#define __NET_h__

#include <pthread.h>
#include <stdlib.h>
#include "uv.h"
#include "json.h"
#include "state.h"
#include "protocol.h"
#include "conf.h"

#define MAX_REQUEST_ARGS 8
#define SERVER_PORT 5010
#define NET_MAX_SIZE 65536

typedef struct net_tcp_context_s net_tcp_context_t;
typedef struct net_tcp_context_sync_s net_tcp_context_sync_t;
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
    char* buf;
    size_t buf_len;
    char* read_chunk_edge;
    char* read_eof_edge;
};

struct net_tcp_context_sync_s {
    int sock;
    struct sockaddr_storage* addr;
    config_data_t* config;
    protocol_value_t* read_payload;
    protocol_value_t* write_payload;
    char* buf;
    size_t buf_len;
    int is_processed;
    char event[128];
    pthread_mutex_t mutex;
};

int net_tcp_context_init(
        net_tcp_context_t* context,
        uv_loop_t* loop,
        char* address,
        int port);

int net_tcp_context_sync_init(
        net_tcp_context_sync_t* context,
        struct sockaddr_storage* addr,
        config_data_t* config);

net_tcp_context_t* net_get_context(state_t* state, void* payload);

int net_connect(net_tcp_context_t* context, char* edge_name);

int net_connect_sync(net_tcp_context_sync_t* context);

int net_disconnect(net_tcp_context_t* context, char* edge_name);

int net_listen(net_tcp_context_t* context, char* edge_name);

int net_read(net_tcp_context_t* context, char* chunk_edge, char* eof_edge);

int net_read_sync(net_tcp_context_sync_t* context);

int net_write(net_tcp_context_t* context, char* edge_name);

int net_write_sync(net_tcp_context_sync_t* context);

int net_call_sync(net_tcp_context_sync_t* context);

int net_hostname(net_tcp_context_t* context, char* addr, int* port);

#endif
