typedef struct {
    unsigned int m_state[4];
    unsigned int m_offset;
    unsigned int m_block;
    unsigned int m_blockOffset;

    unsigned int m_length;

    unsigned char m_buffer[256];
    unsigned char m_digest[16];
} AmiMd5;

void amiMd5Init(AmiMd5 *md5);
void amiMd5Update(AmiMd5 *md5, unsigned int nBytes, unsigned char *buffer);
void amiMd5Finalise(AmiMd5 *md5);
