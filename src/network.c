#define _POSIX_C_SOURCE  200809L

#include "../include/network.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>

#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>          /* struct addrinfo, getaddrinfo(), getnameinfo() */

#include <netinet/tcp.h>    /* TCP_NODELAY */

/* 소켓 옵션 설정 함수 */
int fcntl_setnb(int fd)
{
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0)
    {
        perror("fcntl( F_SETFL )");
        return -1;
    }

    return 0;
}

int setsockopt_reuseaddr(int fd)
{
    int on = 1;
    if ((setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
    {
        perror("setsockopt( SO_REUSEADDR )");
        return -1;
    }

    return 0;
}

int setsockopt_tcpnodelay(int fd)
{
    int on = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) < 0)
    {
        perror("setsockopt( TCP_NODELAY )");
        return -1;
    }

    return 0;
}

int setsockopt_linger(int fd)
{
    struct linger l = {.l_onoff = 1, .l_linger = 0};
    if ((setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l))) < 0)
    {
        perror("setsockopt( SO_LINGER )");
        return -1;
    }

    return 0;
}

int setsockopt_sndbuf(int fd, int size)
{
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0)
    {
        perror("setsockopt( SO_SNDBUF )");
        return -1;
    }

    return 0;
}


/* recv, send wrapping 함수 */
int recv_data(int fd, void *buf, size_t n, int flag)
{
    int n_recv;
    int retry_cnt = 0;
    while (1)
    {
        if ((n_recv = (int)recv(fd, buf, n, flag)) < 0)
        {
            if (errno == EINTR)
            {
                retry_cnt++;
                if (retry_cnt <= MAX_RETRY_CNT)
                    continue;
                else
                    fprintf(stderr, "recv: too many retry!\n");
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("recv");
            }
        }

        /**
         * case 1: 정상적으로 읽거나 쓴 경우
         * case 2: 논블로킹 모드일 때, errno가 EAGAIN, EWOULDBLOCK인 경우
         * case 3: errno가 EINTR, EAGAIN, EWOULDBLOCK이 아닌 경우
         * case 4: 재시도 횟수가 너무 많은 경우
         * 
         * (EAGAIN, EWOULDBLOCK은 논블로킹 모드에서만 발생)
        **/
        break;
    }
    
    return n_recv;
}

int send_data(int fd, const void *buf, size_t n, int flag)
{
    int n_send;
    int retry_cnt = 0;
    while (1)
    {
        if ((n_send = (int)send(fd, buf, n, flag)) < 0)
        {
            if (errno == EINTR)
            {
                retry_cnt++;
                if (retry_cnt <= MAX_RETRY_CNT)
                    continue;
                else
                    fprintf(stderr, "send: too many retry!\n");
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("send");
            }
        }

        /**
         * case 1: 정상적으로 읽거나 쓴 경우
         * case 2: 논블로킹 모드일 때, errno가 EAGAIN, EWOULDBLOCK인 경우
         * case 3: errno가 EINTR, EAGAIN, EWOULDBLOCK이 아닌 경우
         * case 4: 재시도 횟수가 너무 많은 경우
         * 
         * (EAGAIN, EWOULDBLOCK은 논블로킹 모드에서만 발생)
        **/
        break;
    }

    return n_send;
}


int make_listen_socket(const char *host, const char *port)
{
    struct addrinfo hints, *ai;
    int rc_gai;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;

    if ((rc_gai = getaddrinfo(host, port, &hints, &ai))) /* error: gaddrinfo() */
    {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(rc_gai));
        exit(EXIT_FAILURE);
    }

    int fd_listener = -1;
    for (struct addrinfo *curr = ai; curr; curr = curr->ai_next)
    {
        if ((fd_listener = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol)) < 0)   /* error: socket() */
        {
            perror("socket");
            continue;
        }

        if (setsockopt_reuseaddr(fd_listener) < 0)   /* error: setsockopt_reuseaddr() */
        {
            close(fd_listener);
            continue;
        }

        /* tcp no delay */
        if (setsockopt_tcpnodelay(fd_listener) < 0)
        {
            close(fd_listener);
            continue;
        }

        if (fcntl_setnb(fd_listener) < 0)    /* error: fcntl_setnb() */
        {
            close(fd_listener);
            continue;
        }

        if (bind(fd_listener, curr->ai_addr, curr->ai_addrlen) < 0)  /* error: bind() */
        {
            perror("bind");
            close(fd_listener);
            continue;
        }

        if (listen(fd_listener, BACKLOG) < 0)    /* error: listen() */
        {
            perror("listen");
            close(fd_listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai);

    struct sockaddr_storage saddr_s;
    socklen_t len_saddr_s = sizeof(saddr_s);
    if (getsockname(fd_listener, (struct sockaddr *)&saddr_s, &len_saddr_s) < 0)
    {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }
    
    char bind_ip[INET6_ADDRSTRLEN];
    char bind_port[PORTSTRLEN];
    if ((rc_gai = getnameinfo(
        (struct sockaddr *)&saddr_s, len_saddr_s, 
        bind_ip, sizeof(bind_ip),
        bind_port, sizeof(bind_port),
        NI_NUMERICHOST | NI_NUMERICSERV)))
    {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(rc_gai));
        exit(EXIT_FAILURE);
    }
    
    printf("Server opened: (%s:%s)\n", bind_ip, bind_port);

    return fd_listener;
}

int connect_server(const char *host, const char *port)
{
    struct addrinfo hints, *ai;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rc_gai;
    if ((rc_gai = getaddrinfo(host, port, &hints, &ai)))
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc_gai));
        exit(EXIT_FAILURE);
    }

    /* make socket */
    int sd;
    if ((sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
        perror("socket");

    /* server conect */
    if (connect(sd, ai->ai_addr, ai->ai_addrlen) < 0)
        perror("connect");

    freeaddrinfo(ai);

    if (sd > 0)
        printf("chatting server(%s:%s) connect success!\n", host, port);

    return sd;
}
