#pragma once
#include <Windows.h>
#include <stdbool.h>

#include "../ami/ami.h"
#include "mxkAb.h"
#include "mxkCrypt.h"
#include "mxkDefs.h"
#include "mxkDs.h"
#include "mxkN2.h"
#include "mxkPacket.h"
#include "mxkSmbus.h"
#include "mxkTransport.h"

extern unsigned char BILLING_PRIVKEY[917];
extern unsigned char BILLING_PUBKEY[162];
extern unsigned char BILLING_CACERT[817];
extern unsigned char KEYCHIP_ID[16];
extern unsigned char MAIN_ID[16];

MXK_STATUS mxkInit(void);
MXK_STATUS mxkExchengeAesKey(void);
MXK_STATUS mxkVersion(unsigned short* version, MXK_CACHE cache, unsigned char* err);

// MXK
BOOL mxkSetMainId(const unsigned char* main_id);
BOOL mxkGetMainId(unsigned char* main_id);
BOOL mxkGetKeyId(unsigned char* main_id);
BOOL mxkGetPlayCounter(DWORD* play_counter);
BOOL mxkFlashRead(unsigned int address, unsigned int nbytes, unsigned char* buffer);
BOOL mxkEepromRead(unsigned char page, unsigned char* data);
BOOL mxkNvramRead(unsigned short addr, unsigned char blocks, unsigned char* data);
