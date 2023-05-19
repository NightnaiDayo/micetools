#pragma once

#include <Windows.h>
#include <stdint.h>
#include "nfc/nfc.h"

enum TN32Error {
    TN32Error_OK,
    TN32Error_CardError,
    TN32Error_NoAccept,
    TN32Error_InvalidCommand,
    TN32Error_InvalidData,
    TN32Error_SumError,
    TN32Error_HexError,
};
typedef BYTE TN32Error;
enum TN32ErrorLED {
    TN32ErrorLED_OK,
    TN32ErrorLED_CommandUnknown,
    TN32ErrorLED_OverflowErr,
    TN32ErrorLED_ParityErr,
    TN32ErrorLED_OverrunErr,
    TN32ErrorLED_FramingErr,
    TN32ErrorLED_SumErr,
    TN32ErrorLED_ParameterErr,
    TN32ErrorLED_HexErr,
};
typedef BYTE TN32ErrorLED;

/**
 * 51 and 55 are guesses based on the code. Makes sense given their opcodes
 * too.
 * TODO: Validate against a real board
 */
enum TN32_Opcode {
    TN32Op_GetFWVersion = 0x30,
    TN32Op_GetHWVersion = 0x32,

    TN32Op_RadioOn = 0x40,
    TN32Op_RadioOff = 0x41,
    TN32Op_CardDetect = 0x42,
    TN32Op_CardSelect = 0x43,
    TN32Op_CardHalt = 0x44,

    TN32Op_MifareKeySetA = 0x50,  // BNG key
    TN32Op_MifareAuthorizeA = 0x51,
    TN32Op_MifareRead = 0x52,
    TN32Op_MifareWrite = 0x53,
    TN32Op_MifareKeySetB = 0x54,  // AiMe key
    TN32Op_MifareAuthorizeB = 0x55,

    // Firmware update
    TN32Op_EnterUpdaterMode = 0x60,
    TN32Op_SendHex = 0x61,
    TN32Op_Reset = 0x62,
    TN32Op_SendBindata = 0x63,
    TN32Op_BindataExec = 0x64,

    // Felica
    TN32Op_FelicaPush = 0x70,
    TN32Op_FelicaEncap = 0x71,

    // No responses to either
    Led_SetColour = 0x80,
    Led_SetColourRGB = 0x81,

    Led_GetInfo = 0xf0,
    Led_FirmwareSum = 0xf2,
    Led_SendHex = 0xf3,
    Led_EnterBootMode = 0xf4,
    Led_Reset = 0xf5,
};
