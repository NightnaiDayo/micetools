#include "../../lib/libpcp/libpcp.h"

pcpa_callback mxkBinaryCallback;
extern byte BINARY_DATA[4096];
extern size_t BINARY_DATA_LEN;

#define MDM_NUM_CALLBACKS 11

#define MXMASTER "mxmaster."
#define FOREGROUND MXMASTER##"foreground."

// Misc
#define MXM_RECONNECT_USB MXMASTER##"reconnect.usb.device"
#define MXM_DEVELOP MXMASTER##"develop"
pcpa_callback mdmPcpReconnectUsbDevice;
pcpa_callback mdmPcpCheckDevelopMode;

// Logs
#define MXM_LOG_AVAILALBE MXMASTER##"logging_available"
#define MXM_OUTPUT_LOG MXMASTER##"output_log"
#define MXM_ERASE_LOG MXMASTER##"erase_log"
pcpa_callback mdmPcpLogAvailable;
pcpa_callback mdmPcpOutputLog;
pcpa_callback mdmPcpEraseLog;

// Foreground control
#define MXM_FG_CURRENT FOREGROUND##"current"
#define MXM_FG_NEXT FOREGROUND##"next"
#define MXM_FG_ACTIVE FOREGROUND##"active"
#define MXM_FG_FAULT FOREGROUND##"fault"
#define MXM_FG_GETCOUNT FOREGROUND##"getcount"
#define MXM_FG_SETCOUNT FOREGROUND##"setcount"
pcpa_callback mdmPcpCurrentFgprocess;
pcpa_callback mdmPcpNextFgprocess;
pcpa_callback mdmPcpActiveFgprocess;
pcpa_callback mdmPcpFaultFgprocess;
pcpa_callback mdmPcpGetStartCount;
pcpa_callback mdmPcpSetStartCount;
