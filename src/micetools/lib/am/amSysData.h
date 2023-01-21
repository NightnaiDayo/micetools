typedef struct {
    unsigned short m_Year;
    unsigned char m_Month;
    unsigned char m_Day;
    unsigned char m_Hour;
    unsigned char m_Minute;
    unsigned char m_Second;
    unsigned char Rsv07[1];
} AM_SYSDATA_TIMESTAMP;

void amSysDataGetDotClockTiming(void);
void amSysDataGetOsVersionString(void);
