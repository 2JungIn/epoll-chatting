#include "../include/worker_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

worker_queue *init_worker_queue(void)
{
    worker_queue *wq = (worker_queue *)malloc(sizeof(worker_queue));
    assert(wq);     /* error: malloc() */

    wq->l = init_list(NULL, NULL);
    assert(wq->l);  /* error: init_list() */

    int rc;
    if ((rc = pthread_mutex_init(&wq->mutex, NULL)))
    {
        fprintf(stderr, "pthread_mutex_init: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }
    if ((rc = pthread_cond_init(&wq->cond, NULL)))
    {
        fprintf(stderr, "pthread_cond_init: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    return wq;
}

int destroy_worker_queue(worker_queue *wq, void (*free_item)(void *))
{
    assert(wq);         /* error: invalid arg */
    assert(free_item);  /* error: invalid arg */

    int rc;
    if ((rc = pthread_mutex_lock(&wq->mutex)))
    {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(rc));
        return -1;
    }
    
    wq->l->free_item = free_item;
    destroy_list(wq->l);

    if ((rc = pthread_mutex_unlock(&wq->mutex)))
    {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(rc));
        return -1;
    }

    if ((rc = pthread_mutex_destroy(&wq->mutex)))
    {
        fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(rc));
        return -1;
    }
    if ((rc = pthread_cond_destroy(&wq->cond)))
    {
        fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(rc));
        return -1;
    }

    free(wq);

    return 0;
}


int worker_queue_push(worker_queue *wq, packet *item)
{
    assert(wq);     /* error: invalid arg */
    assert(item);   /* error: invalid arg */

    int rc;
    int ret_val;
    if ((rc = pthread_mutex_lock(&wq->mutex)))
    {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(rc));
        return -1;
    }
    
    ret_val = list_insert_back(wq->l, item);

    if ((rc = pthread_mutex_unlock(&wq->mutex)))
    {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(rc));
        return -1;
    }

    if ((rc = pthread_cond_signal(&wq->cond)))
    {
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(rc));
        return -1;
    }

    return ret_val;
}

packet *worker_queue_pop(worker_queue *wq)
{
    assert(wq);         /* error: invalid arg */

    packet *ret_val;
    int rc;
    if ((rc = pthread_mutex_lock(&wq->mutex)))
    {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(rc));
        return NULL;
    }
    
    if (list_empty(wq->l))
    {
        if ((rc = pthread_cond_wait(&wq->cond, &wq->mutex)))
        {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(rc));
            return NULL;
        }
    }

    if (list_empty(wq->l))
        ret_val = NULL;
    else
        ret_val = list_delete_front(wq->l);

    if ((rc = pthread_mutex_unlock(&wq->mutex)))
    {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(rc));
        return NULL;
    }

    return ret_val;
}
