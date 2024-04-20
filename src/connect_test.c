#define _POSIX_C_SOURCE 200809L

#include <stdio.h>

#include <sys/resource.h>
#include <unistd.h>

#include "../include/utils.h"
#include "../include/network.h"

#define MAX_CONNECTION  10000
#define SERVER_ADDRESS  "127.0.0.1"
#define SERVER_PORT     "56562"

int main(void)
{
    /* 연결 테스트 */
    struct rlimit rlimit;
    if (getrlimit(RLIMIT_OFILE, &rlimit) < 0)
        unix_error("getrlimit");
    printf("rlim_cur: %ld, rlim_max: %ld\n", rlimit.rlim_cur, rlimit.rlim_max);
    
    if (rlimit.rlim_cur < MAX_CONNECTION)
    {
        rlimit.rlim_cur += MAX_CONNECTION;
        if (setrlimit(RLIMIT_OFILE, &rlimit) < 0)
            unix_error("setrlimit");

        if (getrlimit(RLIMIT_OFILE, &rlimit) < 0)
            unix_error("getrlimit");

        printf("rlim_cur: %ld, rlim_max: %ld\n", rlimit.rlim_cur, rlimit.rlim_max);
    }

    /* 서버 연결 */
    int connections[MAX_CONNECTION];
    for (int i = 0; i < MAX_CONNECTION; i++)
    {
        if ((connections[i] = connect_server(SERVER_ADDRESS, SERVER_PORT)) < 0)
            fprintf(stderr, "connection: %d connect error!\n", connections[i]);
    }

    getchar();

    /* 연결 종료*/
    for (int i = 0; i < MAX_CONNECTION; i++)
    {
        if (close(connections[i]) < 0)
        {
            fprintf(stderr, "connection: %d ", connections[i]);
            perror("close");
        }
    }

    return 0;
}
