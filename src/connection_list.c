#include "../include/connection_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


static int compare(const connection *conn1, const connection *conn2)
{
    if (conn1 < conn2)
        return -1;
    else if (conn1 > conn2)
        return 1;
    
    return 0;   /* equal */
}


connection_list *init_connection_list(void (*free_item)(void *)){
    assert(free_item);  /* error: invalid arg */

    return init_list(free_item, (int (*)(const void *, const void *))compare);
}

void destroy_connection_list(connection_list *conn_list)
{
    destroy_list(conn_list);
}


int connection_list_insert(connection_list *conn_list, connection *conn)
{
    return list_insert_back(conn_list, conn);
}

void *connection_list_delete(connection_list *conn_list, connection *conn)
{
    return list_delete(conn_list, conn);
}


/* iterator */
iterator connection_list_begin(const connection_list *conn_list)
{
    assert(conn_list);  /* error: invalid arg */

    return list_begin(conn_list);
}

iterator connection_list_next(const iterator iter)
{
    return list_next(iter);
}

iterator connection_list_end(const connection_list *conn_list)
{
    assert(conn_list);  /* error: invalid arg */

    return list_end(conn_list);
}

connection *connection_list_iter_get(const iterator iter)
{
    return (connection *)list_iter_get(iter);
}
