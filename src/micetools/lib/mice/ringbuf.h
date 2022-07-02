#pragma once
#include <stdbool.h>

// We use shorts for indexes so the max allowed here is 0x7fff
#define RING_BUFFER_SIZE 2048

typedef struct ring_buffer {
    unsigned char buffer[RING_BUFFER_SIZE];
    short read;
    short write;
} ring_buffer_t;

bool ringbuf_write(ring_buffer_t* ring, unsigned const char* data, short bytes);
short ringbuf_read(ring_buffer_t* ring, unsigned char* data, short bytes);
short ringbuf_available(ring_buffer_t* ring);
void ringbuf_purge(ring_buffer_t* ring);
