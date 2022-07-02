#include <ctype.h>

unsigned char hex_value(char c) {
    if (isdigit((unsigned char)c)) return c - '0';

    switch (c) {
        case 'a':
        case 'A':
            return 0x0A;
        case 'b':
        case 'B':
            return 0x0B;
        case 'c':
        case 'C':
            return 0x0C;
        case 'd':
        case 'D':
            return 0x0D;
        case 'e':
        case 'E':
            return 0x0E;
        case 'f':
        case 'F':
            return 0x0F;
        default:
            return 0xFF;
    }
}
int hex_to_bin(char *hex, unsigned char *bin, int hex_len) {
    if (hex_len == 0) return 1;

    int work_len = 0;
    for (int hex_ptr = 0; hex_ptr < hex_len; hex_ptr += 2, work_len++) {
        char msn = hex[hex_ptr];

        if ('0' <= msn && msn <= '9')
            msn = msn - '0';
        else if ('A' <= msn && msn <= 'F')
            msn = msn - 'A' + 10;
        else if ('a' <= msn && msn <= 'f')
            msn = msn - 'a' + 10;
        else
            return 0;

        char lsn = hex_ptr + 1 == hex_len ? '0' : hex[hex_ptr + 1];

        if ('0' <= lsn && lsn <= '9')
            lsn = lsn - '0';
        else if ('A' <= lsn && lsn <= 'F')
            lsn = lsn - 'A' + 10;
        else if ('a' <= lsn && lsn <= 'f')
            lsn = lsn - 'a' + 10;
        else
            return 0;

        bin[work_len] = msn << 4 | lsn;
    }
    return 1;
}
