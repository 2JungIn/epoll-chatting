#include "../include/queue.h"
#include <stddef.h>

queue *init_queue(void (*free_item)(void *))
{
    return init_list(free_item, NULL);
}

void destroy_queue(queue *q)
{
    destroy_list(q);
}


int queue_empty(const queue *q)
{
    return list_empty(q);
}

int queue_size(const queue *q)
{
    return list_size(q);
}


void *queue_front(const queue *q)
{
    return list_front(q);
}

void queue_enqueue(queue *q, void *item)
{
    list_insert_back(q, item);
}

void *queue_dequeue(queue *q)
{
    return list_delete_front(q);
}

void queue_clear(queue *q)
{
    list_clear(q);
}
