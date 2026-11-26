#include "server.h"
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

int main(int argc, char **argv)
{
    int port = ONLINE_DEFAULT_PORT;
    if (argc > 1) {
        char *end = NULL;
        long value = strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || value < 1 || value > 65535) {
            fprintf(stderr, "usage: %s [port]\n", argv[0]);
            return 1;
        }
        port = (int)value;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "shogun-server: WSAStartup failed\n");
        return 1;
    }
#endif

    if (!online_token()) {
        fprintf(stderr, "shogun-server: SHOGUN_TOKEN is not set\n");
        return 1;
    }

    printf("SHOGUN matchmaking server on port %d\n", port);
    fflush(stdout);
    if (!online_server_run(port)) {
        fprintf(stderr, "shogun-server: could not listen on port %d\n", port);
        return 1;
    }
    return 0;
}
