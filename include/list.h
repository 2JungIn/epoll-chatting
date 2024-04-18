#ifndef _LIST_H_
#define _LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list
{
    int count;
    
    struct node *head;
    struct node *tail;

    void (*free_item)(void *);
    int (*compare)(const void *, const void *);
} list;
typedef struct node *iterator;


list *init_list(void (*free_item)(void *), int (*compare)(const void *, const void *));
void destroy_list(list *l);


int list_empty(const list *l);
int list_size(const list *l);


void *list_front(const list *l);
void *list_back(const list *l);
void *list_at(const list *l, const int idx);

/* search */
void *list_find_item(const list *l, const void *item);
int list_find_index(const list *l, const void *item);

int list_reverse(list *l);

/* insert */
int list_insert_front(list *l, void *item);
int list_insert_back(list *l, void *item);
int list_insert_at(list *l, const int idx, void *item);

/* delete */
void *list_delete_front(list *l);
void *list_delete_back(list *l);
void *list_delete_at(list *l, const int idx);
void *list_delete(list *l, const void *item);

void list_clear(list *l);

/* iterator */
iterator list_begin(const list *l);
iterator list_next(const iterator iter);
iterator list_end(const list *l);
void *list_iter_get(const iterator iter);

#ifdef __cplusplus
}
#endif

#endif  /* _LIST_H_ */