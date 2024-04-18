#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"

typedef list queue;

queue *init_queue(void (*free_item)(void *));
void destroy_queue(queue *q);

int queue_empty(const queue *q);
int queue_size(const queue *q);

void *queue_front(const queue *q);
void queue_enqueue(queue *q, void *item);
void *queue_dequeue(queue *q);
void queue_clear(queue *q);

#ifdef __cplusplus
}
#endif

#endif  /* _QUEUE_H_ */