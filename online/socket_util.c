#include "socket_util.h"

#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#endif

void line_buffer_init(LineBuffer *buffer)
{
    buffer->data = NULL;
    buffer->len = 0;
    buffer->cap = 0;
}

void line_buffer_done(LineBuffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->len = 0;
    buffer->cap = 0;
}

int line_buffer_push(LineBuffer *buffer, const char *data, int count)
{
    if (count <= 0) return 1;
    size_t need = buffer->len + (size_t)count + 1;
    if (need > buffer->cap) {
        size_t cap = buffer->cap ? buffer->cap : 256;
        while (cap < need) cap *= 2;
        char *grown = (char *)realloc(buffer->data, cap);
        if (!grown) return 0;
        buffer->data = grown;
        buffer->cap = cap;
    }
    memcpy(buffer->data + buffer->len, data, (size_t)count);
    buffer->len += (size_t)count;
    buffer->data[buffer->len] = '\0';
    return 1;
}

int line_buffer_next(LineBuffer *buffer, char *line, size_t size)
{
    if (!buffer->data || size == 0) return 0;
    char *newline = (char *)memchr(buffer->data, '\n', buffer->len);
    if (!newline) return 0;
    size_t n = (size_t)(newline - buffer->data);
    if (n >= size) n = size - 1;
    memcpy(line, buffer->data, n);
    line[n] = '\0';
    size_t used = (size_t)(newline - buffer->data) + 1;
    memmove(buffer->data, buffer->data + used, buffer->len - used);
    buffer->len -= used;
    buffer->data[buffer->len] = '\0';
    return 1;
}

int socket_startup(void)
{
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0 ? 0 : -1;
#else
    return 0;
#endif
}

int socket_set_nonblock(int fd)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket((SOCKET)fd, FIONBIO, &mode) == 0 ? 0 : -1;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

int socket_last_error(void)
{
#ifdef _WIN32
    return (int)WSAGetLastError();
#else
    return errno;
#endif
}

int socket_would_block(int error)
{
#ifdef _WIN32
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
#else
    return error == EAGAIN || error == EWOULDBLOCK || error == EINPROGRESS;
#endif
}

int socket_send_all(int fd, const char *data, size_t size)
{
    size_t sent = 0;
    while (sent < size) {
#ifdef _WIN32
        int n = send((SOCKET)fd, data + sent, (int)(size - sent), 0);
#else
        ssize_t n = send(fd, data + sent, size - sent, 0);
#endif
        if (n < 0) {
            int error = socket_last_error();
#ifdef _WIN32
            if (error == WSAEINTR) continue;
#else
            if (error == EINTR) continue;
#endif
            if (socket_would_block(error)) return 1;
            return 0;
        }
        if (n == 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

int socket_send_line(int fd, const char *format, ...)
{
    char message[ONLINE_MAX_LINE];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= sizeof(message)) return 0;
    message[written] = '\n';
    return socket_send_all(fd, message, (size_t)written + 1);
}

int online_discover(char *host, size_t host_size, int *port)
{
    if (!host || host_size == 0 || !port) return 0;
    int fd = (int)socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return 0;

    int broadcast = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast,
               sizeof(broadcast));

    const char *request = "DISCOVER\n";
    struct sockaddr_in target;
    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons((unsigned short)ONLINE_DEFAULT_PORT);
    target.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    (void)sendto(fd, request, strlen(request), 0,
                 (struct sockaddr *)&target, sizeof(target));
    target.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    (void)sendto(fd, request, strlen(request), 0,
                 (struct sockaddr *)&target, sizeof(target));

    fd_set readable;
    FD_ZERO(&readable);
    FD_SET((unsigned int)fd, &readable);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 250000;
    int ready = select(fd + 1, &readable, NULL, NULL, &timeout);
    if (ready > 0 && FD_ISSET((unsigned int)fd, &readable)) {
        char packet[128];
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        int n = (int)recvfrom(fd, packet, sizeof(packet) - 1, 0,
                              (struct sockaddr *)&from, &from_len);
        if (n > 0) {
            packet[n] = '\0';
            int found_port = 0;
            if (sscanf(packet, "SHOGUNSERVER %d", &found_port) == 1 &&
                found_port > 0 && found_port < 65536) {
                const char *address = inet_ntoa(from.sin_addr);
                snprintf(host, host_size, "%s", address);
                *port = found_port;
                closesocket(fd);
                return 1;
            }
        }
    }

    closesocket(fd);
    return 0;
}
