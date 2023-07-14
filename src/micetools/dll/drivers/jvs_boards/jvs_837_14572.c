#include <Windows.h>

#include "../../common.h"

#define PLAYER_COUNT 2
#define COIN_COUNTERS 2

#define SWITCH_BYTES 2
#define PLAYER_SWITCH_COUNT 13  // Must be less than 8*SWITCH_BYTES !!

BOOL coin_solenoid = false;

static const char JVS_837_14572_ID[] = "SEGA ENTERPRISES,LTD.;I/O BD JVS;837-13551 ;Ver1.00;98/10";

static void Ctor(PMICE_JVS this) {
    _MiceJvsBase_vftable.ctor(this);
    this->vftable = &_MiceJvs837_14572_vftable;
    this->vftable->readAllButtons = _MiceJvsBase_vftable.readAllButtons;

    this->m_BoardId = JVS_837_14572_ID;
    this->m_CommandVersion = 0x13;
    this->m_JvsRevision = 0x20;
    this->m_VersionComm = 0x10;

    // Coin + Test + 13 switches per player
    if (this->m_ButtonsPerPlayer == 0) this->m_ButtonsPerPlayer = PLAYER_SWITCH_COUNT;
    if (this->m_Coins == 0) this->m_Coins = COIN_COUNTERS;
    if (this->m_Players == 0) this->m_Players = PLAYER_COUNT;

    if (this->m_NumButtons == 0)
        this->m_NumButtons = this->m_Coins + 1 + (this->m_ButtonsPerPlayer * this->m_Players);
    if (this->m_ButtonStates == NULL) {
        this->m_ButtonStates = malloc(this->m_NumButtons * sizeof *this->m_ButtonStates);
        ZeroMemory(this->m_ButtonStates, this->m_NumButtons * sizeof *this->m_ButtonStates);
    }
    if (this->m_Bindings == NULL) {
        this->m_Bindings = malloc(this->m_NumButtons * sizeof *this->m_Bindings);
        ZeroMemory(this->m_Bindings, this->m_NumButtons * sizeof *this->m_Bindings);
    }

    ZeroMemory(this->m_CoinCounts, sizeof this->m_CoinCounts);
}

static JVS_STATUS Transact(PMICE_JVS this, BYTE command) {
    switch (command) {
        case JVS_CMD_READ_SW:
            BYTE players = MiceJvsRead(this);
            BYTE switch_bytes = MiceJvsRead(this);
            if (players > this->m_Players || switch_bytes != SWITCH_BYTES) {
                MiceJvsWrite(this, JVS_REPORT_PARAM_INVALID);
                return JVS_STATUS_OK;
            }
            MiceJvsWrite(this, JVS_REPORT_OK);

            this->vftable->readAllButtons(this);

            // Test button
            MiceJvsWrite(this, this->m_ButtonStates[this->m_Coins] ? 0x80 : 0x00);

            for (int player = 0; player < players; player++) {
                unsigned short buttons = 0x0000;
                for (int j = 0; j < this->m_ButtonsPerPlayer; j++) {
                    buttons <<= 1;
                    if (this->m_ButtonStates[this->m_Coins + 1 +
                                             (this->m_ButtonsPerPlayer * player) + j]) {
                        buttons |= 1;
                    }
                }
                buttons <<= (16 - this->m_ButtonsPerPlayer);

                for (int i = switch_bytes - 1; i >= 0; i--)
                    MiceJvsWrite(this, (buttons >> (i * 8)) & 0xff);
            }
            return JVS_STATUS_OK;

        case JVS_CMD_READ_COIN:
            MiceJvsWrite(this, JVS_REPORT_OK);
            unsigned char coin_count;
            coin_count = MiceJvsRead(this);

            MiceInputPollDevices();
            for (int i = 0; i < this->m_Coins; i++)
                if (MiceInputGetButtonState(&this->m_Bindings[i])) this->m_CoinCounts[i]++;

            for (unsigned char slot = 0; slot < this->m_Coins; slot++) {
                MiceJvsWrite(this, (this->m_CoinCounts[slot] >> 8) & 0xff);
                MiceJvsWrite(this, this->m_CoinCounts[slot] & 0xff);
            }
            return JVS_STATUS_OK;

        case JVS_CMD_READ_ANALOGS:
            MiceJvsWrite(this, JVS_REPORT_OK);
            // TODO: Actually emulate these (super low priority)
            unsigned char analog_count;
            analog_count = MiceJvsRead(this);
            for (int i = analog_count; i > 0; i--) {
                MiceJvsWrite(this, 0xde);
                MiceJvsWrite(this, 0xad);
            }
            return JVS_STATUS_OK;

        case JVS_CMD_WRITE_GPIO1:
            MiceJvsWrite(this, JVS_REPORT_OK);
            unsigned char gpio_bytes;
            gpio_bytes = MiceJvsRead(this);
            for (int i = 0; i < gpio_bytes; i++) {
                unsigned char gpio_value;
                gpio_value = MiceJvsRead(this);

                if (i == 0) {
                    if (!!(gpio_value & 0x80) != coin_solenoid) {
                        coin_solenoid = !!(gpio_value & 0x80);
                        log_info(plfMxJvs, "Coin solenoid: %s",
                                 coin_solenoid ? "Locked" : "Unlocked");
                    }
                }

                // log_warning(plfMxJvs, "Unhandled GPIO write: *(%d) = %02x", i, gpio_value);
            }
            return JVS_STATUS_OK;
        case JVS_CMD_WRITE_GPIO2:
        case JVS_CMD_WRITE_GPIO3:
            MiceJvsWrite(this, JVS_REPORT_OK);
            unsigned char gpioByteIndex;
            unsigned char gpioByteData;
            gpioByteIndex = MiceJvsRead(this);
            gpioByteData = MiceJvsRead(this);
            log_warning(plfMxJvs, "GPIO%d Unhandled: [%02x]=%02x",
                        (command - JVS_CMD_WRITE_GPIO2) + 2, gpioByteIndex, gpioByteData);
            return JVS_STATUS_OK;

        case JVS_CMD_COIN_DECREASE:
            MiceJvsWrite(this, JVS_REPORT_OK);

            unsigned char slot;
            slot = MiceJvsRead(this);
            unsigned char cAmount[2];
            cAmount[0] = MiceJvsRead(this);
            cAmount[1] = MiceJvsRead(this);
            unsigned short sAmount = cAmount[0] << 8 | cAmount[1];
            // board->coin_counts[slot] -= sAmount;
            return JVS_STATUS_OK;

        default:
            return _MiceJvsBase_vftable.transact(this, command);
    }
}

MICE_JVS_vftable _MiceJvs837_14572_vftable = {
    .ctor = Ctor,
    .transact = Transact,
};
