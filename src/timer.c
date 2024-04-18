#define _POSIX_C_SOURCE  200809L

#include "../include/timer.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <sys/timerfd.h>
#include <unistd.h>

int make_timerfd(void)
{
    /* create timerfd */
    int tfd;
    if ((tfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC)) < 0)
    {
        perror("timer_create");
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
