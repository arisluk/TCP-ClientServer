#ifndef COMMON
#define COMMON
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <cerrno>
#include <cstring>
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

#pragma pack(1)
struct header {
    uint32_t sequence_number;
    uint32_t ack_number;
    uint16_t connection_id;
    uint8_t empty;
    uint8_t flags;
};

uint16_t set_flags(uint8_t& flag, bool ACK, bool SYN, bool FIN);

bool err(int rc, const char* message, int exit_code = -1);
void _exit(const char* message, int exit_code = 1);

template <typename... Args>
void _log(Args&&... args) {
    if (OPT_LOG) {
        std::cerr << "SERVER DEBUG: ";
        (std::cerr << ... << args);
        std::cerr << std::endl;
    }
}
#endif