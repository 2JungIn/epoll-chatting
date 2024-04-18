#include "../include/list.h"
#include <stdlib.h>

struct node
{
    void *item;
    struct node *next;
};


static struct node *init_node(void *item)
{
    struct node *n = NULL;
    if ((n = (struct node *)malloc(sizeof(struct node))) == NULL)    /* error: maloc() */
        return NULL;
    
    n->item = item;
    n->next = NULL;

    return n;
}


list *init_list(void (*free_item)(void *), int (*compare)(const void *, const void *))
{
    list *l = NULL;
    if ((l = (list *)malloc(sizeof(list))) == NULL)   /* error: malloc() */
        return NULL;

    l->count = 0;
    l->head = l->tail = NULL;
    l->free_item = free_item;
    l->compare = compare;

    return l;
}

void destroy_list(list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return ;

    list_clear(l);
    free(l);
}


int list_empty(const list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return -1;

    return l->count == 0;
}

int list_size(const list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return -1;

    return l->count;
}


void *list_front(const list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;

    if (list_empty(l))
        return NULL;

    return l->head->item;
}

void *list_back(const list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;

    if (list_empty(l))
        return NULL;

    return l->tail->item;
}

void *list_at(const list *l, const int idx)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;

    if (idx < 0 || idx >= list_size(l))   /* error: list out of ange */
        return NULL;

    struct node *curr = l->head;
    for (int i = 0; i < idx; i++)
        curr = curr->next;

    return curr->item;
}


/* search */
void *list_find_item(const list *l, const void *item)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;
    
    int (*compare)(const void *, const void *) = l->compare;
    if (compare == NULL)    /* 비교 함수가 없으면 원하는 값을 찾을 수 없다. */
        return NULL;

    struct node *curr = l->head;
    while (curr && compare(curr->item, item) != 0)
        curr = curr->next;

    if (curr == NULL)    /* not found */
        return NULL;

    return curr->item;  /* found */
}

int list_find_index(const list *l, const void *item)
{
    if (l == NULL)    /* error: invalid arg */
        return -1;

    int (*compare)(const void *, const void *) = l->compare;
    if (compare == NULL)    /* 비교 함수가 없으면 원하는 값을 찾을 수 없다. */
        return -1;

    int index = 0;
    struct node *curr = l->head;
    while (curr && compare(curr->item, item) != 0)
    {
        curr = curr->next;   
        index++;
    }

    if (curr == NULL)    /* not found */
        return -1;

    return index;  /* found */
}


int list_reverse(list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return -1;

    if (l->count > 1)
    {
        struct node *prev_node = NULL;
        struct node *curr_node = l->head;
        l->tail = l->head;    /* 리스트를 뒤집으면 tail이 head가 됨 */
        while (curr_node)
        {
            struct node *temp = prev_node;
            prev_node = curr_node;
            curr_node = curr_node->next;
            prev_node->next = temp;
        }
        l->head = prev_node;     /* 리스트를 뒤집으면 head가 tail이 됨 */
    }

    return 0;
}

/* insert */
int list_insert_front(list *l, void *item)
{
    if (l == NULL)    /* error: invalid arg */
        return -1;

    if (item == NULL)    /* error: invalid arg */
        return -1;

    struct node *new_node = init_node(item);

    new_node->next = l->head;
    if (list_empty(l))
        l->tail = new_node;

    l->head = new_node;

    l->count++;

    return 0;
}

int list_insert_back(list *l, void *item)
{
    if (l == NULL)    /* error: invalid arg */
        return -1;

    if (item == NULL)    /* error: invalid arg */
        return -1;

    struct node *new_node = init_node(item);

    if (list_empty(l))
        l->head = new_node;
    else
        l->tail->next = new_node;

    l->tail = new_node;

    l->count++;

    return 0;
}

int list_insert_at(list *l, const int idx, void *item)
{
    if (l == NULL)    /* error: invalid arg */
        return -1;

    if (idx < 0 || idx >= list_size(l))    /* error: out of range */
        return -1;
    
    if (item == NULL)    /* error: invalid arg */
        return -1;

    struct node *prev_node = NULL;
    struct node *curr_node = l->head;
    for (int i = 0; i < idx; i++)
    {
        prev_node = curr_node;
        curr_node = curr_node->next;
    }
    
    if (prev_node == NULL && curr_node == l->head)
    {
        return list_insert_front(l, item);
    }
    else if (prev_node == l->tail && curr_node == NULL)
    {
        return list_insert_back(l, item);
    }
    else
    {
        struct node *new_node = init_node(item);

        new_node->next = curr_node;
        prev_node->next = new_node;

        l->count++;
    }

    return 0;
}


/* delete */
void *list_delete_front(list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;
    
    if (list_empty(l))
        return NULL;

    struct node *curr_node = l->head;

    if (curr_node == l->tail)
        l->tail = NULL;

    l->head = l->head->next;
    
    void *ret_item = NULL;

    void (*free_item)(void *) = l->free_item;
    if (free_item)
        free_item(curr_node->item);
    else
        ret_item = curr_node->item;
    free(curr_node);
    l->count--;

    return ret_item;
}

void *list_delete_back(list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;
    
    if (list_empty(l))
        return NULL;

    struct node *curr_node = l->head;
    struct node *prev_node = NULL;
    while (curr_node->next)
    {
        prev_node = curr_node;
        curr_node = curr_node->next;
    }

    if (prev_node == NULL)
        l->head = NULL;
    else
        prev_node->next = NULL;

    l->tail = prev_node;
    
    void *ret_item = NULL;

    void (*free_item)(void *) = l->free_item;
    if (free_item)
        free_item(curr_node->item);
    else
        ret_item = curr_node->item;
    free(curr_node);
    l->count--;

    return ret_item;
}

void *list_delete_at(list *l, const int idx)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;
    
    if (idx < 0 || idx >= list_size(l)) /* error: list out of ange */
        return NULL;

    struct node *curr_node = l->head;
    struct node *prev_node = NULL;
    for (int i = 0; i < idx; i++)
    {
        prev_node = curr_node;
        curr_node = curr_node->next;
    }

    if (prev_node == NULL && curr_node == l->head)
    {
        return list_delete_front(l);
    }
    else if (prev_node == l->tail && curr_node == NULL)
    {
        return list_delete_back(l);
    }
    else
    {
        prev_node->next = curr_node->next;

        void *ret_item = NULL;

        void (*free_item)(void *) = l->free_item;
        if (free_item)
            free_item(curr_node->item);
        else
            ret_item = curr_node->item;
        free(curr_node);
        l->count--;

        return ret_item;
    }

    return NULL;
}

void *list_delete(list *l, const void *item)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;

    int (*compare)(const void *, const void *) = l->compare;
    if (compare == NULL)    /* 비교 함수가 없으면 원하는 값을 찾을 수 없다. */
        return NULL;

    struct node *prev_node = NULL;
    struct node *curr_node = l->head;
    while (curr_node && compare(curr_node->item, item) != 0)
    {
        prev_node = curr_node;
        curr_node = curr_node->next;
    }

    if (curr_node == NULL)    /* not found */
        return NULL;

    if (curr_node == l->head)
    {
        return list_delete_front(l);
    }
    else if (curr_node == l->tail)
    {
        return list_delete_back(l);
    }
    else
    {
        prev_node->next = curr_node->next;
        
        void *ret_item = NULL;

        void (*free_item)(void *) = l->free_item;
        if (free_item)
            free_item(curr_node->item);
        else
            ret_item = curr_node->item;
        free(curr_node);
        l->count--;

        return ret_item;
    }

    return NULL;
}


void list_clear(list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return ;
        
    while (!list_empty(l))
        list_delete_front(l);
}


/* iterator */
iterator list_begin(const list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;

    if (list_empty(l))
        return NULL;

    return l->head;
}

iterator list_next(const iterator iter)
{
    if (iter == NULL)
        return NULL;
    
    return iter->next;
}

iterator list_end(const list *l)
{
    if (l == NULL)    /* error: invalid arg */
        return NULL;

    if (list_empty(l))
        return NULL;

    return NULL;
}

void *list_iter_get(const iterator iter)
{
    if (iter == NULL)
        return NULL;
    
    return iter->item;
}
