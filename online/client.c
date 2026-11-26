#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/stat.h>
#endif

static void copy_text(char *destination, size_t size, const char *source)
{
    if (!destination || size == 0) return;
    if (!source) {
        destination[0] = '\0';
        return;
    }
    size_t n = strlen(source);
    if (n >= size) n = size - 1;
    memcpy(destination, source, n);
    destination[n] = '\0';
}

static const unsigned char hidden_server[] = {
    0x6b, 0x63, 0x68, 0x74, 0x6b, 0x6c, 0x62, 0x74, 0x6a,
    0x74, 0x6d, 0x63, 0x60, 0x6d, 0x6d, 0x6d, 0x6d
};
static const unsigned char hidden_server_key = 0x5a;

const char *online_default_server(void)
{
    static char decoded[sizeof(hidden_server) + 1];
    static int ready = 0;
    if (!ready) {
        for (size_t i = 0; i < sizeof(hidden_server); ++i) {
            decoded[i] = (char)(hidden_server[i] ^ hidden_server_key);
        }
        decoded[sizeof(hidden_server)] = '\0';
        ready = 1;
    }
    return decoded;
}

static void pref_path(char *out, size_t size)
{
    const char *home = getenv("HOME");
    if (!home || !*home) home = getenv("USERPROFILE");
#ifdef _WIN32
    const char separator = '\\';
#else
    const char separator = '/';
#endif
    if (home && *home) {
        snprintf(out, size, "%s%c.shogun_online.cfg", home, separator);
    } else {
        snprintf(out, size, "shogun-online.cfg");
    }
}

static void set_field(char *out, size_t size, const char *value)
{
    if (!out || size == 0) return;
    size_t n = value ? strlen(value) : 0;
    while (n > 0 && (value[n - 1] == '\r' || value[n - 1] == '\n')) --n;
    if (n >= size) n = size - 1;
    if (n > 0) memcpy(out, value, n);
    out[n] = '\0';
}

void online_prefs_load(char *token, size_t token_size)
{
    if (token && token_size) token[0] = '\0';
    copy_text(token, token_size, getenv("SHOGUN_TOKEN"));

    char path[512];
    pref_path(path, sizeof(path));
    FILE *file = fopen(path, "r");
    if (!file) return;
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (token && token_size && !token[0] && strncmp(line, "TOKEN=", 6) == 0) {
            set_field(token, token_size, line + 6);
        }
    }
    fclose(file);
}

void online_prefs_save(const char *token)
{
    char path[512];
    pref_path(path, sizeof(path));
    FILE *file = fopen(path, "w");
    if (!file) return;
    fprintf(file, "TOKEN=%s\n", token ? token : "");
    fclose(file);
#ifndef _WIN32
    (void)chmod(path, 0600);
#endif
}

static void set_message(OnlineNet *net, Game *game, const char *message)
{
    copy_text(net->message, sizeof(net->message), message);
    if (game) copy_text(game->online_message, sizeof(game->online_message), message);
}

static void lost_connection(OnlineNet *net, Game *game, const char *message)
{
    online_net_close(net);
    set_message(net, game, message);
    if (!game) return;
    game->online_phase = ONLINE_NONE;
    game->local_side = -1;
    if (game->screen == SCREEN_PLAY || game->screen == SCREEN_WINNER ||
        game->screen == SCREEN_ONLINE_MATCH || game->screen == SCREEN_PAUSE) {
        game->screen = SCREEN_ONLINE_WAIT;
    }
}

static int parse_endpoint(const char *text, char *host, size_t host_size,
                          int *port)
{
    if (!text || !*text) return 0;
    const char *colon = strrchr(text, ':');
    if (!colon) return 0;
    size_t n = (size_t)(colon - text);
    if (n == 0 || n >= host_size) return 0;
    memcpy(host, text, n);
    host[n] = '\0';
    char *end = NULL;
    long value = strtol(colon + 1, &end, 10);
    if (end == colon + 1 || *end != '\0' || value < 1 || value > 65535) return 0;
    *port = (int)value;
    return 1;
}

static int resolve_endpoint(const char *endpoint, char *host, size_t host_size,
                            int *port)
{
    if (!endpoint || !*endpoint || strcmp(endpoint, "AUTO") == 0) {
        char discovered[64];
        int discovered_port = ONLINE_DEFAULT_PORT;
        if (online_discover(discovered, sizeof(discovered), &discovered_port)) {
            copy_text(host, host_size, discovered);
            *port = discovered_port;
            return 1;
        }
        endpoint = online_default_server();
    }
    return parse_endpoint(endpoint, host, host_size, port);
}

void online_net_init(OnlineNet *net)
{
    net->fd = -1;
    net->state = NET_OFFLINE;
    net->name[0] = '\0';
    net->host[0] = '\0';
    net->server[0] = '\0';
    net->token[0] = '\0';
    net->port = ONLINE_DEFAULT_PORT;
    net->message[0] = '\0';
    net->connect_checked = 0;
    net->auth_sent = 0;
    line_buffer_init(&net->input);
}

void online_net_close(OnlineNet *net)
{
    if (!net) return;
    if (net->fd >= 0) {
        closesocket(net->fd);
        net->fd = -1;
    }
    line_buffer_done(&net->input);
    net->state = NET_OFFLINE;
    net->connect_checked = 0;
    net->auth_sent = 0;
}

int online_net_connected(const OnlineNet *net)
{
    return net && net->fd >= 0 && net->state != NET_OFFLINE;
}

int online_net_connect_addr(OnlineNet *net, const char *host, int port,
                             const char *name, const char *token)
{
    if (!net || !host || !name || !token || !*token ||
        port < 1 || port > 65535) return 0;
    if (socket_startup() != 0) return 0;

    online_net_close(net);

    char service[16];
    snprintf(service, sizeof(service), "%d", port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *addresses = NULL;
    if (getaddrinfo(host, service, &hints, &addresses) != 0 || !addresses) {
        return 0;
    }

    int fd = (int)socket(addresses->ai_family, addresses->ai_socktype,
                         addresses->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(addresses);
        return 0;
    }
    socket_set_nonblock(fd);
    int result = connect(fd, addresses->ai_addr, (socklen_t)addresses->ai_addrlen);
    freeaddrinfo(addresses);

    if (result != 0) {
        int error = socket_last_error();
        if (!socket_would_block(error)) {
            closesocket(fd);
            return 0;
        }
    }

    net->fd = fd;
    net->state = NET_CONNECTING;
    net->connect_checked = 0;
    net->auth_sent = 0;
    copy_text(net->token, sizeof(net->token), token);
    snprintf(net->host, sizeof(net->host), "%s", host);
    snprintf(net->server, sizeof(net->server), "%s:%d", host, port);
    net->port = port;
    online_sanitize_name(name, net->name, sizeof(net->name));
    line_buffer_init(&net->input);
    return 1;
}

int online_net_connect_string(OnlineNet *net, const char *endpoint,
                               const char *name, const char *token)
{
    if (!net || !name || !token || !*token) return 0;
    char host[sizeof(net->host)];
    int port = ONLINE_DEFAULT_PORT;
    if (!resolve_endpoint(endpoint, host, sizeof(host), &port)) return 0;
    return online_net_connect_addr(net, host, port, name, token);
}

int online_net_connect(OnlineNet *net, const char *name, const char *token)
{
    const char *endpoint = getenv("SHOGUN_SERVER");
    if (!endpoint || !*endpoint) endpoint = online_default_server();
    return online_net_connect_string(net, endpoint, name, token);
}

static int send_hello(OnlineNet *net)
{
    return socket_send_line(net->fd, "HELLO %s", net->name);
}

static int send_auth(OnlineNet *net, const char *nonce)
{
    if (!net->token[0]) return 0;
    char hash[ONLINE_HASH_HEX_SIZE];
    if (!online_hash_hex(online_hash(nonce, net->token), hash,
                         sizeof(hash))) return 0;
    if (!socket_send_line(net->fd, "AUTH %s", hash)) return 0;
    net->auth_sent = 1;
    return send_hello(net);
}

static void handle_line(OnlineNet *net, Game *game, const char *line)
{
    int match_id = 0;
    int from = 0;
    int to = 0;
    int side = 0;
    uint32_t seed = 0;
    char text[ONLINE_MESSAGE_SIZE];
    char nonce[ONLINE_HASH_HEX_SIZE];

    if (!net->auth_sent && online_parse_nonce(line, nonce, sizeof(nonce))) {
        if (!send_auth(net, nonce)) {
            lost_connection(net, game, "AUTHENTICATION FAILED");
        }
        return;
    }
    if (online_word(line, "WELCOME")) {
        net->state = NET_WAITING;
        if (game) {
            snprintf(game->name, sizeof(game->name), "%s", net->name);
            game->online_phase = ONLINE_WAITING;
            game->screen = SCREEN_ONLINE_WAIT;
            set_message(net, game, "WAITING FOR AN OPPONENT...");
        }
    } else if (online_word(line, "WAITING")) {
        net->state = NET_WAITING;
        if (game) {
            game->online_phase = ONLINE_WAITING;
            set_message(net, game, "WAITING FOR AN OPPONENT...");
        }
    } else if (online_parse_challenge(line, &match_id, text, sizeof(text))) {
        net->state = NET_CHALLENGE;
        if (game) {
            game->match_id = match_id;
            copy_text(game->opponent, sizeof(game->opponent), text);
            game->challenge_cursor = 0;
            game->online_phase = ONLINE_MATCHMAKING;
            game->screen = SCREEN_ONLINE_MATCH;
            set_message(net, game, "OPPONENT WANTS TO PLAY");
        }
    } else if (online_parse_cancelled(line, &match_id, text, sizeof(text))) {
        net->state = NET_WAITING;
        if (game) {
            game->online_phase = ONLINE_WAITING;
            game->screen = SCREEN_ONLINE_WAIT;
            if (strcmp(text, "DECLINED") == 0) {
                set_message(net, game, "OPPONENT DECLINED");
            } else if (strcmp(text, "LEFT") == 0) {
                set_message(net, game, "OPPONENT LEFT");
            } else {
                set_message(net, game, "MATCH CANCELLED");
            }
        }
    } else if (online_parse_start(line, &match_id, &seed, &side)) {
        net->state = NET_PLAYING;
        if (game) {
            game->match_id = match_id;
            game_start_online(game, seed, (Side)side);
        }
    } else if (online_parse_move(line, &match_id, &from, &to)) {
        if (!game || match_id != game->match_id) return;
        Move move;
        move.from.row = from / BOARD_SIZE;
        move.from.col = from % BOARD_SIZE;
        move.to.row = to / BOARD_SIZE;
        move.to.col = to % BOARD_SIZE;
        move.capture = 0;
        if (!game_apply_remote_move(game, &move)) {
            lost_connection(net, game, "OPPONENT SENT AN INVALID MOVE");
        }
    } else if (online_word(line, "ERROR")) {
        const char *reason = line + strlen("ERROR");
        while (*reason == ' ') ++reason;
        set_message(net, game, reason);
        if (game) {
            game->online_phase = ONLINE_NONE;
            game->local_side = -1;
            game->screen = SCREEN_ONLINE_WAIT;
        }
    } else if (online_word(line, "PONG")) {
        /* Keepalive reply. */
    }
}

void online_net_update(OnlineNet *net, Game *game)
{
    if (!net || net->fd < 0) return;

    if (net->state == NET_CONNECTING && !net->connect_checked) {
        fd_set writable;
        FD_ZERO(&writable);
        FD_SET((unsigned int)net->fd, &writable);
        struct timeval timeout = {0, 0};
        if (select(net->fd + 1, NULL, &writable, NULL, &timeout) > 0) {
            int error = 0;
            socklen_t length = sizeof(error);
            if (getsockopt(net->fd, SOL_SOCKET, SO_ERROR, (char *)&error,
                           &length) != 0 || error != 0) {
                lost_connection(net, game, "COULD NOT CONNECT TO SERVER");
                return;
            }
            net->connect_checked = 1;
        }
    }

    fd_set readable;
    FD_ZERO(&readable);
    FD_SET((unsigned int)net->fd, &readable);
    struct timeval timeout = {0, 0};
    int ready = select(net->fd + 1, &readable, NULL, NULL, &timeout);
    if (ready < 0) {
        if (!socket_would_block(socket_last_error())) {
            lost_connection(net, game, "CONNECTION LOST");
        }
        return;
    }
    if (ready == 0 || !FD_ISSET((unsigned int)net->fd, &readable)) return;

    char packet[1024];
#ifdef _WIN32
    int count = recv((SOCKET)net->fd, packet, sizeof(packet), 0);
#else
    ssize_t received = recv(net->fd, packet, sizeof(packet), 0);
    int count = (int)received;
#endif
    if (count == 0) {
        lost_connection(net, game, "SERVER CLOSED THE CONNECTION");
        return;
    }
    if (count < 0) {
        int error = socket_last_error();
        if (!socket_would_block(error)) {
            lost_connection(net, game, "CONNECTION LOST");
        }
        return;
    }
    if (!line_buffer_push(&net->input, packet, count)) {
        lost_connection(net, game, "OUT OF MEMORY");
        return;
    }

    char line[ONLINE_MAX_LINE];
    while (line_buffer_next(&net->input, line, sizeof(line))) {
        handle_line(net, game, line);
        if (net->fd < 0) break;
    }
}

void online_net_send_accept(OnlineNet *net, int match_id)
{
    if (online_net_connected(net)) {
        (void)socket_send_line(net->fd, "ACCEPT %d", match_id);
    }
}

void online_net_send_decline(OnlineNet *net, int match_id)
{
    if (online_net_connected(net)) {
        (void)socket_send_line(net->fd, "DECLINE %d", match_id);
    }
}

void online_net_send_move(OnlineNet *net, int match_id, const Move *move)
{
    if (!move || !online_net_connected(net)) return;
    int from = move->from.row * BOARD_SIZE + move->from.col;
    int to = move->to.row * BOARD_SIZE + move->to.col;
    (void)socket_send_line(net->fd, "MOVE %d %d %d", match_id, from, to);
}

void online_net_send_rematch(OnlineNet *net, int match_id, int accepted)
{
    if (online_net_connected(net)) {
        (void)socket_send_line(net->fd, "REMATCH %d %s", match_id,
                               accepted ? "YES" : "NO");
    }
}

void online_net_send_leave(OnlineNet *net, int match_id)
{
    if (online_net_connected(net)) {
        (void)socket_send_line(net->fd, "BYE %d", match_id);
    }
}
