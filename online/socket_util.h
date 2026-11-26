#ifndef SHOGUN_SOCKET_UTIL_H
#define SHOGUN_SOCKET_UTIL_H

#include <stdarg.h>
#include <stddef.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define closesocket close
#endif

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} LineBuffer;

void line_buffer_init(LineBuffer *buffer);
void line_buffer_done(LineBuffer *buffer);
int line_buffer_push(LineBuffer *buffer, const char *data, int count);
int line_buffer_next(LineBuffer *buffer, char *line, size_t size);

int socket_startup(void);
int socket_set_nonblock(int fd);
int socket_send_all(int fd, const char *data, size_t size);
int socket_send_line(int fd, const char *format, ...);
int socket_last_error(void);
int socket_would_block(int error);
int online_discover(char *host, size_t host_size, int *port);

#endif
