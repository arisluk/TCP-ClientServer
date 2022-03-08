#include "common.h"

// #define ACK_MASK 0b00000100
// #define SYN_MASK 0b00000010
// #define FIN_MASK 0b00000001

// uint8_t set_flags(uint8_t& flag, bool ACK, bool SYN, bool FIN) {
//     uint8_t copy = 0;
//     flag         = ACK ? flag | ACK_MASK : flag & ~ACK_MASK;
//     flag         = SYN ? flag | SYN_MASK : flag & ~SYN_MASK;
//     flag         = FIN ? flag | ACK_MASK : flag & ~FIN_MASK;
//     copy         = ACK ? copy | ACK_MASK : copy & ~ACK_MASK;
//     copy         = SYN ? copy | SYN_MASK : copy & ~SYN_MASK;
//     copy         = FIN ? copy | ACK_MASK : copy & ~FIN_MASK;
//     return copy;
// }

bool err(int rc, const char* message, int exit_code) {
    if (rc < 0) {
        if (message)
            fprintf(stderr, "ERROR: %s in server. errno %d: %s\n", message, errno, strerror(errno));
        else
            fprintf(stderr, "ERROR: Unspecified error. errno %d: %s\n", errno, strerror(errno));
        if (exit_code == -1)
            exit(errno);
        else
            exit(exit_code);
        return false;
    }
    return true;
}

void _exit(const char* message, int exit_code) {
    if (message)
        fprintf(stderr, "ERROR: %s.\n", message);
    else
        fprintf(stderr, "ERROR: Unspecified error.\n");
    exit(exit_code);
}

void printpacket(struct packet* pack) {
    _log("seq ", ntohl(pack->packet_head.sequence_number));
    _log("ack ", ntohl(pack->packet_head.ack_number));
    _log("cid ", ntohs(pack->packet_head.connection_id));
    _log("flg ", pack->packet_head.flags);
    _log("pay ", pack->payload);
}