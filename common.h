#ifndef COMMON
#define COMMON
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <stdio.h>

#include <cerrno>
#include <cstring>

#include <chrono>
#include <iostream>

#ifdef DEBUG
#define OPT_LOG 1
#else
#define OPT_LOG 0
#endif

/*
  0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Sequence Number                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Acknowledgment Number                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |         Connection ID         |         Not Used        |A|S|F|
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

#define SPEC_MAX_PACKET_SIZE 524
#define SPEC_MAX_PAYLOAD_SIZE 512
#define SPEC_MAX_SEQ 102400
#define SPEC_MAX_ACK 102400
#define SPEC_RTO_MS 500
#define SPEC_INIT_CWND 512
#define SPEC_MAX_CWND 51200
#define SPEC_RWND 51200
#define SPEC_INIT_SS_THRESH 10000

#define SYN 2 // ...010
#define ACK 4 // ...100
#define FIN 1 // ...001
#define SYNACK 6 // ...110
#define FINACK 5 // ...101

#define TYPE_RECV 1
#define TYPE_SEND 2
#define TYPE_DUP 3
#define TYPE_DROP 4

#define STATE_ACTIVE 1
#define STATE_FIN 2

#define PACKET_FROM_REC 0
#define PACKET_FROM_BUFFER 1
#define PACKET_LAST_FROM_BUFFER 2

#pragma pack(1)
struct header {
    uint32_t sequence_number;
    uint32_t ack_number;
    uint16_t connection_id;
    uint8_t empty;
    uint8_t flags;
};
typedef struct header header;

struct packet {
    header packet_head;
    char payload[SPEC_MAX_PAYLOAD_SIZE];
};
typedef struct packet packet;

void printpacket(struct packet*);

void output_packet(struct packet*, int cwnd, int ss_thresh, int type);

void output_packet_server(struct packet*, int type);

uint64_t time_now_ms();

// set header flags given ACK, SYN, FIN
// uint8_t set_flags(uint8_t& flag, bool ACK, bool SYN, bool FIN);

// exit with error if rc < 0, with message and optional exit code
bool err(int rc, const char* message, int exit_code = -1);

// exit with error message and optional exit code
void _exit(const char* message, int exit_code = 1);

// log if built with "make debug" instead of plain "make server" or "make client"
template <typename... Args>
inline void _log(Args&&... args) {
    #ifdef DEBUG
    std::cerr << "DEBUG: ";
    (std::cerr << ... << args);
    std::cerr << std::endl;
    #endif
    return;
}

class Store {
    public:
        Store();    
        Store(uint32_t seq, uint32_t ack, uint64_t last_time, FILE * writefd, int state);
        uint32_t seq; // what we are expecting, latest in-order seq + 512
        uint32_t ack; // last packet by us that client ACKed
        uint64_t last_time;
        FILE * writefd;
        int state;
};

#endif