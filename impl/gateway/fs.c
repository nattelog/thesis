#include "fs.h"
#include "log.h"

void __fs_on_close(uv_fs_t* req)
{
    log_verbose("__fs_on_close:req=%p", req);

    if (req->result < 0) {
        log_check_uv_r(req->result, "__fs_on_close");
    }

    fs_context_t* context = (fs_context_t*) req->data;

    uv_fs_req_cleanup(req);
    state_run_next(context->state, context->next_edge, context);
}

void __fs_on_write(uv_fs_t* req)
{
    log_verbose("__fs_on_write:req=%p", req);

    if (req->result < 0) {
        log_check_uv_r(req->result, "__fs_on_write");
    }

    int r;
    fs_context_t* context = (fs_context_t*) req->data;
    int fd = context->fd;

    r = uv_fs_close(req->loop, req, fd, __fs_on_close);
    log_check_uv_r(r, "__fs_on_write:uv_fs_close");
}

void __fs_on_open(uv_fs_t* req)
{
    log_verbose("__fs_on_open:req=%p", req);

    if (req->result < 0) {
        log_check_uv_r(req->result, "__fs_on_open");
    }

    int r;
    fs_context_t* context = (fs_context_t*) req->data;
    uv_buf_t bufs[] = {
        { .base = context->content, .len = strlen(context->content) }
    };

    context->fd = req->result;

    r = uv_fs_write(req->loop, req, context->fd, bufs, 1, 0, __fs_on_write);
    log_check_uv_r(r, "__fs_on_open:uv_fs_write");
}

/**
 * Appends context->content to file context->path and closes it. The state with
 * edge edge_name is run after close.
 */
int fs_append(fs_context_t* context, char* edge_name)
{
    log_verbose("fs_append:context=%p, edge_name=\"%s\"", context, edge_name);

    uv_loop_t* loop = context->loop;
    char* path = context->path;
    uv_fs_t* req = malloc(sizeof(uv_fs_t));

    context->next_edge = edge_name;
    req->data = context;

    return uv_fs_open(loop, req, path, UV_FS_O_WRONLY | UV_FS_O_CREAT, 0644, __fs_on_open);
}
