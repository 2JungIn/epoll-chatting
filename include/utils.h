#ifndef _PRINT_H_
#define _PRINT_H_

#ifdef _STDIO_H

#define print_msg(stream, arg...)  do {    \
    flockfile(stream);  \
    fprintf(stream, "[%s/%s:%d] ", __FILE__, __FUNCTION__, __LINE__);  \
    fprintf(stream, arg);   \
    fputc('\n', stream);    \
    funlockfile(stream);    \
} while (0)

#define pr_err(msg...) print_msg(stderr, msg)
#define pr_out(msg...) print_msg(stdout, msg)
#endif

#include <stddef.h>
#include "protocol.h"

#define BUF_SIZE      50    /* date string max size */

void unix_error(const char *msg);
void print_byte(const char *byte, const size_t n);
char *date(void);

/* dummy client */
struct message *dummy_message(void);

#endif /* _PRINT_H_ */
