#define N2_TAG_OFFSET 0
#define N2_PARAMSIZE_OFFSET 2
#define N2_COMMAND_OFFSET 4
#define N2_TAG_SIZE 2
#define N2_PARAMSIZE_SIZE 2
#define N2_COMMAND_SIZE 2
#define N2_CHECKSUM_SIZE 20
#define N2_AUTH_SIZE 20
#define N2_HEADER_SIZE (N2_TAG_SIZE + N2_PARAMSIZE_SIZE + N2_COMMAND_SIZE)

#define N2_TAG_RQU_COMMAND 0xC1
#define N2_TAG_RQU_AUTH_COMMAND 0xC2
#define N2_TAG_RSP_COMMAND 0xC3
#define N2_TAG_RSP_AUTH_COMMAND 0xC4

#define N2_ORD_ENABLE_SESSION 1
#define N2_ORD_DISABLE_SESSION 2
#define N2_ORD_SET_AUTH_KEY 3
#define N2_ORD_SET_ENC_KEY 4
// GAP!
#define N2_ORD_GET_AUTH_LEVEL 9
// GAP!
#define N2_ORD_GET_ERROR_CODE 11
#define N2_ORD_GET_VERSION 12
// GAP! (-> keychip info write?)
#define N2_ORD_READ_KEYCHIP_ID 14
// GAP! (-> gkey write?)
#define N2_ORD_ENCRYPT_WITH_GKEY 16  // keychip.encrypt
#define N2_ORD_DECRYPT_WITH_GKEY 17  // keychip.decrypt
#define N2_ORD_ADD_PLAY_COUNT 18
#define N2_ORD_READ_PLAY_COUNT 19
// GAP! (-> storage? is that still on pic?)
#define N2_ORD_GET_RANDOM_VALUE 24
#define N2_ORD_ENCRYPT_WITH_SKEY 25  // keychip.ssd.hostproof
#define N2_ORD_DECRYPT_WITH_SKEY 26

#define N2_SUCCESS 0
#define N2_INVALID_AUTH 1
#define N2_AUTHFAIL 2
#define N2_LV_ERROR 3
#define N2_BAD_PARAMETER 4
#define N2_EEP_WRITEFAIL 5
#define N2_BAD_TAG 6
#define N2_BAD_ORDINAL 7
#define N2_SUMFAIL 8
#define N2_EEPWRITE_DISABLE 9
#define N2_BAD_DATASIZE 10
#define N2_FAIL 11
