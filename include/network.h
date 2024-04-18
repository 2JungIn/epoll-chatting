#ifndef _NETWORK_H_
#define _NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define BACKLOG         256  /* 최대 백로그 크기 */
#define PORTSTRLEN      sizeof("65535")
#define MAX_RETRY_CNT   5    /* EINTR 발생 시 재시도 가능한 최대 횟수 */
#define RECV_TIMEOUT    100  /* ms */

/* 소켓 옵션 설정 함수 */
int fcntl_setnb(int fd);
int setsockopt_reuseaddr(int fd);
int setsockopt_tcpnodelay(int fd);
int setsockopt_linger(int fd);

/* recv, send wrapping 함수 */
int recv_data(int fd, void *buf, size_t n, int flag);
int send_data(int fd, const void *buf, size_t n, int flag);

/* 타임아웃이 가능한 recv 함수 */
int recv_timeout(int fd, void *buffer, size_t n, int flags, int timeout_ms);

int make_listen_socket(const char *host, const char *port);
int connect_server(const char *host, const char *port);

#ifdef __cplusplus
}
#endif

#endif  /* _NETWORK_H_ */