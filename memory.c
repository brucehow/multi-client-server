#include "server.h"

void *create_shared_memory(size_t size) {
    int protection = PROT_READ | PROT_WRITE;
    int visibility = MAP_ANONYMOUS | MAP_SHARED;
    return mmap(NULL, size, protection, visibility, 0, 0);
}

void *allocate_memory(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror(__func__);
        exit(EXIT_FAILURE);
    }
    return ptr;
}
