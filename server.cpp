// ========================================================================== //
// INCLUDES
// ========================================================================== //

// Standard Libraries
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <vector>

// C libraries
#include <cerrno>
#include <cstring>

// Networking
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>

// Filesystem

// Local
#include "common.h"

using namespace std;

// ========================================================================== //
// DEFINITIONS
// ========================================================================== //

// techncially this can be like 524 bc thats the max datagram size i think?

void *get_in_addr(struct sockaddr *sa) {
    return sa->sa_family == AF_INET
               ? (void *)&(((struct sockaddr_in *)sa)->sin_addr)
               : (void *)&(((struct sockaddr_in6 *)sa)->sin6_addr);
}

std::vector<uint16_t> cid_v;
std::vector<uint32_t> seq_v;
std::vector<time_t> time_v;

// ========================================================================== //
// FUNCTIONS
// ========================================================================== //

void printpacket(struct packet* pack) {
    _log("seq ", pack->packet_head.sequence_number);
    _log("ack ", pack->packet_head.ack_number);
    _log("cid ", pack->packet_head.connection_id);
    _log("flg ", pack->packet_head.flags);
    _log("pay ", pack->payload);
}

int open_socket(int port) {
    // https://man7.org/linux/man-pages/man3/getaddrinfo.3.html

    auto port_name = std::to_string(port);
    _log("SOCKET SETUP: Port name: ", port_name, "\n");

    struct addrinfo hints, *server_info, *p;

    int socket_fd = -1;
    int rc;

    // clear structs
    memset(&hints, 0, sizeof(hints));

    hints.ai_family   = AF_UNSPEC;   // SUPPORT IPV6
    hints.ai_socktype = SOCK_DGRAM;  // SOCK_STREAM - TCP, SOCK_DGRAM -> UDP
    hints.ai_flags    = AI_PASSIVE;  // ACCEPT INCOMING CONNECTIONS ON ANY "WILDCARD ADDRESSES"

    rc = getaddrinfo(NULL, port_name.c_str(), &hints, &server_info);
    if (rc != 0) {
        _log("SOCKET SETUP: Error getting address info", strerror(errno));
        _exit("Getting address info.", errno);
    }
    _log("SOCKET SETUP: getaddrinfo success.\n");

    for (p = server_info; p != NULL; p = p->ai_next) {
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_fd == -1) {
            _log("SOCKET BIND: skipped a socket");
            continue;
        }

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            shutdown(socket_fd, 2);
            _log("SOCKET BIND: failed a bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        _log("SOCKET BIND: failed to bind to any socket");
        _exit("Failed to bind to socket", 2);
    } else {
        _log("SOCKET BIND: successfully bound to a socket");
    }

    freeaddrinfo(server_info);

    _log("SOCKET: ready to recvfrom");

    return socket_fd;
}

int main(int argc, char **argv) {
    int OPT_PORT = 0;
    std::string OPT_DIR;

    int rc = 0;

    if (argc != 3)
        _exit("Invalid arguments.\n usage: ./server <PORT> <FILE-DIR>");

    // if make debug instead of make
    _log("Debug logging enabled.");

    try {
        OPT_PORT = std::stoi(argv[1]);
        OPT_DIR  = argv[2];
        if (OPT_PORT < 0 || OPT_PORT > 65535) throw std::invalid_argument("Invalid Port");
    } catch (const std::exception &e) {
        _log("Invalid arg in ", e.what());
        _exit("Invalid arguments.\nusage: \"./server <PORT> <FILE-DIR>\"");
    }

    _log(OPT_PORT, "|", OPT_DIR);

    int socket_fd;
    socket_fd = open_socket(OPT_PORT);

    packet buffer;

    struct sockaddr_storage client_addr;
    socklen_t address_length;

    // char s[INET6_ADDRSTRLEN];

    uint16_t num_connections = 0;

    while (true) {
        memset(&buffer, 0, sizeof(struct packet));
        rc = recvfrom(socket_fd, &buffer, sizeof(struct packet), 0, (struct sockaddr *)&client_addr, &address_length);
        err(rc, "while recvfrom socket");

        if (buffer.packet_head.flags > 7) {
            _exit("Not Used in header incorrect");
        }

        _log("RECEIVED PACKET:");
        printpacket(&buffer);

        // succesfully got something
        num_connections++;

        //server logic here

        // new connection (incoming SYN)
        if (buffer.packet_head.flags == SYN) {
            time_t curr_time;
            time(&curr_time);
            uint32_t seq_num = 4321; // init seq_num for new connection
            cid_v.push_back(num_connections);
            seq_v.push_back(seq_num);
            time_v.push_back(curr_time);
            
            packet reply;
            memset(&reply, 0, sizeof(struct packet));
            reply.packet_head.sequence_number = seq_num;
            reply.packet_head.ack_number = buffer.packet_head.sequence_number + 1;
            reply.packet_head.connection_id = num_connections;
            reply.packet_head.flags = SYNACK;

            int numbytes = 0;
            numbytes     = sendto(socket_fd, &reply, 12, 0, (struct sockaddr *)&client_addr, address_length);
            err(numbytes, "Sending SYNACK");
            _log("talker: sent ", numbytes, " bytes");
            _log("SENT SYNACK PACKET:");
            printpacket(&reply);
        }



        _log("RECV: Successfully got datagram, length ", rc);
        // _log("got packet from ", inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s)));
    }

    shutdown(socket_fd, 2);
    _log("SHUTDOWN: Closing socket fd");
    return 0;
}
