#include "server.h"

#include "protocol.h"
#include "socket_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#define MAX_CLIENTS 8
#define IDLE_SECONDS 2
#define AUTH_SECONDS 10

typedef enum {
    CLIENT_AUTHENTICATING = 0,
    CLIENT_WAITING,
    CLIENT_CHALLENGING,
    CLIENT_PLAYING
} ClientState;

typedef struct {
    int fd;
    char name[ONLINE_NAME_SIZE];
    ClientState state;
    int match_id;
    int opponent;
    int avoid;
    int rematch;
    time_t ready_time;
    time_t auth_deadline;
    char expected_hash[ONLINE_HASH_HEX_SIZE];
    int authenticated;
    LineBuffer input;
} Client;

typedef struct {
    Client clients[MAX_CLIENTS];
    int listener;
    int discovery;
    int next_match;
    uint32_t seed;
    uint64_t nonce_seed;
    int port;
} Server;

static void server_init(Server *server)
{
    memset(server, 0, sizeof(*server));
    server->listener = -1;
    server->discovery = -1;
    server->next_match = 1;
    server->seed = (uint32_t)time(NULL) ^ 0x5eed1234u;
    server->nonce_seed = (uint64_t)time(NULL) ^ 0x9e3779b97f4a7c15ULL;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        server->clients[i].fd = -1;
        server->clients[i].opponent = -1;
        server->clients[i].avoid = -1;
        line_buffer_init(&server->clients[i].input);
    }
}

static void server_done(Server *server)
{
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (server->clients[i].fd >= 0) closesocket(server->clients[i].fd);
        line_buffer_done(&server->clients[i].input);
    }
    if (server->discovery >= 0) closesocket(server->discovery);
    if (server->listener >= 0) closesocket(server->listener);
}

static uint32_t next_seed(Server *server)
{
    server->seed = server->seed * 1664525u + 1013904223u;
    return server->seed;
}

static uint64_t next_nonce(Server *server, const struct sockaddr_in *peer)
{
    server->nonce_seed = server->nonce_seed * 6364136223846793005ULL +
                         1442695040888963407ULL;
    uint64_t value = server->nonce_seed ^ (uint64_t)time(NULL);
    if (peer) {
        value ^= (uint64_t)peer->sin_addr.s_addr;
        value ^= (uint64_t)ntohs(peer->sin_port) << 16;
    }
    return value;
}

static int valid_client(const Server *server, int index)
{
    return index >= 0 && index < MAX_CLIENTS && server->clients[index].fd >= 0;
}

static void send_cancel(Server *server, int index, const char *reason)
{
    Client *client = &server->clients[index];
    if (!valid_client(server, index)) return;
    int other = client->opponent;
    (void)socket_send_line(client->fd, "CANCELLED %d %s", client->match_id,
                           reason);
    if (valid_client(server, other)) {
        (void)socket_send_line(server->clients[other].fd,
                               "CANCELLED %d %s", client->match_id, reason);
    }
}

static void make_waiting(Server *server, int index, int with_opponent)
{
    Client *client = &server->clients[index];
    if (!valid_client(server, index)) return;
    client->state = CLIENT_WAITING;
    client->opponent = -1;
    client->rematch = 0;
    client->avoid = with_opponent ? -1 : client->avoid;
    client->ready_time = time(NULL) + IDLE_SECONDS;
}

static void cancel_pair(Server *server, int index, const char *reason)
{
    if (!valid_client(server, index)) return;
    int other = server->clients[index].opponent;
    send_cancel(server, index, reason);
    make_waiting(server, index, 0);
    if (valid_client(server, other)) {
        server->clients[other].avoid = index;
        make_waiting(server, other, 0);
        server->clients[other].avoid = index;
    }
}

static void drop_client(Server *server, int index)
{
    if (!valid_client(server, index)) return;
    int other = server->clients[index].opponent;
    Client *client = &server->clients[index];
    if ((client->state == CLIENT_CHALLENGING ||
         client->state == CLIENT_PLAYING) && valid_client(server, other)) {
        (void)socket_send_line(server->clients[other].fd,
                               "CANCELLED %d LEFT", client->match_id);
        server->clients[other].avoid = index;
        make_waiting(server, other, 0);
        server->clients[other].avoid = index;
    }
    closesocket(client->fd);
    client->fd = -1;
    line_buffer_done(&client->input);
}

static void send_challenge(Server *server, int first, int second)
{
    Client *a = &server->clients[first];
    Client *b = &server->clients[second];
    a->match_id = server->next_match++;
    b->match_id = a->match_id;
    a->opponent = second;
    b->opponent = first;
    a->avoid = -1;
    b->avoid = -1;
    a->rematch = 0;
    b->rematch = 0;
    a->state = CLIENT_CHALLENGING;
    b->state = CLIENT_CHALLENGING;
    (void)socket_send_line(a->fd, "CHALLENGE %d %s", a->match_id, b->name);
    (void)socket_send_line(b->fd, "CHALLENGE %d %s", b->match_id, a->name);
}

static void begin_game(Server *server, int first, int second)
{
    Client *a = &server->clients[first];
    Client *b = &server->clients[second];
    uint32_t seed = next_seed(server);
    a->state = CLIENT_PLAYING;
    b->state = CLIENT_PLAYING;
    a->rematch = 0;
    b->rematch = 0;
    (void)socket_send_line(a->fd, "START %d %lu %d", a->match_id,
                           (unsigned long)seed, SIDE_WHITE);
    (void)socket_send_line(b->fd, "START %d %lu %d", b->match_id,
                           (unsigned long)seed, SIDE_RED);
}

static void try_pair(Server *server)
{
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!valid_client(server, i) ||
            server->clients[i].state != CLIENT_WAITING ||
            now < server->clients[i].ready_time) continue;
        for (int j = i + 1; j < MAX_CLIENTS; ++j) {
            if (!valid_client(server, j) ||
                server->clients[j].state != CLIENT_WAITING ||
                now < server->clients[j].ready_time ||
                server->clients[i].avoid == j ||
                server->clients[j].avoid == i) continue;
            send_challenge(server, i, j);
            return;
        }
    }
}

static void handle_auth(Server *server, int index, const char *line)
{
    if (!valid_client(server, index)) return;
    Client *client = &server->clients[index];
    if (client->authenticated) return;
    char hash[ONLINE_HASH_HEX_SIZE];
    if (!online_parse_auth(line, hash, sizeof(hash)) ||
        strcmp(hash, client->expected_hash) != 0) {
        (void)socket_send_line(client->fd, "ERROR BAD PASSWORD");
        drop_client(server, index);
        return;
    }
    client->authenticated = 1;
}

static void handle_hello(Server *server, int index, const char *line)
{
    if (!valid_client(server, index)) return;
    Client *client = &server->clients[index];
    const char *name = line + strlen("HELLO");
    online_sanitize_name(name, client->name, sizeof(client->name));
    if (!client->name[0]) snprintf(client->name, sizeof(client->name), "PLAYER");
    client->state = CLIENT_WAITING;
    client->opponent = -1;
    client->rematch = 0;
    client->ready_time = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (valid_client(server, i) && server->clients[i].state == CLIENT_WAITING) {
            server->clients[i].avoid = -1;
        }
    }
    (void)socket_send_line(client->fd, "WELCOME %s", client->name);
    (void)socket_send_line(client->fd, "WAITING");
    try_pair(server);
}

static void handle_response(Server *server, int index, int match_id, int accept,
                            int from_play)
{
    if (!valid_client(server, index)) return;
    Client *client = &server->clients[index];
    int other = client->opponent;
    if (client->match_id != match_id ||
        (from_play ? client->state != CLIENT_PLAYING
                   : client->state != CLIENT_CHALLENGING) ||
        !valid_client(server, other)) return;

    if (!accept) {
        cancel_pair(server, index, "DECLINED");
        try_pair(server);
        return;
    }

    client->rematch = 1;
    if (server->clients[other].rematch) {
        int first = index < other ? index : other;
        int second = index < other ? other : index;
        begin_game(server, first, second);
    }
}

static void handle_move(Server *server, int index, int match_id, int from,
                        int to)
{
    if (!valid_client(server, index)) return;
    Client *client = &server->clients[index];
    int other = client->opponent;
    if (client->state != CLIENT_PLAYING || client->match_id != match_id ||
        !valid_client(server, other)) return;
    if (from < 0 || from >= BOARD_SIZE * BOARD_SIZE || to < 0 ||
        to >= BOARD_SIZE * BOARD_SIZE) return;
    (void)socket_send_line(server->clients[other].fd, "MOVE %d %d %d",
                           match_id, from, to);
}

static void handle_client_line(Server *server, int index, char *line)
{
    int match_id = 0;
    int from = 0;
    int to = 0;
    int accepted = 0;

    if (!valid_client(server, index)) return;
    if (!server->clients[index].authenticated) {
        handle_auth(server, index, line);
        return;
    }

    if (online_word(line, "HELLO")) {
        handle_hello(server, index, line);
    } else if (online_word(line, "BYE")) {
        drop_client(server, index);
    } else if (online_word(line, "PING")) {
        if (valid_client(server, index)) {
            (void)socket_send_line(server->clients[index].fd, "PONG");
        }
    } else if (online_word(line, "ACCEPT")) {
        const char *p = line + strlen("ACCEPT");
        if (online_parse_int(p, &match_id)) {
            handle_response(server, index, match_id, 1, 0);
        }
    } else if (online_word(line, "DECLINE")) {
        const char *p = line + strlen("DECLINE");
        if (online_parse_int(p, &match_id)) {
            handle_response(server, index, match_id, 0, 0);
        }
    } else if (online_parse_move(line, &match_id, &from, &to)) {
        handle_move(server, index, match_id, from, to);
    } else if (online_parse_rematch(line, &match_id, &accepted)) {
        handle_response(server, index, match_id, accepted, 1);
    }
}

static void accept_clients(Server *server)
{
    for (;;) {
        struct sockaddr_in peer;
        socklen_t length = sizeof(peer);
        int fd = (int)accept(server->listener, (struct sockaddr *)&peer,
                             &length);
        if (fd < 0) return;
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (server->clients[i].fd < 0) {
                slot = i;
                break;
            }
        }
        if (slot < 0) {
            (void)socket_send_line(fd, "ERROR SERVER FULL");
            closesocket(fd);
            continue;
        }
        socket_set_nonblock(fd);
        const char *token = online_token();
        char nonce[ONLINE_HASH_HEX_SIZE];
        if (!token ||
            !online_hash_hex(next_nonce(server, &peer), nonce, sizeof(nonce)) ||
            !socket_send_line(fd, "NONCE %s", nonce)) {
            closesocket(fd);
            continue;
        }
        Client *client = &server->clients[slot];
        client->fd = fd;
        client->state = CLIENT_AUTHENTICATING;
        client->match_id = 0;
        client->opponent = -1;
        client->avoid = -1;
        client->rematch = 0;
        client->ready_time = time(NULL);
        client->auth_deadline = time(NULL) + AUTH_SECONDS;
        (void)online_hash_hex(online_hash(nonce, token), client->expected_hash,
                              sizeof(client->expected_hash));
        client->authenticated = 0;
        client->name[0] = '\0';
        line_buffer_init(&client->input);
    }
}

static void expire_unauthenticated(Server *server)
{
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (valid_client(server, i) && !server->clients[i].authenticated &&
            now >= server->clients[i].auth_deadline) {
            drop_client(server, i);
        }
    }
}

static void handle_discovery(Server *server)
{
    char packet[128];
    struct sockaddr_in peer;
    socklen_t length = sizeof(peer);
    int count = (int)recvfrom(server->discovery, packet, sizeof(packet) - 1, 0,
                              (struct sockaddr *)&peer, &length);
    if (count <= 0) return;
    packet[count] = '\0';
    if (strncmp(packet, "DISCOVER", 8) != 0) return;
    char reply[64];
    snprintf(reply, sizeof(reply), "SHOGUNSERVER %d", server->port);
    (void)sendto(server->discovery, reply, strlen(reply), 0,
                 (struct sockaddr *)&peer, length);
}

static void read_clients(Server *server, fd_set *readable)
{
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        Client *client = &server->clients[i];
        if (client->fd < 0 || !FD_ISSET((unsigned int)client->fd, readable)) {
            continue;
        }
        char packet[1024];
#ifdef _WIN32
        int count = recv((SOCKET)client->fd, packet, sizeof(packet), 0);
#else
        int count = (int)recv(client->fd, packet, sizeof(packet), 0);
#endif
        if (count == 0) {
            drop_client(server, i);
            continue;
        }
        if (count < 0) {
            int error = socket_last_error();
            if (!socket_would_block(error)) drop_client(server, i);
            continue;
        }
        if (!line_buffer_push(&client->input, packet, count)) {
            drop_client(server, i);
            continue;
        }
        char line[ONLINE_MAX_LINE];
        while (line_buffer_next(&client->input, line, sizeof(line))) {
            handle_client_line(server, i, line);
            if (client->fd < 0) break;
        }
    }
}

int online_server_run(int port)
{
    if (port < 1 || port > 65535 || !online_token()) return 0;
    Server server;
    server_init(&server);
    server.port = port;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;
#endif

    server.listener = (int)socket(AF_INET, SOCK_STREAM, 0);
    server.discovery = (int)socket(AF_INET, SOCK_DGRAM, 0);
    if (server.listener < 0 || server.discovery < 0) {
        server_done(&server);
        return 0;
    }

    int reuse = 1;
    setsockopt(server.listener, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&reuse, sizeof(reuse));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);
    if (bind(server.listener, (struct sockaddr *)&address, sizeof(address)) != 0 ||
        listen(server.listener, 8) != 0 ||
        bind(server.discovery, (struct sockaddr *)&address, sizeof(address)) != 0) {
        server_done(&server);
        return 0;
    }
    socket_set_nonblock(server.listener);
    socket_set_nonblock(server.discovery);

    for (;;) {
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET((unsigned int)server.listener, &readable);
        FD_SET((unsigned int)server.discovery, &readable);
        int max_fd = server.listener > server.discovery ? server.listener
                                                        : server.discovery;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (server.clients[i].fd >= 0) {
                FD_SET((unsigned int)server.clients[i].fd, &readable);
                if (server.clients[i].fd > max_fd) max_fd = server.clients[i].fd;
            }
        }
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;
        int ready = select(max_fd + 1, &readable, NULL, NULL, &timeout);
        if (ready <= 0) {
            expire_unauthenticated(&server);
            try_pair(&server);
            continue;
        }
        if (FD_ISSET((unsigned int)server.listener, &readable)) accept_clients(&server);
        if (FD_ISSET((unsigned int)server.discovery, &readable)) handle_discovery(&server);
        read_clients(&server, &readable);
        expire_unauthenticated(&server);
        try_pair(&server);
    }
}
