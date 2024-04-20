#define _POSIX_C_SOURCE  200809L

#include "../include/timer.h"
#include "../include/network.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sys/timerfd.h>
#include <sys/select.h>

#include <unistd.h>


int make_timerfd(void)
{
    /* create timerfd */
    int tfd;
    if ((tfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC)) < 0)
    {
        perror("timerfd_create");
        return -1;
    }

    return tfd;
}

int set_timer(int tfd)
{
    struct itimerspec rt_itspec;
    memset(&rt_itspec, 0, sizeof(rt_itspec));
    rt_itspec.it_value.tv_sec = HB_CYCLE;

    if (timerfd_settime(tfd, 0, &rt_itspec, NULL) < 0)
    {
        perror("timerfd_settime");
        return -1;
    }

    return 0;
}

int timeout(int tfd)
{
    uint64_t read_data;
    int n_read;
    int ret_val = -1;
    if ((n_read = (int)read(tfd, &read_data, sizeof(read_data))) < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            ret_val = 0;    /* not timeout */
        else
            ret_val = -1;   /* error */
    }
    else
    {
        ret_val = 1;    /* timeout */
    }
    
    return ret_val;
}


timer_t make_timer(int interval_delay_ms, int interval_ms, void (*routine)(union sigval))
{
    struct itimerspec rt_itspec;
    /* 1sec = 1000ms = 1000000us = 1000000000ns */
    /* 최초 만료 주기 */
    rt_itspec.it_value.tv_sec = interval_delay_ms / 1000;
    rt_itspec.it_value.tv_nsec = (interval_delay_ms % 1000) * 1000000;

    /* 인터벌 주기 */
    rt_itspec.it_interval.tv_sec = interval_ms / 1000;
    rt_itspec.it_interval.tv_nsec = (interval_ms % 1000) * 1000000;

    timer_t timer;
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_value.sival_ptr = &timer;
    sigev.sigev_notify_function = routine;
    sigev.sigev_notify_attributes = NULL;

    if (timer_create(CLOCK_MONOTONIC, &sigev, &timer) < 0)
        perror("timer_create");
    
    if (timer_settime(timer, 0, &rt_itspec, NULL) < 0)
        perror("timer_settime");

    return timer;
}

void delete_timer(timer_t timer)
{
    if (timer_delete(timer) < 0)
        perror("timer_delete");
}

/* 타임아웃이 가능한 recv 함수 */
int recv_timeout(int fd, void *buffer, size_t n, int flags, int timeout_ms)
{
    /**
     * 1s = 1000 ms = 1000000us
    **/
    long sec = timeout_ms / 1000;
    long usec = (timeout_ms % 1000) * 1000;
    
    fd_set set;

    FD_ZERO(&set);
    FD_SET(fd, &set);
    
    struct timeval tv = { .tv_sec = sec, .tv_usec = usec };
    
    if (select(fd + 1, &set, NULL, NULL, &tv) < 0)
        perror("select");
    
    if (FD_ISSET(fd, &set))
        return recv_data(fd, buffer, n, flags);

    return -1;  /* time out */
}

/* 타임아웃이 가능한 getline 함수 */
int getline_timeout(char **restrict lineptr, size_t *restrict n, FILE *restrict stream, const int timeout_ms)
{
    /**
     * 1s = 1000 ms = 1000000us
    **/
    long sec = timeout_ms / 1000;
    long usec = (timeout_ms % 1000) * 1000;

    fd_set set;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    struct timeval tv = { .tv_sec = sec, .tv_usec = usec };

    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) < 0)
        perror("select");

    if (FD_ISSET(STDIN_FILENO, &set))
        return (int)getline(lineptr, n, stream);

    return -1;  /* timeout */
}
