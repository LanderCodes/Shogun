#ifndef SHOGUN_ONLINE_CLIENT_H
#define SHOGUN_ONLINE_CLIENT_H

#include "core/game.h"
#include "core/types.h"
#include "protocol.h"
#include "socket_util.h"

typedef enum {
    NET_OFFLINE = 0,
    NET_CONNECTING,
    NET_WAITING,
    NET_CHALLENGE,
    NET_PLAYING
} NetState;

typedef struct {
    int fd;
    NetState state;
    char name[ONLINE_NAME_SIZE];
    char host[64];
    char server[128];
    char token[ONLINE_TOKEN_SIZE];
    int port;
    char message[ONLINE_MESSAGE_SIZE];
    LineBuffer input;
    int connect_checked;
    int auth_sent;
} OnlineNet;

const char *online_default_server(void);
void online_prefs_load(char *token, size_t token_size, char *server,
                       size_t server_size);
void online_prefs_save(const char *token, const char *server);
void online_net_init(OnlineNet *net);
int online_net_connect(OnlineNet *net, const char *name, const char *token);
int online_net_connect_addr(OnlineNet *net, const char *host, int port,
                             const char *name, const char *token);
int online_net_connect_string(OnlineNet *net, const char *endpoint,
                               const char *name, const char *token);

void online_net_update(OnlineNet *net, Game *game);
void online_net_send_accept(OnlineNet *net, int match_id);
void online_net_send_decline(OnlineNet *net, int match_id);
void online_net_send_move(OnlineNet *net, int match_id, const Move *move);
void online_net_send_rematch(OnlineNet *net, int match_id, int accepted);
void online_net_send_leave(OnlineNet *net, int match_id);
void online_net_close(OnlineNet *net);
int online_net_connected(const OnlineNet *net);

#endif
