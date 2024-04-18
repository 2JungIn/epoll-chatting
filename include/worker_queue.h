#ifndef _WORKER_QUEUE_H_
#define _WORKER_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "packet.h"
#include "list.h"
#include <pthread.h>

typedef struct worker_queue
{
    list *l;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} worker_queue;

worker_queue *init_worker_queue(void);
int destroy_worker_queue(worker_queue *wq, void (*free_item)(void *));


int worker_queue_push(worker_queue *wq, packet *item);
packet *worker_queue_pop(worker_queue *wq);

#ifdef __cplusplus
}
#endif

#endif  /* _WORKER_QUEUE_H_ */