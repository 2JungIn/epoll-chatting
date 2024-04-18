#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"
#include "packet.h"
#include "queue.h"

typedef struct connection
{
    int socket_fd;
    int timer_fd;
    
    packet *recv_packet;
    queue *send_queue;
} connection;

connection *init_connection(int cfd);
void close_connection(connection *conn);

int recv_packet(connection *conn);
int send_packet(connection *conn);
int connection_timeout(const connection *conn);
int connection_set_timer(const connection *conn);

int send_queue_empty(const connection *conn);
packet *send_queue_top(const connection *conn);
void send_queue_push(connection *conn, packet *item);
void send_queue_pop(connection *conn);

#ifdef __cplusplus
}
#endif

#endif  /* _CONNECTION_H_ */