#include <Windows.h>

#include "../../common.h"
#include "jvs.h"

static void Ctor(PMICE_JVS this) {
    this->vftable = &_MiceJvsBase_vftable;

    this->m_nInIdx = 0;
    this->m_lpInData = NULL;
    this->m_nOutIdx = 0;
    this->m_lpOutData = NULL;

    this->m_BoardId = NULL;

    this->m_Address = 0xff;

    // Sane, but impractical, defaults. Boards should set better values.
    this->m_CommandVersion = 0x10;
    this->m_JvsRevision = 0x20;
    this->m_VersionComm = 0x10;
}

static JVS_STATUS Transact(PMICE_JVS this, BYTE command) {
    switch (command) {
        case JVS_CMD_RESET:
            if (MiceJvsRead(this) != JVS_CMD_RESET_ASSERT) return JVS_STATUS_UKCOM;

            this->m_Address = 0xff;
            this->m_SenseOut = TRUE;
            return JVS_STATUS_SILENCE;
        case JVS_CMD_CHANGE_COMMS:
            return JVS_STATUS_SILENCE;

        case JVS_CMD_ASSIGN_ADDR:
            MiceJvsWrite(this, JVS_REPORT_OK);
            this->m_SenseOut = FALSE;
            // we don't bother remembering the address because at the moment we only simulate
            // a single board in the JVS chain.
            MiceJvsRead(this);
            return JVS_STATUS_OK;

        case JVS_CMD_READ_ID:
            MiceJvsWrite(this, JVS_REPORT_OK);
            if (this->m_BoardId == NULL)
                MiceJvsWrite(this, 0x00);
            else
                for (size_t i = 0; i < strlen(this->m_BoardId); i++)
                    MiceJvsWrite(this, this->m_BoardId[i]);
            return JVS_STATUS_OK;
        case JVS_CMD_GET_CMD_VERSION:
            MiceJvsWrite(this, JVS_REPORT_OK);
            MiceJvsWrite(this, this->m_CommandVersion);
            return JVS_STATUS_OK;
        case JVS_CMD_GET_JVS_VERSION:
            MiceJvsWrite(this, JVS_REPORT_OK);
            MiceJvsWrite(this, this->m_JvsRevision);
            return JVS_STATUS_OK;
        case JVS_CMD_GET_COMM_VERSION:
            MiceJvsWrite(this, JVS_REPORT_OK);
            MiceJvsWrite(this, this->m_VersionComm);
            return JVS_STATUS_OK;

        case JVS_CMD_GET_FEATURES:
            MiceJvsWrite(this, JVS_REPORT_OK);

            MiceJvsWrite(this, JVS_FEATURE_PLAYERS);
            MiceJvsWrite(this, this->m_Players);
            MiceJvsWrite(this, this->m_ButtonsPerPlayer);
            MiceJvsWrite(this, JVS_FEATURE_PAD);
            MiceJvsWrite(this, JVS_FEATURE_COINS);
            MiceJvsWrite(this, this->m_Coins);
            MiceJvsWrite(this, JVS_FEATURE_PAD);
            MiceJvsWrite(this, JVS_FEATURE_PAD);

            // TODO: Make this based on `this`
            MiceJvsWrite(this, JVS_FEATURE_ANALOG);
            MiceJvsWrite(this, 8);                // 8 ADC channels
            MiceJvsWrite(this, JVS_FEATURE_PAD);  // ?? (was "10" prior)
            MiceJvsWrite(this, JVS_FEATURE_PAD);

            // TODO: Make this based on `this`
            MiceJvsWrite(this, JVS_FEATURE_GPIO);
            MiceJvsWrite(this, 6);  // 6 ports
            MiceJvsWrite(this, JVS_FEATURE_PAD);
            MiceJvsWrite(this, JVS_FEATURE_PAD);

            MiceJvsWrite(this, JVS_FEATURE_EOF);
            return JVS_STATUS_OK;

        case JVS_CMD_RECEIVE_MAIN_ID:
            unsigned char tempRead = 0xff;
            while (tempRead != 0) tempRead = MiceJvsRead(this);
            // TODO: Do we need to report here?
            return JVS_STATUS_OK;

        default:
            log_error(plfMxJvs, "Unknown command: 0x%02x", command);
            return JVS_STATUS_UKCOM;
    }
}

static void ReadAllButtons(PMICE_JVS this) {
    MiceInputPollDevices();

    for (DWORD i = 0; i < this->m_NumButtons; i++) {
        this->m_ButtonStates[i] = MiceInputGetButtonState(&(this->m_Bindings[i]));
    }
}

MICE_JVS_vftable _MiceJvsBase_vftable = {
    .ctor = Ctor,
    .transact = Transact,
    .readAllButtons = ReadAllButtons,
};
