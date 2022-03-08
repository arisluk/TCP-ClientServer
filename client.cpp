// ========================================================================== //
// INCLUDES
// ========================================================================== //

// Standard Libraries
#include <fcntl.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <tuple>

// C libraries
#include <cerrno>
#include <csignal>
#include <cstring>

// Networking
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/time.h>

// Filesystem

// Local
#include "common.h"

// ========================================================================== //
// DEFINITIONS
// ========================================================================== //

#ifdef DEBUG
#define OPT_LOG 1
#else
#define OPT_LOG 0
#endif

using namespace std;

std::queue<uint32_t> cwnd_q;
std::queue<int> paysize_q;

int cwnd     = SPEC_INIT_CWND;
int ssthresh = SPEC_INIT_SS_THRESH;

// ========================================================================== //
// FUNCTIONS
// ========================================================================== //
void update_cwnd_ssthresh() {
    if (cwnd < ssthresh) {
        cwnd += SPEC_INIT_CWND;
    } else {
        cwnd += SPEC_INIT_CWND * SPEC_INIT_CWND / cwnd;
    }
    if (cwnd > SPEC_MAX_CWND) {
        cwnd = SPEC_MAX_CWND;
    }
}

void on_timeout() {
    ssthresh = cwnd / 2;
    cwnd     = SPEC_INIT_CWND;
}

void sig_handle(int sig) {
    if (sig == SIGTERM || sig == SIGQUIT) exit(0);
    exit(sig);
}

std::tuple<int, struct addrinfo*> open_socket(const char* hostname, int port) {
    // https://man7.org/linux/man-pages/man3/getaddrinfo.3.html

    auto port_name = std::to_string(port);
    _log("SOCKET SETUP: Port name: ", port_name, "\n");

    struct addrinfo hints, *server_info, *p;

    int socket_fd;
    int rc;

    // Clear structs
    memset(&hints, 0, sizeof(hints));

    hints.ai_family   = AF_UNSPEC;   // SUPPORT IPV6
    hints.ai_socktype = SOCK_DGRAM;  // SOCK_STREAM - TCP, SOCK_DGRAM -> UDP
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_UDP;

    rc = getaddrinfo(hostname, port_name.c_str(), &hints, &server_info);
    if (rc != 0) {
        _log("SOCKET SETUP: Error getting address info", strerror(errno));
        _exit("Getting address info, maybe incorrect hostname or port?", errno);
    }
    _log("SOCKET SETUP: getaddrinfo success.\n");

    for (p = server_info; p != NULL; p = p->ai_next) {
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_fd == -1) {
            _log("SOCKET: skipped a socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        _log("SOCKET: failed to create socket");
        _exit("Failed to bind to socket", 2);
    } else {
        _log("SOCKET: successfully created socket");
    }

    freeaddrinfo(server_info);

    _log("SOCKET: ready to send");

    return std::make_tuple(socket_fd, p);
}

int handshake(int socket_fd, struct sockaddr* addr, socklen_t size, uint32_t* seq_num, uint32_t* ack_num, uint16_t* cid) {
    *seq_num = 12345;
    *ack_num = 0;

    packet syn;
    memset(&syn, 0, sizeof(struct packet));
    syn.packet_head.sequence_number = htonl(*seq_num);
    syn.packet_head.ack_number      = htonl(*ack_num);
    syn.packet_head.connection_id   = htons(0);
    syn.packet_head.flags           = SYN;

    int numbytes = 0;
    numbytes     = sendto(socket_fd, &syn, 12, 0, addr, size);
    err(numbytes, "Sending SYN");
    _log("handshake syn talker: sent ", numbytes, " bytes");
    _log("SENT SYN PACKET:");
    printpacket(&syn);
    output_packet(&syn, cwnd, ssthresh, TYPE_SEND);

    packet syn_ack;
    memset(&syn_ack, 0, sizeof(struct packet));
    int rc = 0;
    rc     = recvfrom(socket_fd, &syn_ack, 12, 0, NULL, 0);
    _log("RECV returned: ", rc);
    err(rc, "HANDSHAKE while recv from socket");

    if (syn_ack.packet_head.flags != SYNACK) {
        _exit("BAD SYNACK RECEIVED");
    }

    *cid     = ntohs(syn_ack.packet_head.connection_id);
    *seq_num = ntohl(syn_ack.packet_head.ack_number);
    *ack_num = ntohl(syn_ack.packet_head.sequence_number) + 1;
    _log("RCV SYNACK PACKET:");
    printpacket(&syn_ack);
    output_packet(&syn_ack, cwnd, ssthresh, TYPE_RECV);

    // packet ack;
    // memset(&ack, 0, sizeof(struct packet));
    // ack.packet_head.sequence_number = htonl(*seq_num);
    // ack.packet_head.ack_number      = htonl(*ack_num);
    // ack.packet_head.connection_id   = htons(*cid);
    // ack.packet_head.flags           = ACK;

    // int numbytes2 = 0;
    // numbytes2     = sendto(socket_fd, &ack, 12, 0, addr, size);
    // err(numbytes2, "Sending handshake ACK");
    // _log("talker: sent ", numbytes, " bytes");
    // _log("SENT ACK PACKET:");
    // printpacket(&ack);
    // output_packet(&ack, cwnd, ssthresh, TYPE_SEND);

    return 0;
}

int main(int argc, char** argv) {
    int OPT_PORT = 0;
    std::string OPT_HOST;
    std::string OPT_DIR;

    signal(SIGQUIT, sig_handle);
    signal(SIGTERM, sig_handle);

    // FD_ZERO(&rfds);
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = SPEC_RTO_MS * 1000;

    if (argc != 4)
        _exit("Invalid arguments.\n usage: \"./client <HOSTNAME-OR-IP> <PORT> <FILENAME>\"");

    _log("Logging enabled.");

    try {
        OPT_HOST = argv[1];
        OPT_PORT = std::stoi(argv[2]);
        OPT_DIR  = argv[3];
        if (OPT_PORT < 0 || OPT_PORT > 65535) throw std::invalid_argument("Invalid Port");
    } catch (const std::exception& e) {
        std::cerr << "Error in: " << e.what() << std::endl;
        _exit("Invalid arguments.\nusage: \"./server <PORT> <FILE-DIR>\"", 1);
    }

    _log(OPT_HOST, "|", OPT_PORT, "|", OPT_DIR);

    int socket_fd;
    struct addrinfo* p;
    std::tie(socket_fd, p) = open_socket(OPT_HOST.c_str(), OPT_PORT);

    // FD_SET(socket_fd, &rfds)
    // udp socket is open and ready to send

    // example send
    // just change second and third fields to change data sent
    // int numbytes = 0;
    // numbytes     = sendto(socket_fd, argv[3], strlen(argv[3]), 0, p->ai_addr, p->ai_addrlen);
    // err(numbytes, "Sending message");
    // _log("talker: sent ", numbytes, " bytes to ", argv[1]);

    /*

        client logic here
    */

    uint32_t seq_num;
    uint32_t ack_num = 0;
    uint16_t cid = 0;
    int amt_sent = 0;
    bool done = false;
    bool truedone = false;

    // try handshake
    handshake(socket_fd, p->ai_addr, p->ai_addrlen, &seq_num, &ack_num, &cid);

    int file_fd = open(argv[3], O_RDONLY);
    err(file_fd, "Opening file");

    seq_num = seq_num;

    struct timeval socket_timeout;
    socket_timeout.tv_sec  = 0;
    socket_timeout.tv_usec = 5000;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &socket_timeout, sizeof(socket_timeout));

    while (truedone == false) {
        packet curr_pack;
        memset(&curr_pack, 0, sizeof(struct packet));
        packet rcv_ack;
        memset(&rcv_ack, 0, sizeof(struct packet));

        int rc = 0;
        if (amt_sent > 0) {
            rc = recvfrom(socket_fd, &rcv_ack, 12, 0, NULL, 0);
            _log("RECV returned, ", rc);
            if (rc > 0) {
                _log("ACK FROM PACK = ", ntohl(rcv_ack.packet_head.ack_number), " front ", cwnd_q.front());
                if (ntohl(rcv_ack.packet_head.ack_number) == cwnd_q.front()) {
                    cwnd_q.pop();
                    amt_sent -= paysize_q.front();
                    paysize_q.pop();
                }
                _log("RCV ACK PACKET:");
                printpacket(&rcv_ack);
                output_packet(&rcv_ack, cwnd, ssthresh, TYPE_RECV);
                update_cwnd_ssthresh();
            }
            if (cwnd_q.size() == 0 && done) {
                truedone = true;
            }
        }

        if (amt_sent <= cwnd) {
            int readLen = read(file_fd, curr_pack.payload, SPEC_MAX_PAYLOAD_SIZE);
            if (readLen < SPEC_MAX_PAYLOAD_SIZE) {
                done = true;
            }
            if (readLen == 0) {
                continue;
            }
            curr_pack.packet_head.sequence_number = htonl(seq_num);
            curr_pack.packet_head.ack_number      = htonl(ack_num);
            curr_pack.packet_head.connection_id   = htons(cid);
            curr_pack.packet_head.flags           = ACK;

            int numbytes = 0;
            numbytes     = sendto(socket_fd, &curr_pack, 12 + readLen, 0, p->ai_addr, p->ai_addrlen);
            err(numbytes, "Sending payload");
            _log("amt_sent talker: sent ", numbytes, " bytes");
            _log("SENT payload PACKET:");
            printpacket(&curr_pack);
            output_packet(&curr_pack, cwnd, ssthresh, TYPE_SEND);
            seq_num += readLen;
            seq_num %= SPEC_MAX_SEQ + 1;
            cwnd_q.push(seq_num);
            _log(seq_num);
            paysize_q.push(readLen);
            amt_sent += readLen;
        }
    }

    packet finpack;
    memset(&finpack, 0, sizeof(struct packet));
    finpack.packet_head.flags           = FIN;
    finpack.packet_head.connection_id   = htons(cid);
    finpack.packet_head.sequence_number = htonl(seq_num);
    finpack.packet_head.ack_number      = htonl(0);

    int numbytes = 0;
    numbytes     = sendto(socket_fd, &finpack, 12, 0, p->ai_addr, p->ai_addrlen);
    err(numbytes, "Sending FIN");
    _log("fin talker: sent ", numbytes, " bytes");
    _log("SENT FIN PACKET:");
    printpacket(&finpack);
    output_packet(&finpack, cwnd, ssthresh, TYPE_SEND);
    

    struct timeval time_val_struct;
    time_val_struct.tv_sec = 0;
    time_val_struct.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &time_val_struct, sizeof(time_val_struct));

    packet finack;
    memset(&finack, 0, sizeof(struct packet));
    int rc = 0;
    rc = recvfrom(socket_fd, &finack, 12, 0, NULL, 0);
    err(rc, "CLIENT END while recvfrom socket");
    _log("RCV FINACK PACKET:");
    printpacket(&finack);
    output_packet(&finack, cwnd, ssthresh, TYPE_RECV);

    bool waiting = true;

    packet leftover_fin;
    memset(&leftover_fin, 0, sizeof(struct packet));

    struct timeval waiting_room;
    struct timeval waiting_room2;
    gettimeofday(&waiting_room, NULL);

    uint32_t start_time = 1000000 * waiting_room.tv_sec + waiting_room.tv_usec;
    uint32_t curr_time;
    seq_num++;
    int newrc = 0;

    while (waiting) {
        gettimeofday(&waiting_room, NULL);
        curr_time = 1000000 * waiting_room.tv_sec + waiting_room.tv_usec;
        uint32_t time_diff = 2000000 - (curr_time - start_time);
        if (time_diff > 2000000) {
            waiting = false;
        }
        waiting_room2.tv_sec = time_diff/1000000;
        waiting_room2.tv_usec = time_diff%1000000;

        memset(&leftover_fin, 0, sizeof(struct packet));
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &waiting_room2, sizeof(waiting_room2));
        newrc = recvfrom(socket_fd, &leftover_fin, 12, 0, NULL, 0);
        err(newrc, "WAITING ROOM while recvfrom socket");
        _log("RCV LEFTOVER FIN PACKET:");
        printpacket(&leftover_fin);
        if (newrc > 0 && leftover_fin.packet_head.flags == FIN) {
            output_packet(&leftover_fin, cwnd, ssthresh, TYPE_RECV);
            packet newack;
            memset(&newack, 0, sizeof(struct packet));
            newack.packet_head.sequence_number = htonl(seq_num);
            newack.packet_head.flags = ACK;
            newack.packet_head.connection_id = htons(cid);
            newack.packet_head.ack_number = htonl(ntohl(leftover_fin.packet_head.sequence_number)+1);

            int newnumbytes = 0;
            newnumbytes     = sendto(socket_fd, &newack, 12, 0, p->ai_addr, p->ai_addrlen);
            err(newnumbytes, "Sending LEFTOVER ACK");
            _log("LEFTOVER ACK talker: sent ", newnumbytes, " bytes");
            _log("SENT LEFTOVER ACK PACKET:");
            printpacket(&newack);
            output_packet(&newack, cwnd, ssthresh, TYPE_SEND);
        }
    }

    shutdown(socket_fd, 2);

    return 0;
}
