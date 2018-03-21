#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "state.h"
#include "net.h"

char* on_request(char* method, char** args, size_t argc)
{
    log_info("got request \"%s\"", method);

    return "result";
}

int main()
{
    uv_loop_t* loop = uv_default_loop();
    tcp_server_context_t server_context;
    state_t* server = tcp_server_machine(
            loop,
            "0.0.0.0",
            5000,
            on_request,
            &server_context);

    log_debug("state in main is");
    state_print(server);

    state_machine_run(server, &server_context);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
