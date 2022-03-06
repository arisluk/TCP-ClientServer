// ========================================================================== //
// INCLUDES
// ========================================================================== //

// Standard Libraries
#include <iostream>
#include <string>
#include <thread>
#include<tuple>

// C libraries
#include <cerrno>
#include <cstring>
#include <csignal>

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

#ifdef DEBUG
#define OPT_LOG 1
#else
#define OPT_LOG 0
#endif

// ========================================================================== //
// FUNCTIONS
// ========================================================================== //

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
    bzero(&hints, sizeof(hints));

    hints.ai_family   = AF_UNSPEC;   // SUPPORT IPV6
    hints.ai_socktype = SOCK_DGRAM;  // SOCK_STREAM - TCP, SOCK_DGRAM -> UDP

    rc = getaddrinfo(hostname, port_name.c_str(), &hints, &server_info);
    if (rc != 0) {
        _log("SOCKET SETUP: Error getting address info", strerror(errno));
        _exit("Getting address info.", errno);
    }
    _log("SOCKET SETUP: getaddrinfo success.\n");

    for(p = server_info; p != NULL; p = p->ai_next) {
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

int main(int argc, char** argv) {
    int OPT_PORT = 0;
    std::string OPT_HOST;
    std::string OPT_DIR;

    if (argc != 4)
        _exit("Invalid arguments.\n usage: \"./client <HOSTNAME-OR-IP> <PORT> <FILENAME>\"");

    _log("Logging enabled.");

    try {
        OPT_HOST = argv[1];
        OPT_PORT = std::stoi(argv[2]);
        OPT_DIR  = argv[3];
    } catch (const std::exception& e) {
        std::cerr << "Error in: " << e.what() << std::endl;
        _exit("Invalid arguments.\nusage: \"./server <PORT> <FILE-DIR>\"");
    }

    _log(OPT_HOST, "|", OPT_PORT, "|", OPT_DIR);

    int socket_fd;
    struct addrinfo *p;
    std::tie(socket_fd, p) = open_socket(OPT_HOST.c_str(), OPT_PORT);

    int numbytes = 0;
    numbytes = sendto(socket_fd, argv[3], strlen(argv[3]), 0, p->ai_addr, p->ai_addrlen);
    err(numbytes, "Sending message");
    _log("talker: sent ", numbytes, " bytes to ", argv[1]);
    shutdown(socket_fd, 2);

    return 0;
}
