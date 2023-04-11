int hex_to_byte(char *hex, unsigned char *val);
unsigned char hex_value(char c);
int hex_to_bin(char *hex, void *bin, int hex_len);
int hex_to_int(unsigned int *value, char *hex);
void bin_to_hex(char *hex, void *bin, int nbytes);
