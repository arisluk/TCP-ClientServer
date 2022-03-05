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
    } catch (const std::exception& e) {
        std::cerr << "Error in: " << e.what() << std::endl;
        _exit("Invalid arguments.\nusage: \"./server <PORT> <FILE-DIR>\"");
    }

    _log(OPT_PORT, "|", OPT_DIR);
    std::cerr << "server is not implemented yet" << std::endl;
}
