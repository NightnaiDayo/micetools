#include "mxkDefs.h"

MXK_STATUS mxkPacketReqSetKeyS(unsigned char* packet);
MXK_STATUS mxkPacketReqSetKeyR(unsigned char* packet);
MXK_STATUS mxkPacketReqGetAppBootInfo(unsigned char* packet);
MXK_STATUS mxkPacketReqEepromRead(unsigned char* packet, unsigned char page);
MXK_STATUS mxkPacketReqGetVersion(unsigned char* packet);
MXK_STATUS mxkPacketReqSetMainId(unsigned char* packet);
MXK_STATUS mxkPacketReqGetMainId(unsigned char* packet);
MXK_STATUS mxkPacketReqGetKeyId(unsigned char* packet);
MXK_STATUS mxkPacketReqGetPlayCounter(unsigned char* packet);
MXK_STATUS mxkPacketReqFlashRead(unsigned char* packet, unsigned int address, unsigned int nbytes);
MXK_STATUS mxkPacketReqNvramRead(unsigned char* packet, unsigned short addr, unsigned char blocks);
