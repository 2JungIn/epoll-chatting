#include "../include/connection.h"
#include "../include/timer.h"
#include "../include/network.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <unistd.h>


connection *init_connection(int cfd)
{
    connection *conn = (connection *)malloc(sizeof(connection));
    assert(conn);

    conn->socket_fd = cfd;
    conn->timer_fd = make_timerfd();

    conn->recv_packet = NULL;
    conn->send_queue = init_queue(free);

    /* set client socket non-bloking mode */
    fcntl_setnb(conn->socket_fd);

    set_timer(conn->timer_fd);

    return conn;
}

void close_connection(connection *conn)
{    
    setsockopt_linger(conn->socket_fd);

    /* close socket descriptor */
    if (close(conn->socket_fd) < 0)
        perror("close");

    /* close timer descriptor */
    if (close(conn->timer_fd) < 0)
        perror("close");

    /* resource free */
    if (conn->recv_packet)
        free(conn->recv_packet);

    destroy_queue(conn->send_queue);

    free(conn);
}



int recv_packet(connection *conn)
{
    int n_recv;

    if (conn->recv_packet == NULL)  /* new packet */
    {
        struct header h;
        if ((n_recv = recv_data(conn->socket_fd, &h, sizeof(h), 0)) > 0)
        {
            /* recv buffer init */
            conn->recv_packet = make_packet(&h);
        }
    }
    else /* resume */
    {
        size_t offset = conn->recv_packet->offset;
        char *data = conn->recv_packet->data + offset;
        size_t n_size = conn->recv_packet->data_size - offset;

        if ((n_recv = recv_data(conn->socket_fd, data, n_size, 0)) > 0)
        {
            /* offset update */
            conn->recv_packet->offset += n_recv;
        }
    }

    return n_recv;
}

int send_packet(connection *conn)
{
    int n_send;

    packet *send_packet = send_queue_top(conn);
    
    size_t offset = send_packet->offset;
    char *data = send_packet->data + offset;
    size_t n_size = send_packet->data_size - offset;

    if ((n_send = send_data(conn->socket_fd, data, n_size, 0)) > 0)
    {
        send_packet->offset += n_send;

        if (send_packet->offset == send_packet->data_size)
            send_queue_pop(conn);
    }

    return n_send;
}

int connection_timeout(const connection *conn)
{
    return timeout(conn->timer_fd);
}

int connection_set_timer(const connection *conn)
{
    return set_timer(conn->timer_fd);
}


int send_queue_empty(const connection *conn)
{
    return queue_empty(conn->send_queue);
}

packet *send_queue_top(const connection *conn)
{
    return (packet *)queue_front(conn->send_queue);
}

void send_queue_push(connection *conn, packet *item)
{
    queue_enqueue(conn->send_queue, item);
}

void send_queue_pop(connection *conn)
{
    queue_dequeue(conn->send_queue);
}
