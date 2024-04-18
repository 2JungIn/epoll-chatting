#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HB_CYCLE     10      /* 하트비트 주기: 5초 */

int make_timerfd(void);
int set_timer(int tfd);
int timeout(int tfd);

#ifdef __cplusplus
}
#endif

#endif  /* _TIMER_H_ */