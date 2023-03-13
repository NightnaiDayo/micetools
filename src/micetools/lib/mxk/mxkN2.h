#pragma once

#include <varargs.h>

void _randomBytes(unsigned char *buf, int nbytes);

int mxkN2UtilVCatenateData(unsigned char *out, size_t *count, unsigned int numStreams,
                           va_list args);
void mxkN2UtilCatenateData(unsigned char *concatinated, size_t *count, unsigned int numStreams,
                           ...);

void mxkN2UtilSha1Many(unsigned char *sum, int numStreams, ...);
int mxkN2UtilHmacPacket(void *val1, unsigned char *sha1, void *nonce, void *key,
                        unsigned char *hash_out);

int mxkN2CmdWriteCommand(unsigned char *param_1, unsigned char *packet, unsigned short tag,
                         unsigned short command, unsigned char *auth_key, unsigned char addr,
                         void *data, size_t nbytes);

int mxkN2CmdEnableSession(void);
int mxkN2CmdSetAuthKey(unsigned char* authKey);

int mxkN2CmdInit(void);
int mxkN2CmdSetDeviceInfo(void);
int mxkN2Authentication(void);
