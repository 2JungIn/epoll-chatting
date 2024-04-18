#ifndef _PACKET_H_
#define _PACKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/protocol.h"
#include <stddef.h>

typedef struct packet
{
    size_t offset;     /* 패킷의 오프셋 offset == packet_size -> 수신 완료! */
    size_t data_size;  /* 패킷 크기 (헤더 + 패이로드) */
    char data[];       /* 패킷 (헤더 + 패이로드) */
} packet;


packet *packet_duplicate(const packet *p);
packet *make_packet(const struct header *h);

packet *heartbeat_packet(const char *data, const uint32_t length);
packet *msg_packet(const char *msg, const uint32_t length);

#ifdef __cplusplus
}
#endif

#endif  /* _PACKET_H_ */