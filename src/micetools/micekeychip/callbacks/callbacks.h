#include "../lib/libpcp/libpcp.h"

pcpa_callback mxkBinaryCallback;
extern byte BINARY_DATA[4096];
extern size_t BINARY_DATA_LEN;

#define KEYCHIP "keychip."
#define APPBOOT KEYCHIP##"appboot."
#define BILLING KEYCHIP##"billing."
#define TRACEDATA KEYCHIP##"tracedata."

// Misc
#define KC_VERSION KEYCHIP##"version"
#define KC_STATUS KEYCHIP##"status"
pcpa_callback mxkPcpVersion;
pcpa_callback mxkPcpStatus;

// Crypto
#define DS_COMPUTE KEYCHIP##"ds.compute"
#define SSD_PROOF KEYCHIP##"ssd.proof"
#define SSD_HOSTPROOF KEYCHIP##"ssd.hostproof"
#define KC_ENCRYPT KEYCHIP##"encrypt"
#define KC_DECRYPT KEYCHIP##"decrypt"
#define KC_SETIV KEYCHIP##"setiv"
pcpa_callback mxkPcpDsCompute;
pcpa_callback mxkPcpSsdProof;
pcpa_callback mxkPcpSsdHostProof;
pcpa_callback mxkPcpEncrypt;
pcpa_callback mxkPcpDecrypt;
pcpa_callback mxkPcpSetIv;

// Appboot
#define AB_GAMEID APPBOOT##"gameid"
#define AB_SYSTEMFLAG APPBOOT##"systemflag"
#define AB_MODELTYPE APPBOOT##"modeltype"
#define AB_FORMATTYPE APPBOOT##"formattype"
#define AB_REGION APPBOOT##"region"
#define AB_PLATFORMID APPBOOT##"platformid"
#define AB_NETWORKADDRESS APPBOOT##"networkaddr"
#define AB_DVD APPBOOT##"dvdflag"
#define AB_SEED APPBOOT##"seed"
pcpa_callback mxkPcpAbGameId;
pcpa_callback mxkPcpAbSystemFlag;
pcpa_callback mxkPcpAbModelType;
pcpa_callback mxkPcpAbFormatType;
pcpa_callback mxkPcpAbRegion;
pcpa_callback mxkPcpAbPlatformId;
pcpa_callback mxkPcpAbNetworkAddress;
pcpa_callback mxkPcpAbDvd;
pcpa_callback mxkPcpAbSeed;

// Billing
#define BIL_KEYID BILLING##"keyid"
#define BIL_MAINID BILLING##"mainid"
#define BIL_PLAYCOUNT BILLING##"playcount"
#define BIL_PLAYLIMIT BILLING##"playlimit"
#define BIL_NEARFULL BILLING##"nearfull"
#define BIL_SIGNATURE BILLING##"signaturepubkey"
#define BIL_CACERT BILLING##"cacertification"
pcpa_callback mxkPcpPbKeyId;
pcpa_callback mxkPcpPbMainId;
pcpa_callback mxkPcpPbPlayCount;
pcpa_callback mxkPcpPbPlayLimit;
pcpa_callback mxkPcpPbNearfull;
pcpa_callback mxkPcpPbSignaturePubKey;
pcpa_callback mxkPcpPbCaCertification;

// Tracedata
#define TRA_RESTORE TRACEDATA##"restore"
#define TRA_PUT TRACEDATA##"put"
#define TRA_GET TRACEDATA##"get"
#define TRA_LOGICALERASE TRACEDATA##"logicalerase"
#define TRA_SECTOREERASE TRACEDATA##"sectorerase"
pcpa_callback mxkPcpTdRestore;
pcpa_callback mxkPcpTdPut;
pcpa_callback mxkPcpTdGet;
pcpa_callback mxkPcpTdLogicalErase;
pcpa_callback mxkPcpTdSectorErase;

// Storage
#define KC_EEPROM KEYCHIP##"eeprom"
#define KC_NVRAM KEYCHIP##"nvram"
#define KC_NVRAM0 KC_NVRAM##"0"
#define KC_NVRAM1 KC_NVRAM##"1"
#define KC_NVRAM2 KC_NVRAM##"2"
#define KC_NVRAM3 KC_NVRAM##"3"
#define KC_NVRAM4 KC_NVRAM##"4"
#define KC_NVRAM5 KC_NVRAM##"5"
#define KC_NVRAM6 KC_NVRAM##"6"
#define KC_NVRAM7 KC_NVRAM##"7"
#define KC_NVRAM8 KC_NVRAM##"8"
#define KC_NVRAM9 KC_NVRAM##"9"
pcpa_callback mxkPcpEeprom;
pcpa_callback mxkPcpNvram;
