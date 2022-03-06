#include "common.h"

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
