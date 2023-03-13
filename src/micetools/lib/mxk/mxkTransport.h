#pragma once
#include <Windows.h>

#include "../ami/ami.h"
#include "mxkDefs.h"

MXK_TRANSPORT_STATUS mxkTransportSend(unsigned char* data, DWORD nbytes);
MXK_TRANSPORT_STATUS mxkTransportRecv(unsigned char* data, DWORD nbytes);
MXK_TRANSPORT_STATUS mxkTransportInit(void);

MXK_STATUS mxkSendPacket(const unsigned char* packet);
MXK_STATUS mxkRecvPacket(unsigned char* packet);
