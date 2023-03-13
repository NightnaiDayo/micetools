#pragma pack(push, 1)
typedef struct {
    unsigned char Unk0[4];
    unsigned char m_EepromPage[32];
    unsigned char m_ChallengeLow[4];
    unsigned char m_Page;
    unsigned char m_UniqueId[7];
    unsigned char Unk30[4];
    unsigned char m_ChallengeHigh[3];
    unsigned char Unk37[9];
} dsShaPayload_t;
#pragma pack(pop)

bool mxkDsExioWaitNotBusy(void);
int mxkDsExioRequestComputeMac(void);
int mxkDsExioWriteInputBuffer(dsShaPayload_t *buffer);
int mxkDsExioReadMacOutputBuffer(unsigned char *macBuffer);

int mxkDsWaitNotBusy(int *param_1);
int mxkDsKeychipComputeMac(unsigned char page, void *challenge, unsigned char *mac, int flag);
int mxkDsGetUniqueNumber(unsigned char *param_1);
bool mxkDsMakeSha1InputForVerify(unsigned char page, dsShaPayload_t *payload,
                                 unsigned char *challenge);
int mxkDsKeychipReadEeprom(unsigned char *pageData, unsigned char page);

int mxkAuthenticationDs(void);
