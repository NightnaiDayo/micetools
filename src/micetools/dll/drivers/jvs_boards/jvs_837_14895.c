#include <Windows.h>

#include "../../common.h"
#include "jvs.h"

#define PLAYER_COUNT 1
#define COIN_COUNTERS 2
#define SWITCH_BYTES 2

BOOL coin_solenoid = false;

static const char JVS_837_14895_ID[] = ";;837-14895;;;FE MODE;";

static void Ctor(PMICE_JVS this) {
    _MiceJvsBase_vftable.ctor(this);
    this->vftable = &_MiceJvs837_14895_vftable;
    this->vftable->readAllButtons = _MiceJvsBase_vftable.readAllButtons;

    this->m_BoardId = JVS_837_14895_ID;
    this->m_CommandVersion = 0x13;
    this->m_JvsRevision = 0x30;
    this->m_VersionComm = 0x10;

    // Eeh whatever I'll do this eventually
    this->m_Players = 0;
    this->m_ButtonsPerPlayer = 0;
    this->m_Coins = 0;
    this->m_NumButtons = 0;
}

// TODO: Urgent: Restore buttons

static JVS_STATUS Transact(PMICE_JVS this, BYTE command) {
    switch (command) {
        case JVS_CMD_GET_FEATURES:
            MiceJvsWrite(this, JVS_REPORT_OK);

            MiceJvsWrite(this, JVS_FEATURE_PLAYERS);
            MiceJvsWrite(this, PLAYER_COUNT);
            MiceJvsWrite(this, 22);  // bits per player
            MiceJvsWrite(this, JVS_FEATURE_PAD);

            // We need at least 15 general outputs
            MiceJvsWrite(this, JVS_FEATURE_GPIO);
            MiceJvsWrite(this, 16);
            MiceJvsWrite(this, JVS_FEATURE_PAD);
            MiceJvsWrite(this, JVS_FEATURE_PAD);

            // Toss in two coin counters for good luck
            MiceJvsWrite(this, JVS_FEATURE_COINS);
            MiceJvsWrite(this, COIN_COUNTERS);
            MiceJvsWrite(this, JVS_FEATURE_PAD);
            MiceJvsWrite(this, JVS_FEATURE_PAD);

            MiceJvsWrite(this, JVS_FEATURE_EOF);
            return JVS_STATUS_OK;

        case JVS_CMD_READ_SW:
            unsigned char players;
            unsigned char switch_bytes;
            players = MiceJvsRead(this);
            switch_bytes = MiceJvsRead(this);
            if (players > PLAYER_COUNT || switch_bytes != 3) {
                MiceJvsWrite(this, JVS_REPORT_PARAM_INVALID);
                return JVS_STATUS_OK;
            }

            // update_jvs_buttons(board);

            // MiceJvsWrite(this, JVS_REPORT_OK);
            // MiceJvsWrite(this, board->last_sysbuttons);
            // for (int player = 0; player < players; player++) {
            //     for (int i = switch_bytes - 1; i >= 0; i--) {
            //         MiceJvsWrite(this, (board->last_buttons[player] >> (i * 8)) & 0xff);
            //     }
            // }
            MiceJvsWrite(this, JVS_REPORT_OK);
            MiceJvsWrite(this, 0);
            for (int player = 0; player < players; player++) {
                for (int i = switch_bytes - 1; i >= 0; i--) {
                    MiceJvsWrite(this, (0x00 >> (i * 8)) & 0xff);
                }
            }
            return JVS_STATUS_OK;

        case JVS_CMD_READ_COIN:
            MiceJvsWrite(this, JVS_REPORT_OK);
            unsigned char coin_count;
            coin_count = MiceJvsRead(this);

            // int coin1 = jvsKeybindings[board->num].buttons[0];
            // int coin2 = jvsKeybindings[board->num].buttons[1];

            // if (coin1 && GetAsyncKeyState(coin1) & 1) board->coin_counts[0]++;
            // if (coin2 && GetAsyncKeyState(coin2) & 1) board->coin_counts[1]++;

            // for (unsigned char slot = 0; slot < coin_count; slot++) {
            //     MiceJvsWrite(this, board->coin_counts[slot] >> 8);
            //     MiceJvsWrite(this, board->coin_counts[slot] & 0xff);
            // }

            for (unsigned char slot = 0; slot < coin_count; slot++) {
                MiceJvsWrite(this, 0);
                MiceJvsWrite(this, 0);
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

MICE_JVS_vftable _MiceJvs837_14895_vftable = {
    .ctor = Ctor,
    .transact = Transact,
};
