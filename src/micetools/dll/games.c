#include "common.h"
#include "devices/_devices.h"
#include "key_config.h"

#define IS_GAME(match) (strncmp(game_id, #match, 4) == 0)

void mice_got_game_id(char game_id[4]) {
    static char last_game_id[4] = { '_', '_', '_', '_' };

    if (memcmp(game_id, last_game_id, 4) == 0) return;
    memcpy(last_game_id, game_id, 4);

    if (!MiceConfig.devices.do_auto) return;

    if (IS_GAME(SBRZ /* maimai */) || IS_GAME(SBXL /* maimai [PLUS] */) ||
        IS_GAME(SBZF /* maimai GreeN [PLUS]  */) || IS_GAME(SDBM /* maimai ORANGE [PLUS] */) ||
        IS_GAME(SDCQ /* maimai PiNK [PLUS] */) || IS_GAME(SDDK /* maimai MURASAKi [PLUS] */) ||
        IS_GAME(SDDZ /* maimai MiLK [PLUS] */) || IS_GAME(SDEY /* maimai FiNALE */) ||
        IS_GAME(SBYC /* maimai china */)) {
        log_info(plfBoot, "Detect game MaiMai");

        MiceConfig.devices.com1 = "";
        MiceConfig.devices.com2 = "aime_tn32msec";
        MiceConfig.devices.com3 = "maitouch";
        MiceConfig.devices.com5 = "";
        MiceConfig.devices.com6 = "mailed";
        MiceConfig.devices.com7 = "";
        MiceConfig.devices.com8 = "mailed";

        if (MiceConfig.keys.board_count == 0) MiceConfig.keys.board_count = 1;

        if (!jvsKeybindings[0].notdefault) {
            // Zero and invert all buttons
            memset(&jvsKeybindings[0].buttons[3 * 2], 0, 12 * 2);
            memset(&jvsKeybindings[0].invert[3 * 2], 1, 12 * 2);

            // Setup default keybinds
            jvsKeybindings[0].buttons[6 * 2] = 'A';   // 1P1
            jvsKeybindings[0].buttons[5 * 2] = 'Q';   // 1P2
            jvsKeybindings[0].buttons[8 * 2] = 'W';   // 1P3
            jvsKeybindings[0].buttons[9 * 2] = 'E';   // 1P4
            jvsKeybindings[0].buttons[10 * 2] = 'D';  // 1P5
            jvsKeybindings[0].buttons[11 * 2] = 'C';  // 1P6
            jvsKeybindings[0].buttons[12 * 2] = 'X';  // 1P7
            jvsKeybindings[0].buttons[13 * 2] = 'Z';  // 1P8

            jvsKeybindings[0].buttons[6 * 2 + 1] = VK_NUMPAD4;   // 2P1
            jvsKeybindings[0].buttons[5 * 2 + 1] = VK_NUMPAD7;   // 2P2
            jvsKeybindings[0].buttons[8 * 2 + 1] = VK_NUMPAD8;   // 2P3
            jvsKeybindings[0].buttons[9 * 2 + 1] = VK_NUMPAD9;   // 2P4
            jvsKeybindings[0].buttons[10 * 2 + 1] = VK_NUMPAD6;  // 2P5
            jvsKeybindings[0].buttons[11 * 2 + 1] = VK_NUMPAD3;  // 2P6
            jvsKeybindings[0].buttons[12 * 2 + 1] = VK_NUMPAD2;  // 2P7
            jvsKeybindings[0].buttons[13 * 2 + 1] = VK_NUMPAD1;  // 2P8
        }
    } else if (IS_GAME(SBYG /* APM2 */)) {
        log_info(plfBoot, "Detect game APM2");

        MiceConfig.devices.com1 = "";
        MiceConfig.devices.com2 = "";
        MiceConfig.devices.com3 = "";
        MiceConfig.devices.com5 = "";
        MiceConfig.devices.com6 = "";
        MiceConfig.devices.com7 = "";
        MiceConfig.devices.com8 = "";

        if (MiceConfig.keys.board_count == 0) MiceConfig.keys.board_count = 1;

        if (!jvsKeybindings[0].notdefault) {
            puts("!!");

            // Zero all buttons
            memset(&jvsKeybindings[0].buttons[3 * 2], 0, 12 * 2);
            memset(&jvsKeybindings[0].invert[3 * 2], 0, 12 * 2);

            jvsKeybindings[0].buttons[0 * 2] = VK_BACK;
            jvsKeybindings[0].buttons[1 * 2] = VK_RETURN;

            jvsKeybindings[0].buttons[3 * 2] = VK_UP;
            jvsKeybindings[0].buttons[4 * 2] = VK_DOWN;
            jvsKeybindings[0].buttons[5 * 2] = VK_LEFT;
            jvsKeybindings[0].buttons[6 * 2] = VK_RIGHT;

            jvsKeybindings[0].buttons[7 * 2] = 'Q';
            jvsKeybindings[0].buttons[8 * 2] = 'W';
            jvsKeybindings[0].buttons[9 * 2] = 'E';
            jvsKeybindings[0].buttons[10 * 2] = 'A';
            jvsKeybindings[0].buttons[11 * 2] = 'S';
            jvsKeybindings[0].buttons[12 * 2] = 'D';

            // 2P is unbound by default, as a (nice) keymap with both
            // will probably also involve changing 1P too.
        }
    } else if (IS_GAME(SBVH /* Sega & Sonic All-Stars Racing Arcade */)) {
        log_info(plfBoot, "Detect game Sega & Sonic All-Stars Racing Arcade");

        MiceConfig.devices.com1 = "servo_15069";
        MiceConfig.devices.com2 = "";
        MiceConfig.devices.com3 = "";
        MiceConfig.devices.com5 = "";
        MiceConfig.devices.com6 = "";
        MiceConfig.devices.com7 = "";
        MiceConfig.devices.com8 = "";
    } else {
        if (game_id[0] != '\0') return log_warning(plfBoot, "Unknown game ID: %.4s", game_id);
        return;
    }

    save_current_config();
    start_devices();
}