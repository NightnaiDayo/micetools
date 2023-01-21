#pragma once
// Ported to C from
// https://github.com/ocornut/imgui_club/blob/master/imgui_memory_editor/imgui_memory_editor.h

#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#ifndef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#endif
#include "cimgui.h"

#ifdef _MSC_VER
#define _PRISizeT "I"
#define ImSnprintf _snprintf
#else
#define _PRISizeT "z"
#define ImSnprintf snprintf
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning( \
    disable : 4996)  // warning C4996: 'sprintf': This function or variable may be unsafe.
#endif

typedef enum DataFormat_ {
    DataFormat_Bin = 0,
    DataFormat_Dec = 1,
    DataFormat_Hex = 2,
    DataFormat_COUNT
} DataFormat;

typedef struct UserData_ {
    char CurrentBufOverwrite[3];  // Input
    int CursorPos;                // Output
} UserData;
// FIXME: We should have a way to retrieve the text edit cursor position
// more easily in the API, this is rather tedious. This is such a ugly mess
// we may be better off not using InputText() at all here.
static int UserData_Callback(ImGuiInputTextCallbackData* data);

typedef struct MemoryEditor_ {
    // Settings
    bool Open;      // = true   // set to false when DrawWindow() was closed. ignore if not using
                    // DrawWindow().
    bool ReadOnly;  // = false  // disable any editing.
    int Cols;       // = 16     // number of columns to display.
    bool OptShowOptions;  // = true   // display options button/context menu. when disabled, options
                          // will be locked unless you provide your own UI for them.
    bool
        OptShowDataPreview;  // = false  // display a footer previewing the decimal/binary/hex/float
                             // representation of the currently selected bytes.
    bool OptShowHexII;      // = false  // display values in HexII representation instead of regular
                            // hexadecimal: hide null/zero bytes, ascii values as ".X".
    bool OptShowAscii;      // = true   // display ASCII representation on the right side.
    bool OptGreyOutZeroes;  // = true   // display null/zero bytes using the TextDisabled color.
    bool OptUpperCaseHex;   // = true   // display hexadecimal values as "FF" instead of "ff".
    int OptMidColsCount;    // = 8      // set to 0 to disable extra spacing between every mid-cols.
    int OptAddrDigitsCount;      // = 0      // number of addr digits to display (default calculated
                                 // based on maximum displayed addr).
    float OptFooterExtraHeight;  // = 0      // space to reserve at the bottom of the widget to add
                                 // custom widgets
    ImU32 HighlightColor;        //          // background color of highlighted bytes.
    ImU8 (*ReadFn)(const ImU8* data, size_t off);  // = 0      // optional handler to read bytes.
    void (*WriteFn)(ImU8* data, size_t off,
                    ImU8 d);  // = 0      // optional handler to write bytes.
    bool (*HighlightFn)(const ImU8* data,
                        size_t off);  //= 0      // optional handler to return Highlight property
                                      //(to support non-contiguous highlighting).

    // [Internal State]
    bool ContentsWidthChanged;
    size_t DataPreviewAddr;
    size_t DataEditingAddr;
    bool DataEditingTakeFocus;
    char DataInputBuf[32];
    char AddrInputBuf[32];
    size_t GotoAddr;
    size_t HighlightMin, HighlightMax;
    int PreviewEndianess;
    ImGuiDataType PreviewDataType;
} MemoryEditor;

void MemoryEditor_Init(MemoryEditor* editor);

void MemoryEditor_GotoAddrAndHighlight(MemoryEditor* editor, size_t addr_min, size_t addr_max);

typedef struct Sizes_ {
    int AddrDigitsCount;
    float LineHeight;
    float GlyphWidth;
    float HexCellWidth;
    float SpacingBetweenMidCols;
    float PosHexStart;
    float PosHexEnd;
    float PosAsciiStart;
    float PosAsciiEnd;
    float WindowWidth;
} Sizes;

void MemoryEditor_CalcSizes(MemoryEditor* editor, Sizes* s, size_t mem_size,
                            size_t base_display_addr);

// Standalone Memory Editor window
void MemoryEditor_DrawWindow(MemoryEditor* editor, const char* title, void* mem_data,
                             size_t mem_size, size_t base_display_addr);

// Memory Editor contents only
void MemoryEditor_DrawContents(MemoryEditor* editor, void* mem_data_void, size_t mem_size,
                               size_t base_display_addr);
void MemoryEditor_DrawOptionsLine(MemoryEditor* editor, const Sizes* s, void* mem_data,
                                  size_t mem_size, size_t base_display_addr);
void MemoryEditor_DrawPreviewLine(MemoryEditor* editor, const Sizes* s, void* mem_data_void,
                                  size_t mem_size, size_t base_display_addr);

// Utilities for Data Preview
const char* MemoryEditor_DataTypeGetDesc(ImGuiDataType data_type);
size_t MemoryEditor_DataTypeGetSize(ImGuiDataType data_type);
const char* MemoryEditor_DataFormatGetDesc(DataFormat data_format);
bool MemoryEditor_IsBigEndian(void);

static void* MemoryEditor_EndianessCopyBigEndian(void* _dst, void* _src, size_t s,
                                                 int is_little_endian);
static void* MemoryEditor_EndianessCopyLittleEndian(void* _dst, void* _src, size_t s,
                                                    int is_little_endian);
void* MemoryEditor_EndianessCopy(MemoryEditor* editor, void* dst, void* src, size_t size);
const char* MemoryEditor_FormatBinary(const uint8_t* buf, int width);
// [Internal]
void MemoryEditor_DrawPreviewData(MemoryEditor* editor, size_t addr, const ImU8* mem_data,
                                  size_t mem_size, ImGuiDataType data_type, DataFormat data_format,
                                  char* out_buf, size_t out_buf_size);