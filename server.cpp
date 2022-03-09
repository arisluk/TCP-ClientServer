// ========================================================================== //
// INCLUDES
// ========================================================================== //

// Standard Libraries
#include <iostream>
#include <chrono>
#include <string>
#include <thread>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <vector>
#include <dirent.h>
#include <map>

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
#include <filesystem>

// Local
#include "common.h"

using namespace std;

// ========================================================================== //
// DEFINITIONS
// ========================================================================== //

std::map<unsigned int, Store> database;

// ========================================================================== //
// FUNCTIONS
// ========================================================================== //

int open_socket(int port, int* addrlen) {
    // https://man7.org/linux/man-pages/man3/getaddrinfo.3.html

    auto port_name = std::to_string(port);
    _log("SOCKET SETUP: Port name: ", port_name, "\n");

    struct addrinfo hints, *server_info, *p;

    int socket_fd = -1;
    int rc;

    // clear structs
    memset(&hints, 0, sizeof(hints));

    hints.ai_family   = AF_INET;   // SUPPORT IPV4 ONLY
    hints.ai_socktype = SOCK_DGRAM;  // SOCK_STREAM - TCP, SOCK_DGRAM -> UDP
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_UDP;

    rc = getaddrinfo(NULL, port_name.c_str(), &hints, &server_info);
    if (rc != 0) {
        _log("SOCKET SETUP: Error getting address info", strerror(errno));
        _exit("Getting address info.", errno);
    }
    _log("SOCKET SETUP: getaddrinfo success.\n");

    for (p = server_info; p != NULL; p = p->ai_next) {
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_fd == -1) continue;

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            shutdown(socket_fd, 2);
            _log("SOCKET BIND: failed a bind");
            continue;
        }
        *addrlen = p->ai_addrlen;
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

uint64_t time_now_ms() {
    using namespace std::chrono;
    return std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
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

    std::filesystem::path dir (OPT_DIR);
    
    int socket_fd;
    int addr_len = 0;
    socket_fd = open_socket(OPT_PORT, &addr_len);

    packet incoming_packet;

    struct sockaddr_storage client_addr;
    socklen_t address_length;

    uint16_t num_connections = 0;

    while (true) {
        memset(&incoming_packet, 0, sizeof(struct packet));
        rc = recvfrom(socket_fd, &incoming_packet, sizeof(struct packet), 0, (struct sockaddr *)&client_addr, &address_length);
        err(rc, "SERVER: while recvfrom socket (server)");

        uint32_t incoming_seq = ntohl(incoming_packet.packet_head.sequence_number);
        uint32_t incoming_ack = ntohl(incoming_packet.packet_head.ack_number);
        uint16_t cid = ntohs(incoming_packet.packet_head.connection_id);
        uint8_t incoming_flag = incoming_packet.packet_head.flags;

        if(cid != 0 && database.count(cid) <= 0) {
            output_packet_server(&incoming_packet, TYPE_DROP);
            continue;
        }

        uint64_t time_now = time_now_ms();

        if (incoming_packet.packet_head.flags > 7) _exit("Incorrect flags");

        _log("RECV: Successfully got datagram, length ", rc);
        _log("RECEIVED PACKET, at time:" , time_now);
        printpacket(&incoming_packet);
        output_packet_server(&incoming_packet, TYPE_RECV);

        bool reply_needed = false;
        uint32_t reply_seq = ntohl(incoming_packet.packet_head.sequence_number);
        uint32_t reply_ack = ntohl(incoming_packet.packet_head.ack_number);
        uint16_t reply_cid = ntohs(incoming_packet.packet_head.connection_id);
        uint8_t reply_flag = incoming_packet.packet_head.flags;
        int reply_type = 0;


        // new connection (incoming SYN)
        if (incoming_flag == SYN) {
            num_connections++;
            
            char filename[50];
            snprintf(filename, 49, "%d.file", num_connections);
            std::filesystem::path new_connection(filename);
            std::filesystem::path full_path = dir / new_connection;

            int write_fd = open(full_path.c_str(), O_CREAT | O_WRONLY, S_IRWXU);
            err(write_fd, "opening path");
            
            _log("WRITEFD = ", write_fd);
            
            reply_needed = true;
            reply_seq = 4321;
            reply_ack = incoming_seq + 1;
            reply_cid = num_connections;
            reply_flag = SYNACK;
            reply_type = TYPE_SEND;

            Store temp(reply_seq, 0, time_now_ms(), write_fd, STATE_ACTIVE);
            database[num_connections] = temp;
        } else if (incoming_packet.packet_head.flags == FIN) {
            database.at(cid).last_time = time_now_ms();
            database.at(cid).state = STATE_FIN;
            close(database.at(cid).writefd);

            reply_needed = true;
            reply_seq = database.at(cid).ack;
            reply_ack = incoming_seq + 1;
            reply_cid = cid;
            reply_flag = FINACK;
            reply_type = TYPE_SEND;
        }
        else if (incoming_packet.packet_head.flags == ACK) {
            database.at(cid).seq = (incoming_seq + rc - 12) % (SPEC_MAX_SEQ + 1);
            database.at(cid).ack = incoming_ack;
            database.at(cid).last_time = time_now_ms();

            int written = write(database.at(cid).writefd, incoming_packet.payload, rc-12);
            _log("writte = ", written);

            reply_needed = true;
            reply_seq = incoming_ack;
            reply_ack = database.at(cid).seq;
            reply_cid = cid;
            reply_flag = ACK;
            reply_type = TYPE_SEND;

            
            if (database.at(cid).state == STATE_FIN) reply_needed = false;
        }
        else {
            database.at(cid).seq = (incoming_seq + rc - 12)  % (SPEC_MAX_SEQ + 1);
            database.at(cid).last_time = time_now_ms();

            if (database.count(cid) <= 0) continue;
            int written = write(database.at(cid).writefd, incoming_packet.payload, rc-12);
            _log("writte = ", written);

            reply_needed = true;
            reply_seq = database.at(cid).ack;
            reply_ack = database.at(cid).seq;
            reply_flag = ACK;
            reply_cid = cid;
            reply_type = TYPE_SEND;
        }

        if (reply_needed) {
            packet reply;
            memset(&reply, 0, sizeof(struct packet));
            reply.packet_head.sequence_number = htonl(reply_seq);
            reply.packet_head.ack_number = htonl(reply_ack);
            reply.packet_head.connection_id = htons(reply_cid);
            reply.packet_head.flags = reply_flag;

            int numbytes = 0;
            numbytes     = sendto(socket_fd, &reply, 12, 0, (struct sockaddr *)&client_addr, addr_len);
            _log(&client_addr, addr_len);
            err(numbytes, "Sending response");
            _log("talker: sent ", numbytes, " bytes");
            _log("SENT PACKET:");
            printpacket(&reply);
            output_packet_server(&reply, reply_type);    
        }

    }

    shutdown(socket_fd, 2);
    _log("SHUTDOWN: Closing socket fd");
    return 0;
}
