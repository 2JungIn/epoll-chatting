#ifndef _CONNECTION_LIST_H_
#define _CONNECTION_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "connection.h"
#include "list.h"

typedef list connection_list;

connection_list *init_connection_list(void (*free_item)(void *));
void destroy_connection_list(connection_list *conn_list);

int connection_list_insert(connection_list *conn_list, connection *conn);
void *connection_list_delete(connection_list *conn_list, connection *conn);

/* iterator */
iterator connection_list_begin(const connection_list *conn_list);
iterator connection_list_next(const iterator iter);
iterator connection_list_end(const connection_list *conn_list);
connection *connection_list_iter_get(const iterator iter);

#ifdef __cplusplus
}
#endif

#endif  /* _CONNECTION_LIST_H_ */