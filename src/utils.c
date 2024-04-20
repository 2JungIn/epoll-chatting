#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int range_random(const int from, const int to)
{
    if (from > to)
        return -1;
    /**
     * rand() % (b - a + 1) -> 0 ~ (b-a) ... (1)
     * (1) + a -> a ~ b
    **/
    return (rand() % (to - from + 1)) + from;
}


void unix_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void print_byte(const char *byte, const size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        printf("%02x", (unsigned char)byte[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
        else if ((i + 1) % 8 == 0)
            printf("    ");
        else
            printf(" ");
    }

    printf("\n");
}

char *date(void)
{
    static char buffer[BUF_SIZE];

    time_t now = time(NULL);
    struct tm *tmp = NULL;
    if ((tmp = localtime(&now)) == NULL) /* error: localtime() */
    {
        perror("localtime");
        return NULL;
    }
    
    if (strftime(buffer, sizeof(buffer), "%Y. %m. %d. (%a) %T %Z(GMT%z)", tmp) == 0)  /* error: strftime() */
    {
        fprintf(stderr, "strftime() returned 0!\n");
        return NULL;
    }

    return buffer;
}

/* dummy client */
struct message *dummy_message(void)
{
    int msg_length = range_random(1, MAX_MSG_LENGTH);

    size_t allocate_size = sizeof(struct message) + msg_length;
    struct message *m = (struct message *)malloc(allocate_size);

    m->msg_length = msg_length;
    int i;
    for (i = 0; i < msg_length - 1; i++)
    {
        if (rand() & 1) /* rnad() % 2 == 1*/
            m->msg[i] = (char)range_random((int)'A', (int)'Z');
        else
            m->msg[i] = (char)range_random((int)'a', (int)'z');
    }
    m->msg[i] = '\0';

    return m;
}
