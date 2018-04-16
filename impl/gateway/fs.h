#ifndef __FS_h__
#define __FS_h__

#include "uv.h"
#include "state.h"

#define FS_MAX_BUF 128

typedef struct fs_context_s fs_context_t;

struct fs_context_s {
    state_t* state;
    uv_loop_t* loop;
    int fd;
    char path[FS_MAX_BUF];
    char* content;
    char* next_edge;
    void* data;
};

int fs_append(fs_context_t* context, char* edge_name);

#endif
