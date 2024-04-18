#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
