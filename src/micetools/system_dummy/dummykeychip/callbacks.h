#include "../../lib/libpcp/libpcp.h"

pcpa_callback mdkBinaryCallback;
extern byte BINARY_DATA[4096];
extern size_t BINARY_DATA_LEN;

#define KEYCHIP "keychip."
#define APPBOOT KEYCHIP##"appboot."
#define BILLING KEYCHIP##"billing."
#define TRACEDATA KEYCHIP##"tracedata."

// Misc
#define KC_VERSION KEYCHIP##"version"
#define KC_STATUS KEYCHIP##"status"
pcpa_callback mdkPcpVersion;
pcpa_callback mdkPcpStatus;

// Crypto
#define DS_COMPUTE KEYCHIP##"ds.compute"
#define SSD_PROOF KEYCHIP##"ssd.proof"
#define SSD_HOSTPROOF KEYCHIP##"ssd.hostproof"
#define KC_ENCRYPT KEYCHIP##"encrypt"
#define KC_DECRYPT KEYCHIP##"decrypt"
#define KC_SETIV KEYCHIP##"setiv"
pcpa_callback mdkPcpDsCompute;
pcpa_callback mdkPcpSsdProof;
pcpa_callback mdkPcpSsdHostProof;
pcpa_callback mdkPcpEncrypt;
pcpa_callback mdkPcpDecrypt;
pcpa_callback mdkPcpSetIv;

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
pcpa_callback mdkPcpAbGameId;
pcpa_callback mdkPcpAbSystemFlag;
pcpa_callback mdkPcpAbModelType;
pcpa_callback mdkPcpAbFormatType;
pcpa_callback mdkPcpAbRegion;
pcpa_callback mdkPcpAbPlatformId;
pcpa_callback mdkPcpAbNetworkAddress;
pcpa_callback mdkPcpAbDvd;
pcpa_callback mdkPcpAbSeed;

// Billing
#define BIL_KEYID BILLING##"keyid"
#define BIL_MAINID BILLING##"mainid"
#define BIL_PLAYCOUNT BILLING##"playcount"
#define BIL_PLAYLIMIT BILLING##"playlimit"
#define BIL_NEARFULL BILLING##"nearfull"
#define BIL_SIGNATURE BILLING##"signaturepubkey"
#define BIL_CACERT BILLING##"cacertification"
pcpa_callback mdkPcpPbKeyId;
pcpa_callback mdkPcpPbMainId;
pcpa_callback mdkPcpPbPlayCount;
pcpa_callback mdkPcpPbPlayLimit;
pcpa_callback mdkPcpPbNearfull;
pcpa_callback mdkPcpPbSignaturePubKey;
pcpa_callback mdkPcpPbCaCertification;

// Tracedata
#define TRA_RESTORE TRACEDATA##"restore"
#define TRA_PUT TRACEDATA##"put"
#define TRA_GET TRACEDATA##"get"
#define TRA_LOGICALERASE TRACEDATA##"logicalerase"
#define TRA_SECTOREERASE TRACEDATA##"sectorerase"
pcpa_callback mdkPcpTdRestore;
pcpa_callback mdkPcpTdPut;
pcpa_callback mdkPcpTdGet;
pcpa_callback mdkPcpTdLogicalErase;
pcpa_callback mdkPcpTdSectorErase;

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
pcpa_callback mdkPcpEeprom;
pcpa_callback mdkPcpNvram;
