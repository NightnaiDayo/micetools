#include "ringbuf.h"

#include <memory.h>

void ringbuf_purge(ring_buffer_t* ring) {
    ring->read = 0;
    ring->write = 0;
}

short ringbuf_available(ring_buffer_t* ring) {
    short populated = ring->write - ring->read;
    if (populated < 0) populated += RING_BUFFER_SIZE;
    return populated;
}

bool ringbuf_write(ring_buffer_t* ring, unsigned const char* data, short bytes) {
    short unpopulated = ring->read - ring->write;
    if (unpopulated < 0) unpopulated += RING_BUFFER_SIZE;
    bool overflow = unpopulated < bytes;

    while (bytes > 0) {
        short chunk;
        if (bytes + ring->write > RING_BUFFER_SIZE)
            chunk = RING_BUFFER_SIZE - ring->write;
        else
            chunk = bytes;

        memcpy(ring->buffer + ring->write, data, chunk);
        data += chunk;
        bytes -= chunk;
        ring->write = (ring->write + chunk) % RING_BUFFER_SIZE;
    }

    return overflow;
}

short ringbuf_read(ring_buffer_t* ring, unsigned char* data, short bytes) {
    short populated = ring->write - ring->read;
    if (populated < 0) populated += RING_BUFFER_SIZE;
    // Underflow
    if (populated < bytes) bytes = populated;

    short left = bytes;
    while (left > 0) {
        short chunk;
        if (left + ring->read > RING_BUFFER_SIZE)
            chunk = RING_BUFFER_SIZE - ring->read;
        else
            chunk = left;

        memcpy(data, ring->buffer + ring->read, chunk);
        data += chunk;
        left -= chunk;
        ring->read = (ring->read + chunk) % RING_BUFFER_SIZE;
    }

    return bytes;
}
