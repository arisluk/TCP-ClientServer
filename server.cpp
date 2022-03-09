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

std::map<uint16_t, std::map<int32_t, packet>> out_of_order;

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

    uint16_t num_connections = 0;

    while (true) {
        memset(&incoming_packet, 0, sizeof(struct packet));

        

        uint32_t incoming_seq;
        uint32_t incoming_ack;
        uint16_t cid;
        uint8_t incoming_flag;

        int packet_from = PACKET_FROM_REC;
        for (auto& [c_id, value] : out_of_order) {
            uint32_t smallest_seq = value.begin()->first;
            while (smallest_seq < database.at(c_id).seq)
            {
                value.erase(smallest_seq);
                smallest_seq = value.begin()->first;
            }
            
            smallest_seq = value.begin()->first;
            if (smallest_seq == database.at(c_id).seq)
            {
                //
                incoming_packet = value.begin()->second;
                if (value.size() == 1) {
                    packet_from = PACKET_LAST_FROM_BUFFER;
                } else {
                    packet_from = PACKET_FROM_BUFFER;
                }
            };
        }

        if (packet_from == PACKET_FROM_REC) {
            addr_len = sizeof(client_addr);
            rc = recvfrom(socket_fd, &incoming_packet, sizeof(struct packet), 0, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len);
            err(rc, "SERVER: while recvfrom socket (server)");
            _log("RECV: Successfully got datagram, length ", rc);
            _log("RECEIVED PACKET, at time:" , time_now_ms());
            
            incoming_seq = ntohl(incoming_packet.packet_head.sequence_number);
            incoming_ack = ntohl(incoming_packet.packet_head.ack_number);
            cid = ntohs(incoming_packet.packet_head.connection_id);
            incoming_flag = incoming_packet.packet_head.flags;

            if (incoming_flag != SYN && incoming_seq < database.at(cid).seq) {
                output_packet_server(&incoming_packet, TYPE_DROP);
                continue;
            }

            printpacket(&incoming_packet);
            output_packet_server(&incoming_packet, TYPE_RECV);

        } else if (packet_from == PACKET_FROM_BUFFER || packet_from == PACKET_LAST_FROM_BUFFER) {
            incoming_seq = ntohl(incoming_packet.packet_head.sequence_number);
            incoming_ack = ntohl(incoming_packet.packet_head.ack_number);
            cid = ntohs(incoming_packet.packet_head.connection_id);
            incoming_flag = incoming_packet.packet_head.flags;

            if (incoming_flag != SYN && database.count(cid) <= 0) {
                out_of_order.erase(cid);
                continue;
            }
        }

        if(cid != 0 && database.count(cid) <= 0) {
            output_packet_server(&incoming_packet, TYPE_DROP);
            continue;
        }

        uint64_t time_now = time_now_ms();

        if (incoming_flag > 7) _exit("Incorrect flags");


        bool reply_needed = false;
        uint32_t reply_seq = 0;
        uint32_t reply_ack = 0;
        uint16_t reply_cid = 0;
        uint8_t reply_flag = 0;
        int reply_type = 0;

        // Check timing for RTO
        for (auto const& [key, val] : database)
        {
            uint64_t time_diff = time_now - val.last_time;
            _log(key, " time diff = ", time_diff / 10);
            if (time_now > val.last_time && time_diff > 10000) {
                database.at(key).state = STATE_FIN;
                char err_msg[50];
                sprintf(err_msg, "ERROR%ld, %ld", key, time_diff);
                int written = write(database.at(key).writefd, err_msg, sizeof(err_msg));
                _log("writte = ", written);
                close(database.at(key).writefd);
                database.erase(key);
                out_of_order.erase(key);
            }
        }


        // new connection (incoming SYN)
        if (incoming_flag == SYN) {
            num_connections++;
            num_connections %= 11;
            
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

            Store temp(reply_ack, 0, time_now_ms(), write_fd, STATE_ACTIVE);
            database[num_connections] = temp;
        } else if (incoming_flag == FIN) {
            database.at(cid).state = STATE_FIN;
            close(database.at(cid).writefd);

            reply_needed = true;
            reply_seq = database.at(cid).ack;
            reply_ack = incoming_seq + 1;
            reply_cid = cid;
            reply_flag = FINACK;
            reply_type = TYPE_SEND;
        }
        else if (incoming_flag == ACK) {
            if (incoming_seq != database.at(cid).seq) {
                database.at(cid).seq = database.at(cid).seq;
                if (packet_from == PACKET_FROM_REC) {
                    packet copy = incoming_packet;
                    out_of_order[cid][incoming_seq] = copy;
                }
            } else {
                database.at(cid).seq = (incoming_seq + rc - 12) % (SPEC_MAX_SEQ + 1);
                int written = write(database.at(cid).writefd, incoming_packet.payload, rc-12);
                _log("writte = ", written);
            }
            database.at(cid).ack = incoming_ack;
            database.at(cid).last_time = time_now_ms();

            reply_needed = true;
            reply_seq = incoming_ack;
            reply_ack = database.at(cid).seq;
            reply_cid = cid;
            reply_flag = ACK;
            reply_type = TYPE_SEND;
            if (database.at(cid).state == STATE_FIN) {
                reply_needed = false;
                database.erase(cid);
                out_of_order.erase(cid);
            }
        }
        else {
            if (incoming_seq != database.at(cid).seq) {
                database.at(cid).seq = database.at(cid).seq;
                if (packet_from == PACKET_FROM_REC) {
                    packet copy = incoming_packet;
                    out_of_order[cid][incoming_seq] = copy;
                }
            } else {
                database.at(cid).seq = (incoming_seq + rc - 12) % (SPEC_MAX_SEQ + 1);
                int written = write(database.at(cid).writefd, incoming_packet.payload, rc-12);
                _log("writte = ", written);
            }
            database.at(cid).last_time = time_now_ms();

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
