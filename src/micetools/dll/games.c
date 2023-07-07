#include "common.h"
#include "devices/_devices.h"
#include "drivers/jvs_boards/jvs.h"
#include "input.h"
#include "key_config.h"

#define IS_GAME(match) (strncmp(game_id, #match, 4) == 0)

MICE_BUTTON_BINDING maimaiDefaultBindings[31] = {
    // Coins
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = '1' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = '2' },
    // Test
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    // 1P
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = 'E' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = 'W' },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = 'D' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = 'C' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = 'X' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = 'Z' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = 'A' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = 'Q' },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    // 2P
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = VK_NUMPAD9 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = VK_NUMPAD8 },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = VK_NUMPAD6 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = VK_NUMPAD3 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = VK_NUMPAD2 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = VK_NUMPAD1 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = VK_NUMPAD4 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = TRUE, .m_AsyncKey.m_Key = VK_NUMPAD7 },
    { .m_Type = MICE_BB_TYPE_UNBOUND },
};
MICE_BUTTON_BINDING apmDefaultBindings[31] = {
    // Coins
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_F1 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_F2 },
    // Test
    { .m_Type = MICE_BB_TYPE_UNBOUND },
    // 1P
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_RETURN },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = '*' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_UP },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_DOWN },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_LEFT },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_RIGHT },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY,
      .m_Invert = FALSE,
      .m_AsyncKey.m_Key = '1' },  // 1P Light
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY,
      .m_Invert = FALSE,
      .m_AsyncKey.m_Key = '2' },  // 1P Middle
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY,
      .m_Invert = FALSE,
      .m_AsyncKey.m_Key = '3' },  // 1P Heavy
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = '4' },  // 1P Exs
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = '5' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = '6' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY,
      .m_Invert = FALSE,
      .m_AsyncKey.m_Key = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY,
      .m_Invert = FALSE,
      .m_AsyncKey.m_Key = MICE_BB_TYPE_UNBOUND },

    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_BACK },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = '-' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_NUMPAD8 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_NUMPAD2 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_NUMPAD4 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = VK_NUMPAD6 },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = 'Q' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = 'W' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = 'E' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = 'R' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = 'T' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY, .m_Invert = FALSE, .m_AsyncKey.m_Key = 'Y' },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY,
      .m_Invert = FALSE,
      .m_AsyncKey.m_Key = MICE_BB_TYPE_UNBOUND },
    { .m_Type = MICE_BB_TYPE_GET_ASYNC_KEY,
      .m_Invert = FALSE,
      .m_AsyncKey.m_Key = MICE_BB_TYPE_UNBOUND },
};

void _MiceSetupMaimai() {
    MiceConfig.devices.com1 = "";
    MiceConfig.devices.com2 = "aime_tn32msec";
    MiceConfig.devices.com3 = "maitouch";
    MiceConfig.devices.com5 = "";
    MiceConfig.devices.com6 = "mailed";
    MiceConfig.devices.com7 = "";
    MiceConfig.devices.com8 = "mailed";

    // Maimai is going to keep checking COM5 and COM7; we don't care.
    MiceConfig.devices.add_idlers = false;

    MiceConfig.sysconf.dipsw = 0b01111000;
    MiceConfig.window.nosize = true;

    if (MiceConfig.keys.board_count == 0) MiceConfig.keys.board_count = 1;

    free(_MiceJvsBoards[0].m_Bindings);
    _MiceJvsBoards[0].m_Bindings = malloc(sizeof maimaiDefaultBindings);
    memcpy(_MiceJvsBoards[0].m_Bindings, maimaiDefaultBindings, sizeof maimaiDefaultBindings);
    _MiceJvsBoards[0].m_NumButtons = _countof(maimaiDefaultBindings);
    _MiceJvsBoards[0].m_Coins = 2;
    _MiceJvsBoards[0].m_Players = 2;
    _MiceJvsBoards[0].m_ButtonsPerPlayer = 14;
    free(_MiceJvsBoards[0].m_ButtonStates);
    _MiceJvsBoards[0].m_ButtonStates =
        malloc(_countof(maimaiDefaultBindings) * sizeof *_MiceJvsBoards[0].m_ButtonStates);

    // Maimai is going to check if these exist. They do, now.
    make_dirs(MiceIpcRelativePath("dev\\w\\maimai"));
    make_dirs(MiceIpcRelativePath("dev\\v\\maimai"));
    make_dirs(MiceIpcRelativePath("dev\\y\\maimai_deliver\\maimai"));
}
void _MiceSetupApm() {
    MiceConfig.devices.com1 = "";
    MiceConfig.devices.com2 = "";
    MiceConfig.devices.com3 = "";
    MiceConfig.devices.com5 = "";
    MiceConfig.devices.com6 = "";
    MiceConfig.devices.com7 = "";
    MiceConfig.devices.com8 = "";

    MiceConfig.sysconf.dipsw = 0b01000000;  // 720p

    if (MiceConfig.keys.board_count == 0) MiceConfig.keys.board_count = 1;

    free(_MiceJvsBoards[0].m_Bindings);
    _MiceJvsBoards[0].m_Bindings = malloc(sizeof apmDefaultBindings);
    memcpy(_MiceJvsBoards[0].m_Bindings, apmDefaultBindings, sizeof apmDefaultBindings);
    _MiceJvsBoards[0].m_NumButtons = _countof(apmDefaultBindings);
    _MiceJvsBoards[0].m_Coins = 2;
    _MiceJvsBoards[0].m_Players = 2;
    _MiceJvsBoards[0].m_ButtonsPerPlayer = 14;
    free(_MiceJvsBoards[0].m_ButtonStates);
    _MiceJvsBoards[0].m_ButtonStates =
        malloc(_countof(apmDefaultBindings) * sizeof *_MiceJvsBoards[0].m_ButtonStates);
}

void _MiceGotGameId(char game_id[4]) {
    static bool gotGameId = false;
    if (gotGameId) return;
    if (game_id[0] != 'S') return;
    if (game_id[1] < 'A' || game_id[1] > 'Z') return;
    if (game_id[2] < 'A' || game_id[2] > 'Z') return;
    if (game_id[3] < 'A' || game_id[3] > 'Z') return;
    gotGameId = true;

    if (!MiceConfig.devices.do_auto) return;

    if (IS_GAME(SBRZ /* maimai */) || IS_GAME(SBXL /* maimai [PLUS] */) ||
        IS_GAME(SBZF /* maimai GreeN [PLUS]  */) || IS_GAME(SDBM /* maimai ORANGE [PLUS] */) ||
        IS_GAME(SDCQ /* maimai PiNK [PLUS] */) || IS_GAME(SDDK /* maimai MURASAKi [PLUS] */) ||
        IS_GAME(SDDZ /* maimai MiLK [PLUS] */) || IS_GAME(SDEY /* maimai FiNALE */) ||
        IS_GAME(SBYC /* maimai china */)) {
        log_info(plfBoot, "Detect game MaiMai");
        _MiceSetupMaimai();
    } else if (IS_GAME(SBYG /* APM2 */) || IS_GAME(SDCM /* UNIB */)) {
        log_info(plfBoot, "Detect game APM2");

        _MiceSetupApm();

        // MiceConfig.devices.com1 = "";
        // MiceConfig.devices.com2 = "";
        // MiceConfig.devices.com3 = "";
        // MiceConfig.devices.com5 = "";
        // MiceConfig.devices.com6 = "";
        // MiceConfig.devices.com7 = "";
        // MiceConfig.devices.com8 = "";

        // if (MiceConfig.keys.board_count == 0) MiceConfig.keys.board_count = 1;

        // if (!jvsKeybindings[0].notdefault) {
        //     puts("!!");

        //     // Zero all buttons
        //     memset(&jvsKeybindings[0].buttons[3 * 2], 0, 12 * 2);
        //     memset(&jvsKeybindings[0].invert[3 * 2], 0, 12 * 2);

        //     jvsKeybindings[0].buttons[0 * 2] = VK_BACK;
        //     jvsKeybindings[0].buttons[1 * 2] = VK_RETURN;

        //     jvsKeybindings[0].buttons[3 * 2] = VK_UP;
        //     jvsKeybindings[0].buttons[4 * 2] = VK_DOWN;
        //     jvsKeybindings[0].buttons[5 * 2] = VK_LEFT;
        //     jvsKeybindings[0].buttons[6 * 2] = VK_RIGHT;

        //     jvsKeybindings[0].buttons[7 * 2] = 'Q';
        //     jvsKeybindings[0].buttons[8 * 2] = 'W';
        //     jvsKeybindings[0].buttons[9 * 2] = 'E';
        //     jvsKeybindings[0].buttons[10 * 2] = 'A';
        //     jvsKeybindings[0].buttons[11 * 2] = 'S';
        //     jvsKeybindings[0].buttons[12 * 2] = 'D';

        //     // 2P is unbound by default, as a (nice) keymap with both
        //     // will probably also involve changing 1P too.
        // }
    } else if (IS_GAME(SBVH /* Sega & Sonic All-Stars Racing Arcade */)) {
        log_info(plfBoot, "Detect game Sega & Sonic All-Stars Racing Arcade");

        MiceConfig.devices.com1 = "servo_15069";
        MiceConfig.devices.com2 = "";
        MiceConfig.devices.com3 = "";
        MiceConfig.devices.com5 = "";
        MiceConfig.devices.com6 = "";
        MiceConfig.devices.com7 = "";
        MiceConfig.devices.com8 = "";
    } else if (IS_GAME(SBWU)) {
        log_info(plfBoot, "Detect game Gacchu Guts");
        MiceConfig.devices.com1 = "gacchu_guts_card";
        MiceConfig.devices.com2 = "gacchu_guts_screen";
        MiceConfig.devices.com3 = "gacchu_guts_screen";
        MiceConfig.devices.com5 = "";
        MiceConfig.devices.com6 = "";
        MiceConfig.devices.com7 = "";
        MiceConfig.devices.com8 = "";
    } else {
        if (game_id[0] != '\0') {
            log_warning(plfBoot, "Unknown game ID: %.4s", game_id);
            return;
        }
        return;
    }

    MiceConfig.devices.do_auto = false;

    save_current_config(true);
    start_devices();
}