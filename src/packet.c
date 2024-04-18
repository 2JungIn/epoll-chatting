#include "../include/packet.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


packet *packet_duplicate(const packet *p)
{
    assert(p);   /* error: invalid arg */
    
    size_t allocate_size = sizeof(struct packet) + p->data_size;
    struct packet *new_packet = (struct packet *)malloc(allocate_size);
    assert(new_packet); /* error: malloc() */

    return (struct packet *)memcpy(new_packet, p, allocate_size);
}

packet *make_packet(const struct header *h)
{
    size_t data_size = sizeof(struct header) + h->size;
    size_t allocate_size = sizeof(packet) + data_size;
    packet *new_packet = (packet *)malloc(allocate_size);
    assert(new_packet);

    new_packet->offset = sizeof(struct header);
    new_packet->data_size = data_size;
    char *raw_data = new_packet->data;

    memcpy(raw_data, h, sizeof(struct header));

    return new_packet;
}


packet *heartbeat_packet(const char *data, const uint32_t length)
{
    struct header h = { .type = HEARTBEAT, .size = sizeof(struct message) + length };

    packet *new_packet = make_packet(&h);
    new_packet->offset = 0;
    char *raw_data = new_packet->data;

    struct message *m = (struct message *)(raw_data + sizeof(struct header));
    m->msg_length = length;
    memcpy(m->msg, data, length);

    return new_packet;
}

packet *msg_packet(const char *msg, const uint32_t length)
{
    struct header h = { .type = CHAT_MSG, .size = sizeof(struct message) + length };

    packet *new_packet = make_packet(&h);
    new_packet->offset = 0;
    char *raw_data = new_packet->data;

    struct message *m = (struct message *)(raw_data + sizeof(struct header));
    m->msg_length = length;
    memcpy(m->msg, msg, length);

    return new_packet;
}
