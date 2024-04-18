#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "../include/utils.h"
#include "../include/network.h"
#include "../include/connection.h"
#include "../include/connection_list.h"

#define DEFAULT_PORT         "56562"
#define MAX_EVENT            64
#define CLIENT_EVENT         ( EPOLLIN | EPOLLET )
#define EPOLL_WAIT_TIMEOUT   1000 /* ms */

/* 스레드 상테 */
#define STOP    0
#define RUNNING 1

typedef struct thread_arg
{
    int state;          /* 0: stop, 1: run */
    int fd_listener;    /* accept에서 필요함 */
    int max_event;      /* 최대 이벤트 개수 */
    pthread_t tid;      /* 스레드 id */
} thread_arg;


connection_list *conn_list;


/* SIGPIPE 무시하도록 설정하는 함수 */
void sigpipe_ignore(void);

/* epoll 워커 스레드 */
void *epoll_worker(void *param);

/* 이벤트 등록, 수정, 삭제 */
int add_event(int epfd, struct connection *conn, uint32_t event_flag);
int mod_event(int epfd, struct connection *conn, uint32_t event_flag);
int del_event(int epfd, struct connection *conn);

/* epoll 이벤트 처리 함수 */
int accept_socket(int epfd, int fd_listener);
int recv_event(int epfd, connection *conn);
int send_event(int epfd, connection *conn);


/* 채팅 메시지 브로드캐스팅 */
int recv_complete(int epfd, connection *conn);
int broadcast(int epfd, connection_list *conn_list, struct packet *p);


int main(void)
{
    /* SIGPIPE 시그널 무시 */
    sigpipe_ignore();

    /* 커넥션 리스트 초기화 */
    conn_list = init_connection_list((void (*)(void *))close_connection);

    /* 리스너 소켓 생성 */
    int fd_listener = make_listen_socket(NULL, DEFAULT_PORT);

    /* epoll 스레드 매개변수 초기화 */
    thread_arg epoll_thread_arg = { .state = RUNNING, .fd_listener = fd_listener, .max_event = MAX_EVENT };

    /* epoll 스레드 생성 */
    int rc;
    if ((rc = pthread_create(&epoll_thread_arg.tid, NULL, epoll_worker, &epoll_thread_arg)))
    {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    char *buffer = NULL;
    size_t buffer_size = MAX_MSG_LENGTH;
    int n_read;
    while (1)
    {
        if ((n_read = (int)getline(&buffer, &buffer_size, stdin)) < 0)
            unix_error("getline");

        buffer[n_read - 1] = '\0';  /* '\n' 제거 */
        if (!strcmp(buffer, "/exit"))
            break;

        printf("%s\n", buffer);
    }

    free(buffer);

    /* epoll 워커 스레드 종료 */
    epoll_thread_arg.state = STOP;

    /* epoll 스레드 join */
    if ((rc = pthread_join(epoll_thread_arg.tid, NULL)))
    {
        fprintf(stderr, "pthread_join: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    close(fd_listener);
    destroy_connection_list(conn_list);

    exit(EXIT_SUCCESS);
}


/* SIGPIPE 무시하도록 설정하는 함수 */
void sigpipe_ignore(void)
{
    struct sigaction action;

    action.sa_handler = SIG_IGN;
    if (sigemptyset(&action.sa_mask) < 0)
        unix_error("sigemptyset");
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0)
        unix_error("sigaction");
}


/* epoll 워커 스레드 */
void *epoll_worker(void *param)
{
    thread_arg *arg = (thread_arg *)param;
    const int fd_listener = arg->fd_listener;
    const int max_event = arg->max_event;

    /* epoll 생성 */
    int epfd;
    if ((epfd = epoll_create(1)) < 0)
        unix_error("epoll_create");

    /* 리스너소켓 이벤트 등록 */
    struct epoll_event event;
    event.data.fd = fd_listener;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd_listener, &event) < 0)
        unix_error("epoll_ctl( EPOLL_CTL_ADD )");


    struct epoll_event *ep_events = NULL;
    if ((ep_events = (struct epoll_event *)calloc(max_event, sizeof(struct epoll_event))) == NULL)
    {
        fprintf(stderr, "calloc( %ld ): failed!\n", max_event * sizeof(struct epoll_event));
        pthread_exit(NULL);
    }

    int ret_epoll;
    while (arg->state)
    {
        if ((ret_epoll = epoll_wait(epfd, ep_events, MAX_EVENT, EPOLL_WAIT_TIMEOUT)) < 0)
        {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }
        
        for (int i = 0; i < ret_epoll; i++)
        {
            struct epoll_event *current = &ep_events[i];
            struct connection *conn = (struct connection *)current->data.ptr;

            if (current->events & EPOLLIN)   /* EPOLLIN */
            {
                if (current->data.fd == fd_listener)
                {
                    /* 리스너 소켓에서 이벤트 발생 */
                    accept_socket(epfd, fd_listener);
                }
                else
                {
                    /* 클라이언트 소켓에서 수신 가능 이벤트 발생 */
                    recv_event(epfd, conn);
                }
            }   /* end if EPOLLIN */
            else if (current->events & EPOLLOUT)    /* EPOLLOUT */
            {
                /* 클라이언트 소켓에서 송신 가능 이벤트 발생 */
                send_event(epfd, conn);
            }   /* end if EPOLLOUT */
            else if (current->events & EPOLLERR)    /* EPOLLERR */
            {
                pr_err("EPOLLERR");
            }   /* end if EPOLLERR */
            else    /* Unkown */
            {
                pr_out("UNKWON!");
            }
        }   /* end for (i) */
    }

    free(ep_events);
    close(epfd);

    pthread_exit(NULL);
}



/* 이벤트 등록, 수정, 삭제 */
int add_event(int epfd, struct connection *conn, uint32_t event_flag)
{
    struct epoll_event event;

    event.data.ptr = conn;
    event.events = event_flag;

    /* add socket event */
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn->socket_fd, &event) < 0)
    {
        perror("epoll_ctl( EPOLL_CTL_ADD )");
        return -1;
    }

    /* add timeout event */
    event.data.ptr = conn;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn->timer_fd, &event) < 0)
    {
        perror("epoll_ctl( EPOLL_CTL_ADD )");
        return -1;
    }

    return 0;
}

int mod_event(int epfd, struct connection *conn, uint32_t event_flag)
{
    struct epoll_event event;

    event.data.ptr = conn;
    event.events = event_flag;

    /* modifie socket event */
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->socket_fd, &event) < 0)
    {
        perror("epoll_ctl( EPOLL_CTL_MOD )");
        return -1;
    }

    return 0;
}

int del_event(int epfd, struct connection *conn)
{
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, conn->socket_fd, NULL) < 0)
    {
        perror("epoll_ctl( EPOLL_CTL_DEL )");
        return -1;
    }
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, conn->timer_fd, NULL) < 0)
    {
        perror("epoll_ctl( EPOLL_CTL_DEL )");
        return -1;
    }

    return 0;
}



/* epoll 이벤트 처리 함수 */
int accept_socket(int epfd, int fd_listener)
{
    int retry_cnt = 0;
    int ret_val = -1;
    int cfd;
    while (1)
    {   
        if ((cfd = accept(fd_listener, NULL, NULL)) < 0)
        {
            ret_val = -1;
            if (errno == EINTR)
            {
                retry_cnt++;
                if (retry_cnt <= MAX_RETRY_CNT)
                    continue;
                else
                    fprintf(stderr, "too many retry!\n");
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("accept");
            }
            else    /* errno == EAGAIN || errno == EWOULDBLOCK */
            {
                ret_val = 0;
            }

            break;
        }

        struct connection *new_conn = init_connection(cfd);
        add_event(epfd, new_conn, CLIENT_EVENT | EPOLLOUT);

        connection_list_insert(conn_list, new_conn);

        pr_out("accept(): add socket = %d", new_conn->socket_fd);
        retry_cnt = 0;
        ret_val = 0;    /* success! */
    }

    return ret_val;
}

int recv_event(int epfd, connection *conn)
{
    if (connection_timeout(conn))   /* 타임아웃 발생 */
    {
        pr_out("closeed by host (sfd=%d)", conn->socket_fd);

        del_event(epfd, conn);
        connection_list_delete(conn_list, conn);

        return 0;
    }
    
    /* 읽기 가능한 바이트 확인 */
    int readable_byte;
    if (ioctl(conn->socket_fd, FIONREAD, &readable_byte) < 0)
        unix_error("ioctl( FIONREAD )");

    if (readable_byte == 0)  /* 수신측에서 연결 종료 */
    {
        pr_out("closeed by host (sfd=%d)", conn->socket_fd);

        del_event(epfd, conn);
        connection_list_delete(conn_list, conn);

        return 0;
    }

    int n_recv = 0;
    while (readable_byte > 0)
    {
        /* 읽기 가능한 바이트가 프로토콜의 헤더 크기 아상이면 데이터 읽음 */
        if (conn->recv_packet != NULL)   /* resume */
            n_recv = recv_packet(conn);
        else if (readable_byte >= sizeof(struct header))    /* new packet */
            n_recv = recv_packet(conn);
        else
            break;

        if (n_recv < 0) /* recv_packet()에서 오류 발생 */
            break;

        readable_byte -= n_recv;    /* readable byte update */

        if (conn->recv_packet->offset == conn->recv_packet->data_size)  /* recv complete */
        {
            recv_complete(epfd, conn);

            /* 새로운 패킷 수신 준비 */
            free(conn->recv_packet);
            conn->recv_packet = NULL;
        }
    }  /* end if recv data */

    connection_set_timer(conn);

    return n_recv;
}

int send_event(int epfd, connection *conn)
{
    int n_send = 0;
    while (!send_queue_empty(conn) && (n_send = send_packet(conn)) > 0);

    /* 송신할 패킷이 없음 */
    if (send_queue_empty(conn))
    {
        mod_event(epfd, conn, CLIENT_EVENT);
        return 0;
    }

    if (n_send == 0)
    {
        pr_out("closeed by host (sfd=%d)", conn->socket_fd);

        del_event(epfd, conn);
        connection_list_delete(conn_list, conn);

        return 0;
    }

    return n_send;
}


/* 채팅 메시지 브로드캐스팅 */
int broadcast(int epfd, connection_list *conn_list, struct packet *p)
{
    iterator conn_list_begin = connection_list_begin(conn_list);
    iterator conn_list_end = connection_list_end(conn_list);
    for (iterator itr = conn_list_begin; itr != conn_list_end; itr = connection_list_next(itr))
    {
        struct connection *conn = connection_list_iter_get(itr);

        struct packet *new_packet = packet_duplicate(p);
        new_packet->offset = 0;
        
        send_queue_push(conn, new_packet);
        mod_event(epfd, conn, CLIENT_EVENT | EPOLLOUT);
    }

    return 0;
}

int recv_complete(int epfd, connection *conn)
{
    packet *recved_packet = conn->recv_packet;
    char *raw_data = recved_packet->data;

    struct header *h = (struct header *)raw_data;
    if (h->type == HEARTBEAT)
    {
        struct message *m = (struct message *)(raw_data + sizeof(struct header));
        pr_out("client(%d) heartbeat: %*s", conn->socket_fd, (int)m->msg_length, m->msg);
    }
    else if (h->type == CHAT_MSG)
    {
        /* broadcast */
        struct message *m = (struct message *)(raw_data + sizeof(struct header));
        pr_out("client(%d) message: %*s", conn->socket_fd, (int)m->msg_length, m->msg);
        broadcast(epfd, conn_list, conn->recv_packet);
    }
    else    /* Unkown */
    {
        print_byte(conn->recv_packet->data, conn->recv_packet->data_size);
    }

    return 0;
}
