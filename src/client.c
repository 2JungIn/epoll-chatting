#define _POSIX_C_SOURCE  200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <signal.h>

#include "../include/utils.h"
#include "../include/packet.h"
#include "../include/network.h"
#include "../include/worker_queue.h"


/* 스레드 상테 */
#define STOP    0
#define RUNNING 1

#define INTERVAL_DELAY  1000    /* 최초 타이머 만기 지연 (ms) */
#define INTERVAL        7000    /* 타이머 만기 주기 (ms) */

typedef struct thread_arg
{
    int state;       /* 0: stop, 1: run */
    int fd;          /* accept에서 필요함 */
    pthread_t tid;   /* 스레드 id */
    void *(*start_routine)(void *);   /* 스레드 작업 함수 */
} thread_arg;


struct worker_queue *send_queue;
struct worker_queue *msg_queue;


/* 데이터 송수신 스레드 */
void *send_thread(void *param);
void *recv_thread(void *param);

/* 데이터 처리 스레드 */
void *print_thread(void *param);

/* 스레드 생성, 제거 */
void thread_create(thread_arg *args, const int n);
void thread_cleanup(thread_arg *args, const int n);

/* 타이머 만료 시 하트비트를 보내는 스레드 */
void send_heartbeat(union sigval arg);


int main(void)
{
    int sd = connect_server("127.0.0.1", "56562");

    /* 스레드 생성 */
    thread_arg args[] =
    {
        { .fd = sd, .state = RUNNING, .start_routine = recv_thread },
        { .fd = sd, .state = RUNNING, .start_routine = send_thread },
        { .fd = -1, .state = RUNNING, .start_routine = print_thread }
    };
    int n = (int)(sizeof(args) / sizeof(args[0]));
    thread_create(args, n);

    /* 하트 비트를 보내기 위한 타이머 생성 */
    struct itimerspec rt_itspec;
    /* 1sec = 1000ms = 1000000us = 1000000000ns */
    /* 최초 만료 주기 */
    rt_itspec.it_value.tv_sec = INTERVAL_DELAY / 1000;
    rt_itspec.it_value.tv_nsec = (INTERVAL_DELAY % 1000) * 1000000;

    /* 인터벌 주기 */
    rt_itspec.it_interval.tv_sec = INTERVAL / 1000;
    rt_itspec.it_interval.tv_nsec = (INTERVAL % 1000) * 1000000;

    timer_t heartbeat_timer;
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_value.sival_ptr = &heartbeat_timer;
    sigev.sigev_notify_function = send_heartbeat;
    sigev.sigev_notify_attributes = NULL;

    if (timer_create(CLOCK_MONOTONIC, &sigev, &heartbeat_timer) < 0)
        unix_error("timer_create");
    
    if (timer_settime(heartbeat_timer, 0, &rt_itspec, NULL) < 0)
        unix_error("timer_settime");
    
    char *input = NULL;
    size_t len_input = MAX_MSG_LENGTH;
    int ret_line;
    while (1)
    {
        if ((ret_line = getline(&input, &len_input, stdin)) < 0)
            unix_error("getline");
        
        input[ret_line - 1] = '\0';    /* '\n' 제거 */
        if (!strcmp(input, "/exit"))
            break;

        worker_queue_push(send_queue, msg_packet(input, ret_line));
    }

    free(input);

    /* 타이머 제거 */
    if (timer_delete(heartbeat_timer) < 0)
        unix_error("timer_delete");

    /* 스레드 클린업 */
    thread_cleanup(args, n);
    close(sd);

    exit(EXIT_SUCCESS);
}


void *send_thread(void *param)
{
    thread_arg *arg = (thread_arg *)param;
    int fd = arg->fd;

    int n_send;
    while (arg->state)
    {
        struct packet *send_packet = worker_queue_pop(send_queue);
        
        if (arg->state)
        {
            while (send_packet->offset < send_packet->data_size)
            {
                size_t offset = send_packet->offset;
                char *data = send_packet->data + offset;
                size_t n_size = send_packet->data_size - offset;

                if ((n_send = send_data(fd, data, n_size, 0)) < 0)
                    break;
                else if (n_send == 0)
                    break;
                else
                    send_packet->offset += n_send;
            }
        }

        free(send_packet);
    }

    pthread_exit(NULL);
}

void *recv_thread(void *param)
{
    thread_arg *arg = (thread_arg *)param;
    int fd = arg->fd;

    struct header h;
    int n_recv;
    while (arg->state)
    {
        if ((n_recv = recv_timeout(fd, &h, sizeof(struct header), 0, RECV_TIMEOUT)) < 0)
        {
            if (errno)
                break;
        }
        else if (n_recv == 0)
        {
            break;
        }
        else
        {
            struct packet *recv_packet = make_packet(&h);

            while (recv_packet->offset < recv_packet->data_size)
            {
                size_t offset = recv_packet->offset;
                char *data = recv_packet->data + offset;
                size_t n_size = recv_packet->data_size - offset;

                if ((n_recv = recv_data(fd, data, n_size, 0)) < 0)
                    break;
                else if (n_recv == 0)
                    break;
                else
                    recv_packet->offset += n_recv;
            }

            if (h.type == CHAT_MSG)    /* chat message */
                worker_queue_push(msg_queue, recv_packet);
            else    /* unkown type */
                print_byte(recv_packet->data, recv_packet->data_size);
        }
    }
    
    pthread_exit(NULL);
}

void *print_thread(void *param)
{
    thread_arg *arg = (thread_arg *)param;

    while (arg->state)
    {
        struct packet *print_packet = worker_queue_pop(msg_queue);

        if (arg->state)
        {
            char *raw_data = print_packet->data;
            struct message *m = (struct message *)(raw_data + sizeof(struct header));

            printf("> %s\n", m->msg);
        }

        free(print_packet);
    }

    pthread_exit(NULL);
}


void thread_create(thread_arg *args, const int n)
{
    send_queue = init_worker_queue();
    msg_queue = init_worker_queue();

    int rc;
    for (int i = 0; i < n; i++)
    {
        if ((rc = pthread_create(&args[i].tid, NULL, args[i].start_routine, &args[i])))
        {
            fprintf(stderr, "[send_thread] pthread_crate: %s", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }
}

void thread_cleanup(thread_arg *args, const int n)
{
    /* running 에서 stop을 변경 */
    for (int i = 0; i < n; i++)
        args[i].state = STOP;

    int rc;
    /* 브로드 캐스팅을 해서 cond_wait 상태인 스레드들을 모두 깨움 */
    if ((rc = pthread_cond_broadcast(&send_queue->cond)) < 0)
    {
        fprintf(stderr, "pthread_cond_broadcast: %s", strerror(rc));
        exit(EXIT_FAILURE);
    }
    if ((rc = pthread_cond_broadcast(&msg_queue->cond)) < 0)
    {
        fprintf(stderr, "pthread_cond_broadcast: %s", strerror(rc));
        exit(EXIT_FAILURE);
    }

    /* thread join */
    for (int i = 0; i < n; i++)
    {
        if ((rc = pthread_join(args[i].tid, NULL)))
        {
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }

    /* worker queue 파괴 */
    destroy_worker_queue(send_queue, free);
    destroy_worker_queue(msg_queue, free);
}


void send_heartbeat(union sigval arg)
{
    char *current_date = date();
    worker_queue_push(send_queue, heartbeat_packet(date(), strlen(current_date) + 1));
}
