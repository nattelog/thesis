#include <stdlib.h>
#include <unistd.h>
#include "log.h"
#include "state.h"
#include "net.h"
#include "config.h"

void usage()
{
    char* usage_str = "    usage: ./gateway [<options>...]\n\n"
        "    OPTIONS\n"
        "        -h\n"
        "            Show this message.\n\n"
        "        -d <architecture>\n"
        "            The architecture of the event dispatcher. Can be one of the following:.\n"
        "                serial\n"
        "                preemptive\n"
        "                cooperative\n\n"
        "        -e <architecture>\n"
        "            The architecture of the event handler. Same alternatives as for the dispatcher.\n\n"
        "        -c <value>\n"
        "            The CPU intensity each event induce. Value between 0 and 1.\n\n"
        "        -i <value>\n"
        "            The I/O intensity each event induce. Value between 0 and 1.\n\n"
        "        -l <address>:<port>\n"
        "            The address and port of the log server.\n\n"
        "        -n <address>:<port>\n"
        "            The address and port of the nameservice.\n\n"
        "";

    printf("%s\n", usage_str);
}

char* on_request(char* method, char** args, size_t argc)
{
    log_info("got request \"%s\"", method);

    return "result";
}

int main(int argc, char** argv)
{
    int r = 0;
    config_data_t config;
    int input_flag;
    uv_loop_t* loop = uv_default_loop();
    tcp_server_context_t server_context;
    state_t* server;

    opterr = 0;
    config_init(&config);

    while ((input_flag = getopt(argc, argv, "hd:e:c:i:l:n:")) != -1) {
        switch (input_flag) {
            case 'h':
                usage();
                return 0;
            case 'd':
                config.dispatcher = optarg;
                break;
            case 'e':
                config.eventhandler = optarg;
                break;
            case 'c':
                config.cpu = atof(optarg);
                break;
            case 'i':
                config.io = atof(optarg);
                break;
            case 'l':
                r = config_parse_address(optarg, &config.logserver_address, &config.logserver_port);
                if (r) {
                    log_error("could not parse logserver address");
                    return 1;
                }
                break;
            case 'n':
                r = config_parse_address(optarg, &config.nameservice_address, &config.nameservice_port);
                if (r) {
                    log_error("could not parse nameserver address");
                    return 1;
                }
                break;
            default:
                break;
        }
    }

    char j_str[1024];
    config_to_json(&config, &j_str);
    printf("%s\n", j_str);

    server = tcp_server_machine(
            loop,
            "0.0.0.0",
            5002,
            on_request,
            &server_context);

    log_init(loop, config.logserver_address, config.logserver_port);

    state_machine_run(server, &server_context);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
