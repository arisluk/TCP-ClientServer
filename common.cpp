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
            fprintf(stderr, "ERROR: %s. errno %d: %s\n", message, errno, strerror(errno));
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
    _log("============================");
    _log("seq ", ntohl(pack->packet_head.sequence_number));
    _log("ack ", ntohl(pack->packet_head.ack_number));
    _log("cid ", ntohs(pack->packet_head.connection_id));
    _log("flg ", pack->packet_head.flags);
    // _log("pay ", pack->payload);
    _log("============================");
}


void output_packet(struct packet* pack, int cwnd, int ss_thresh, int type) {            
    if (type == 1) {
        // RECV" <Sequence Number> <Acknowledgement Number> <Connection ID> <CWND> <SS-THRESH> ["ACK"] ["SYN"] ["FIN"]
        std::cout << "RECV ";
        std::cout << ntohl(pack->packet_head.sequence_number) << " ";
        std::cout << ntohl(pack->packet_head.ack_number) << " ";
        std::cout << ntohs(pack->packet_head.connection_id) << " ";
        std::cout << cwnd << " ";
        std::cout << ss_thresh << " ";

        if (pack->packet_head.flags == ACK) std::cout << "ACK";
        if (pack->packet_head.flags == SYN) std::cout << "SYN";
        if (pack->packet_head.flags == FIN) std::cout << "FIN";
        if (pack->packet_head.flags == SYNACK) std::cout << "ACK SYN";
        if (pack->packet_head.flags == FINACK) std::cout << "ACK FIN";
        std::cout << std::endl;
    } else if (type == 2) {
        // "SEND" <Sequence Number> <Acknowledgement Number> <Connection ID> <CWND> <SS-THRESH> ["ACK"] ["SYN"] ["FIN"] ["DUP"]
        std::cout << "SEND ";
        std::cout << ntohl(pack->packet_head.sequence_number) << " ";
        std::cout << ntohl(pack->packet_head.ack_number) << " ";
        std::cout << ntohs(pack->packet_head.connection_id) << " ";
        std::cout << cwnd << " ";
        std::cout << ss_thresh << " ";

        if (pack->packet_head.flags == ACK) std::cout << "ACK";
        if (pack->packet_head.flags == SYN) std::cout << "SYN";
        if (pack->packet_head.flags == FIN) std::cout << "FIN";
        if (pack->packet_head.flags == SYNACK) std::cout << "ACK SYN";
        if (pack->packet_head.flags == FINACK) std::cout << "ACK FIN";
        std::cout << std::endl;
    } else if (type == 3) {
        std::cout << "SEND ";
        std::cout << ntohl(pack->packet_head.sequence_number) << " ";
        std::cout << ntohl(pack->packet_head.ack_number) << " ";
        std::cout << ntohs(pack->packet_head.connection_id) << " ";
        std::cout << cwnd << " ";
        std::cout << ss_thresh << " ";
        if (pack->packet_head.flags == ACK) std::cout << "ACK ";
        if (pack->packet_head.flags == SYN) std::cout << "SYN ";
        if (pack->packet_head.flags == FIN) std::cout << "FIN ";
        if (pack->packet_head.flags == SYNACK) std::cout << "ACK SYN";
        if (pack->packet_head.flags == FINACK) std::cout << "ACK FIN";
        std::cout << "DUP";
        std::cout << std::endl;
    } else if (type == 4) {
        // "DROP" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"]
        std::cout << "DROP ";
        std::cout << ntohl(pack->packet_head.sequence_number) << " ";
        std::cout << ntohl(pack->packet_head.ack_number) << " ";
        std::cout << ntohs(pack->packet_head.connection_id) << " ";

        if (pack->packet_head.flags == ACK) std::cout << "ACK ";
        if (pack->packet_head.flags == SYN) std::cout << "SYN ";
        if (pack->packet_head.flags == FIN) std::cout << "FIN ";
        if (pack->packet_head.flags == SYNACK) std::cout << "ACK SYN";
        if (pack->packet_head.flags == FINACK) std::cout << "ACK FIN";
        std::cout << std::endl;
    }
}

void output_packet_server(struct packet* pack, int type) {            
    if (type == TYPE_RECV) {
        // "RECV" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"]
        std::cout << "RECV ";
        std::cout << ntohl(pack->packet_head.sequence_number) << " ";
        std::cout << ntohl(pack->packet_head.ack_number) << " ";
        std::cout << ntohs(pack->packet_head.connection_id) << " ";

        if (pack->packet_head.flags == ACK) std::cout << "ACK";
        if (pack->packet_head.flags == SYN) std::cout << "SYN";
        if (pack->packet_head.flags == FIN) std::cout << "FIN";
        if (pack->packet_head.flags == SYNACK) std::cout << "ACK SYN";
        if (pack->packet_head.flags == FINACK) std::cout << "ACK FIN";
        std::cout << std::endl;
    } else if (type == TYPE_SEND) {
        // "SEND" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"] ["DUP"]
        std::cout << "SEND ";
        std::cout << ntohl(pack->packet_head.sequence_number) << " ";
        std::cout << ntohl(pack->packet_head.ack_number) << " ";
        std::cout << ntohs(pack->packet_head.connection_id) << " ";

        if (pack->packet_head.flags == ACK) std::cout << "ACK";
        if (pack->packet_head.flags == SYN) std::cout << "SYN";
        if (pack->packet_head.flags == FIN) std::cout << "FIN";
        if (pack->packet_head.flags == SYNACK) std::cout << "ACK SYN";
        if (pack->packet_head.flags == FINACK) std::cout << "ACK FIN";
        std::cout << std::endl;
    } else if (type == TYPE_DUP) {
        // "SEND" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"] ["DUP"]
        std::cout << "SEND ";
        std::cout << ntohl(pack->packet_head.sequence_number) << " ";
        std::cout << ntohl(pack->packet_head.ack_number) << " ";
        std::cout << ntohs(pack->packet_head.connection_id) << " ";

        if (pack->packet_head.flags == ACK) std::cout << "ACK ";
        if (pack->packet_head.flags == SYN) std::cout << "SYN ";
        if (pack->packet_head.flags == FIN) std::cout << "FIN ";
        if (pack->packet_head.flags == SYNACK) std::cout << "ACK SYN";
        if (pack->packet_head.flags == FINACK) std::cout << "ACK FIN";
        std::cout << "DUP";
        std::cout << std::endl;
    } else if (type == TYPE_DROP) {
        // "DROP" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"]
        std::cout << "DROP ";
        std::cout << ntohl(pack->packet_head.sequence_number) << " ";
        std::cout << ntohl(pack->packet_head.ack_number) << " ";
        std::cout << ntohs(pack->packet_head.connection_id) << " ";

        if (pack->packet_head.flags == ACK) std::cout << "ACK ";
        if (pack->packet_head.flags == SYN) std::cout << "SYN ";
        if (pack->packet_head.flags == FIN) std::cout << "FIN ";
        if (pack->packet_head.flags == SYNACK) std::cout << "ACK SYN";
        if (pack->packet_head.flags == FINACK) std::cout << "ACK FIN";
        std::cout << std::endl;
    }
}

Store::Store() {
    seq = 0;
    ack = 0;
    last_time = 0;
    writefd = 0;
    state = 0;
}

Store::Store(uint32_t sq, uint32_t ak, uint64_t lte, int wfd, int s) {
    seq = sq;
    ack = ak;
    last_time = lte;
    writefd = wfd;
    state = s;
}