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
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>

// Filesystem

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

bool err(int rc, const char* message, int exit_code = -1) {
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

void _exit(const char* message, int exit_code = 1) {
    if (message)
        fprintf(stderr, "ERROR: %s.\n", message);
    else
        fprintf(stderr, "Unspecified error.\n");
    exit(exit_code);
}

template <typename... Args>
void _log(Args&&... args) {
    if (OPT_LOG) {
        std::cerr << "SERVER DEBUG: ";
        (std::cerr << ... << args);
        std::cerr << std::endl;
    }
}


int open_socket(int port) {
    // https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    
    auto port_name = std::to_string(port);
    _log("SOCKET SETUP: Port name n", port_name, "\n");


    struct addrinfo hints, *res;

    int sockfd, rc;

    // Clear structs
    bzero(&hints, sizeof(hints));

    // SUPPORT IPV6
    hints.ai_family = AF_UNSPEC;

    // SOCK_STREAM - TCP, SOCK_DGRAM -> UDP
    hints.ai_socktype = SOCK_DGRAM;

    // ACCEPT INCOMING CONNECTIONS ON ANY "WILDCARD ADDRESSES"
    hints.ai_flags = AI_PASSIVE;

    // TRY GET ADDRESS
    rc = getaddrinfo(NULL, port_name.c_str(), &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "Error: Getting address info. errno %d: %s\n", errno, gai_strerror(rc));
        exit(errno);
    }
    _log("SOCKET SETUP: getaddrinfo success.\n");

    // TRY CREATE SOCKET
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    err(sockfd, "Unable to create socket");
    _log("SOCKET SETUP: socket success.\n");

    int yes = 1;
    rc      = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    err(rc, "Unable to set socket options");

    // TRY BIND SOCKET
    rc = bind(sockfd, res->ai_addr, res->ai_addrlen);
    err(rc, "Unable to bind socket");
    _log("SOCKET SETUP: bind success.\n");

    return sockfd;
}

int main(int argc, char** argv) {
    int OPT_PORT = 0;
    std::string OPT_DIR;

    int rc = 0;

    if (argc != 3)
        _exit("Invalid arguments.\n usage: \"./server <PORT> <FILE-DIR>\"");

    _log("Logging enabled.");

    try {
        OPT_PORT = std::stoi(argv[1]);
        OPT_DIR  = argv[2];
        if (OPT_PORT < 0 || OPT_PORT > 65535) throw std::runtime_error("Invalid Port");
    } catch (const std::exception& e) {
        _log("Invalid arg in ", e.what());
        _exit("Invalid arguments.\nusage: \"./server <PORT> <FILE-DIR>\"");
    }

    

    _log(OPT_PORT, "|", OPT_DIR);

    int socket_fd;
    socket_fd = open_socket(OPT_PORT);


    char buffer[1024];
    while (true)
    {
        rc = recvfrom(socket_fd, (void*) buffer, sizeof(buffer), 0, ())
    }
    
}
