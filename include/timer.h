#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <signal.h>     /* union sigval */

#define HB_CYCLE     10      /* 하트비트 주기 sec  */

int make_timerfd(void);
int set_timer(int tfd);
int timeout(int tfd);

timer_t make_timer(int interval_delay_ms, int interval_ms, void (*routine)(union sigval));
void delete_timer(timer_t timer);

#ifdef _NETWORK_
int recv_timeout(int fd, void *buffer, size_t n, int flags, int timeout_ms)
#endif

#ifdef _STDIO_H
int getline_timeout(char **restrict lineptr, size_t *restrict n, FILE *restrict stream, const int timeout_ms);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* _TIMER_H_ */