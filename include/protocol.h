#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdint.h>

#define MAX_MSG_LENGTH 4096


enum header_type
{
    HEARTBEAT,  /* 하트비트: 송신 시간을 보냄 */
    CHAT_MSG,   /* C -> S, S -> C 채팅 메시지 */
};

typedef struct header
{
    uint32_t type;
    uint32_t size;
} header;

typedef struct message
{
    uint32_t msg_length;
    char msg[];
} message;

#endif  /* _PROTOCOL_H_ */