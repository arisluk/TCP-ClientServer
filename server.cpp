// ========================================================================== //
// INCLUDES
// ========================================================================== //

// Standard Libraries
#include <iostream>
#include <string>
#include <thread>

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

// ========================================================================== //
// DEFINITIONS
// ========================================================================== //

#define BUFFER_SIZE 1024

void *get_in_addr(struct sockaddr *sa) {
  return sa->sa_family == AF_INET
    ? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
    : (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// ========================================================================== //
// FUNCTIONS
// ========================================================================== //

int open_socket(int port) {
    // https://man7.org/linux/man-pages/man3/getaddrinfo.3.html

    auto port_name = std::to_string(port);
    _log("SOCKET SETUP: Port name: ", port_name, "\n");

    struct addrinfo hints, *server_info, *p;

    int socket_fd = -1;
    int rc;

    // Clear structs
    bzero(&hints, sizeof(hints));

    hints.ai_family   = AF_UNSPEC;   // SUPPORT IPV6
    hints.ai_socktype = SOCK_DGRAM;  // SOCK_STREAM - TCP, SOCK_DGRAM -> UDP
    hints.ai_flags    = AI_PASSIVE;  // ACCEPT INCOMING CONNECTIONS ON ANY "WILDCARD ADDRESSES"

    rc = getaddrinfo(NULL, port_name.c_str(), &hints, &server_info);
    if (rc != 0) {
        _log("SOCKET SETUP: Error getting address info", strerror(errno));
        _exit("Getting address info.", errno);
    }
    _log("SOCKET SETUP: getaddrinfo success.\n");

    for(p = server_info; p != NULL; p = p->ai_next) {
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

int main(int argc, char** argv) {
    int OPT_PORT = 0;
    std::string OPT_DIR;

    int rc = 0;

    if (argc != 3)
        _exit("Invalid arguments.\n usage: ./server <PORT> <FILE-DIR>");

    _log("Logging enabled.");

    try {
        OPT_PORT = std::stoi(argv[1]);
        OPT_DIR  = argv[2];
        if (OPT_PORT < 0 || OPT_PORT > 65535) throw std::invalid_argument("Invalid Port");
    } catch (const std::exception& e) {
        _log("Invalid arg in ", e.what());
        _exit("Invalid arguments.\nusage: \"./server <PORT> <FILE-DIR>\"");
    }

    _log(OPT_PORT, "|", OPT_DIR);

    int socket_fd;
    socket_fd = open_socket(OPT_PORT);

    
    char buffer[BUFFER_SIZE];

    struct sockaddr_storage client_addr;
    socklen_t address_length;
    
    char s[INET6_ADDRSTRLEN];

    while (true) {
        rc = recvfrom(socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &address_length);
        err(rc, "recvfrom socket");
        _log("RECV: Successfully got datagram, length ", rc);
        _log("got packet from ", inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s)));

    }

    shutdown(socket_fd, 2);
    _log("SHUTDOWN: Closing socket fd");
    return 0;
}
