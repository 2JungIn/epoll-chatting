#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <unistd.h>
#include <spawn.h>

#define MAX_CLIENTS         500
#define SEND_CLIENT_CNT     100
#define MAX_FILE_NAME_LEN   1024
#define EXECUTE_PROCESS     "./dummy_client"

#define INTERVAL_DELAY      1000    /* 최초 타이머 만기 지연 (ms) */
#define INTERVAL            3000    /* 타이머 만기 주기 (ms) */

void unix_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void send_sigusr1(union sigval arg)
{
    if (kill(-getpid(), SIGUSR1) < 0)
        unix_error("kill");
}


int main(void)
{
    sigset_t sig_mask;
    if (sigemptyset(&sig_mask) < 0)
        unix_error("sigemptyset");
    if (sigaddset(&sig_mask, SIGUSR1) < 0)
        unix_error("sigaddset");
    if (sigprocmask(SIG_SETMASK, &sig_mask, NULL) < 0)
        unix_error("sigprocmask");

    char buffer[MAX_FILE_NAME_LEN];
    char *argv[] = { EXECUTE_PROCESS, buffer, NULL, NULL };
    pid_t childs[MAX_CLIENTS];

    int rc;
    posix_spawnattr_t spawn_attr;
    if ((rc = posix_spawnattr_init(&spawn_attr)))
    {
        fprintf(stderr, "posix_spawnattr_init: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }
    if ((rc = posix_spawnattr_setflags(&spawn_attr, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK)))
    {
        fprintf(stderr, "posix_spawnattr_setflags: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    /* child process create */
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        sprintf(argv[1], "log/client(%d).txt", i);

        if (i < SEND_CLIENT_CNT)
            argv[2] = "send";
        else
            argv[2] = "listen";

        if ((rc = posix_spawn(&childs[i], argv[0], NULL, &spawn_attr, argv, NULL)))
        {
            fprintf(stderr, "posix_spawn: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }

    if ((rc = posix_spawnattr_destroy(&spawn_attr)))
    {
        fprintf(stderr, "posix_spawnattr_destroy: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    sleep(5);

    if (kill(-getpid(), SIGUSR1) < 0)
        unix_error("kill");

    getchar();

    struct itimerspec rt_itspec;
    /* 1sec = 1000ms = 1000000us = 1000000000ns */
    /* 최초 만료 주기 */
    rt_itspec.it_value.tv_sec = INTERVAL_DELAY / 1000;
    rt_itspec.it_value.tv_nsec = (INTERVAL_DELAY % 1000) * 1000000;

    /* 인터벌 주기 */
    rt_itspec.it_interval.tv_sec = INTERVAL / 1000;
    rt_itspec.it_interval.tv_nsec = (INTERVAL % 1000) * 1000000;

    timer_t timer;
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_value.sival_ptr = &timer;
    sigev.sigev_notify_function = send_sigusr1;
    sigev.sigev_notify_attributes = NULL;

    if (timer_create(CLOCK_MONOTONIC, &sigev, &timer) < 0)
        unix_error("timer_create");
    
    if (timer_settime(timer, 0, &rt_itspec, NULL) < 0)
        unix_error("timer_settime");

    /* child process wait */
    int status;
    int exit_status;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (waitpid(childs[i], &status, 0) < 0)
            unix_error("waitpid");
        if (WIFEXITED(status) && (exit_status = WEXITSTATUS(status)) != 0)
            printf("child %d: exit status: %d\n", i, exit_status);
        else if (WIFSIGNALED(status))
            printf("child %d: term sig: %d\n", i, WTERMSIG(status));
    }

    if (timer_delete(timer) < 0)
        unix_error("timer_delete");

    printf("stress test exit\n");

    exit(EXIT_SUCCESS);
}