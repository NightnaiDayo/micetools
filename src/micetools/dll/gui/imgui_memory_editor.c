#pragma once
// Ported to C from
// https://github.com/ocornut/imgui_club/blob/master/imgui_memory_editor/imgui_memory_editor.h

#include "imgui_memory_editor.h"

// FIXME: We should have a way to retrieve the text edit cursor position
// more easily in the API, this is rather tedious. This is such a ugly mess
// we may be better off not using InputText() at all here.
static int UserData_Callback(ImGuiInputTextCallbackData* data) {
    UserData* user_data = (UserData*)data->UserData;
    if (!ImGuiInputTextCallbackData_HasSelection(data)) user_data->CursorPos = data->CursorPos;
    if (data->SelectionStart == 0 && data->SelectionEnd == data->BufTextLen) {
        // When not editing a byte, always refresh its InputText content
        // pulled from underlying memory data (this is a bit tricky, since
        // InputText technically "owns" the master copy of the buffer we
        // edit it in there)
        ImGuiInputTextCallbackData_DeleteChars(data, 0, data->BufTextLen);
        ImGuiInputTextCallbackData_InsertChars(data, 0, user_data->CurrentBufOverwrite, NULL);
        data->SelectionStart = 0;
        data->SelectionEnd = 2;
        data->CursorPos = 0;
    }
    return 0;
}

void MemoryEditor_Init(MemoryEditor* editor) {
    // Settings
    editor->Open = true;
    editor->ReadOnly = false;
    editor->Cols = 16;
    editor->OptShowOptions = true;
    editor->OptShowDataPreview = false;
    editor->OptShowHexII = false;
    editor->OptShowAscii = true;
    editor->OptGreyOutZeroes = true;
    editor->OptUpperCaseHex = true;
    editor->OptMidColsCount = 8;
    editor->OptAddrDigitsCount = 0;
    editor->OptFooterExtraHeight = 0.0f;
    editor->HighlightColor = IM_COL32(255, 255, 255, 50);
    editor->ReadFn = NULL;
    editor->WriteFn = NULL;
    editor->HighlightFn = NULL;

    // State/Internals
    editor->ContentsWidthChanged = false;
    editor->DataPreviewAddr = editor->DataEditingAddr = (size_t)-1;
    editor->DataEditingTakeFocus = false;
    memset(editor->DataInputBuf, 0, sizeof(editor->DataInputBuf));
    memset(editor->AddrInputBuf, 0, sizeof(editor->AddrInputBuf));
    editor->GotoAddr = (size_t)-1;
    editor->HighlightMin = editor->HighlightMax = (size_t)-1;
    editor->PreviewEndianess = 0;
    editor->PreviewDataType = ImGuiDataType_S32;
}

void MemoryEditor_GotoAddrAndHighlight(MemoryEditor* editor, size_t addr_min, size_t addr_max) {
    editor->GotoAddr = addr_min;
    editor->HighlightMin = addr_min;
    editor->HighlightMax = addr_max;
}

void MemoryEditor_CalcSizes(MemoryEditor* editor, Sizes* s, size_t mem_size,
                            size_t base_display_addr) {
    ImGuiStyle* style = igGetStyle();
    s->AddrDigitsCount = editor->OptAddrDigitsCount;
    if (s->AddrDigitsCount == 0)
        for (size_t n = base_display_addr + mem_size - 1; n > 0; n >>= 4) s->AddrDigitsCount++;
    s->LineHeight = igGetTextLineHeight();
    ImVec2 textSize;
    igCalcTextSize(&textSize, "F", NULL, false, -1.0f);
    s->GlyphWidth = textSize.x + 1;  // We assume the font is mono-space
    s->HexCellWidth =
        (float)(int)(s->GlyphWidth * 2.5f);  // "FF " we include trailing space in the
                                             // width to easily catch clicks everywhere
    s->SpacingBetweenMidCols =
        (float)(int)(s->HexCellWidth *
                     0.25f);  // Every OptMidColsCount columns we add a bit of extra spacing
    s->PosHexStart = (s->AddrDigitsCount + 2) * s->GlyphWidth;
    s->PosHexEnd = s->PosHexStart + (s->HexCellWidth * editor->Cols);
    s->PosAsciiStart = s->PosAsciiEnd = s->PosHexEnd;
    if (editor->OptShowAscii) {
        s->PosAsciiStart = s->PosHexEnd + s->GlyphWidth * 1;
        if (editor->OptMidColsCount > 0)
            s->PosAsciiStart +=
                (float)((editor->Cols + editor->OptMidColsCount - 1) / editor->OptMidColsCount) *
                s->SpacingBetweenMidCols;
        s->PosAsciiEnd = s->PosAsciiStart + editor->Cols * s->GlyphWidth;
    }
    s->WindowWidth =
        s->PosAsciiEnd + style->ScrollbarSize + style->WindowPadding.x * 2 + s->GlyphWidth;
}

// Standalone Memory Editor window
void MemoryEditor_DrawWindow(MemoryEditor* editor, const char* title, void* mem_data,
                             size_t mem_size, size_t base_display_addr) {
    Sizes s;
    MemoryEditor_CalcSizes(editor, &s, mem_size, base_display_addr);

    ImVec2 vec1, vec2;
    vec1.x = s.WindowWidth;
    vec1.y = s.WindowWidth * 0.60f;
    igSetNextWindowSize(vec1, ImGuiCond_FirstUseEver);
    vec1.x = 0.0f;
    vec1.y = 0.0f;
    vec2.x = s.WindowWidth;
    vec2.y = FLT_MAX;
    igSetNextWindowSizeConstraints(vec1, vec2, NULL, NULL);

    editor->Open = true;
    if (igBegin(title, &editor->Open, ImGuiWindowFlags_NoScrollbar)) {
        if (igIsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
            igIsMouseReleased_Nil(ImGuiMouseButton_Right))
            igOpenPopup_Str("context", 0);
        MemoryEditor_DrawContents(editor, mem_data, mem_size, base_display_addr);
        if (editor->ContentsWidthChanged) {
            MemoryEditor_CalcSizes(editor, &s, mem_size, base_display_addr);
            igGetWindowSize(&vec1);
            vec1.x = s.WindowWidth;
            igSetWindowSize_Vec2(vec1, 0);
        }
    }
    igEnd();
}

// Memory Editor contents only
void MemoryEditor_DrawContents(MemoryEditor* editor, void* mem_data_void, size_t mem_size,
                               size_t base_display_addr) {
    if (editor->Cols < 1) editor->Cols = 1;

    ImU8* mem_data = (ImU8*)mem_data_void;
    Sizes s;
    MemoryEditor_CalcSizes(editor, &s, mem_size, base_display_addr);
    ImGuiStyle* style = igGetStyle();

    // We begin into our scrolling region with the 'ImGuiWindowFlags_NoMove' in order to prevent
    // click from moving the window. This is used as a facility since our main click detection code
    // doesn't assign an ActiveId so the click would normally be caught as a window-move.
    const float height_separator = style->ItemSpacing.y;
    float footer_height = editor->OptFooterExtraHeight;
    if (editor->OptShowOptions)
        footer_height += height_separator + igGetFrameHeightWithSpacing() * 1;
    if (editor->OptShowDataPreview)
        footer_height += height_separator + igGetFrameHeightWithSpacing() * 1 +
                         igGetTextLineHeightWithSpacing() * 3;

    ImVec2 vec1, vec2;
    vec1.x = 0;
    vec1.y = -footer_height;
    igBeginChild_Str("##scrolling", vec1, false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
    ImDrawList* draw_list = igGetWindowDrawList();

    vec1.y = 0;
    igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, vec1);
    igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, vec1);

    // We are not really using the clipper API correctly here, because we rely on
    // visible_start_addr/visible_end_addr for our scrolling function.
    const int line_total_count = (int)((mem_size + editor->Cols - 1) / editor->Cols);
    ImGuiListClipper clipper;
    ImGuiListClipper_Begin(&clipper, line_total_count, s.LineHeight);

    bool data_next = false;

    if (editor->ReadOnly || editor->DataEditingAddr >= mem_size)
        editor->DataEditingAddr = (size_t)-1;
    if (editor->DataPreviewAddr >= mem_size) editor->DataPreviewAddr = (size_t)-1;

    size_t preview_data_type_size =
        editor->OptShowDataPreview ? MemoryEditor_DataTypeGetSize(editor->PreviewDataType) : 0;

    size_t data_editing_addr_next = (size_t)-1;
    if (editor->DataEditingAddr != (size_t)-1) {
        // Move cursor but only apply on next frame so scrolling with be synchronized (because
        // currently we can't change the scrolling while the window is being rendered)
        if (igIsKeyPressed_Bool(igGetKeyIndex(ImGuiKey_UpArrow), true) &&
            (ptrdiff_t)editor->DataEditingAddr >= (ptrdiff_t)editor->Cols) {
            data_editing_addr_next = editor->DataEditingAddr - editor->Cols;
        } else if (igIsKeyPressed_Bool(igGetKeyIndex(ImGuiKey_DownArrow), true) &&
                   (ptrdiff_t)editor->DataEditingAddr < (ptrdiff_t)mem_size - editor->Cols) {
            data_editing_addr_next = editor->DataEditingAddr + editor->Cols;
        } else if (igIsKeyPressed_Bool(igGetKeyIndex(ImGuiKey_LeftArrow), true) &&
                   (ptrdiff_t)editor->DataEditingAddr > (ptrdiff_t)0) {
            data_editing_addr_next = editor->DataEditingAddr - 1;
        } else if (igIsKeyPressed_Bool(igGetKeyIndex(ImGuiKey_RightArrow), true) &&
                   (ptrdiff_t)editor->DataEditingAddr < (ptrdiff_t)mem_size - 1) {
            data_editing_addr_next = editor->DataEditingAddr + 1;
        }
    }

    // Draw vertical separator
    ImVec2 window_pos;
    igGetWindowPos(&window_pos);
    if (editor->OptShowAscii) {
        vec1.x = window_pos.x + s.PosAsciiStart - s.GlyphWidth;
        vec1.y = window_pos.y;
        vec2.x = window_pos.x + s.PosAsciiStart - s.GlyphWidth;
        vec2.y = window_pos.y + 9999;
        ImDrawList_AddLine(draw_list, vec1, vec2, igGetColorU32_Col(ImGuiCol_Border, 1.0f), 1);
    }

    const ImU32 color_text = igGetColorU32_Col(ImGuiCol_Text, 1.0f);
    const ImU32 color_disabled =
        editor->OptGreyOutZeroes ? igGetColorU32_Col(ImGuiCol_TextDisabled, 1.0f) : color_text;

    const char* format_address =
        editor->OptUpperCaseHex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";
    const char* format_data = editor->OptUpperCaseHex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
    const char* format_byte = editor->OptUpperCaseHex ? "%02X" : "%02x";
    const char* format_byte_space = editor->OptUpperCaseHex ? "%02X " : "%02x ";

    while (ImGuiListClipper_Step(&clipper))
        for (int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd;
             line_i++)  // display only visible lines
        {
            size_t addr = (size_t)(line_i * editor->Cols);
            igText(format_address, s.AddrDigitsCount, base_display_addr + addr);

            // Draw Hexadecimal
            for (int n = 0; n < editor->Cols && addr < mem_size; n++, addr++) {
                float byte_pos_x = s.PosHexStart + s.HexCellWidth * n;
                if (editor->OptMidColsCount > 0)
                    byte_pos_x += (float)(n / editor->OptMidColsCount) * s.SpacingBetweenMidCols;
                igSameLine(byte_pos_x, -1.0f);

                // Draw highlight
                bool is_highlight_from_user_range =
                    (addr >= editor->HighlightMin && addr < editor->HighlightMax);
                bool is_highlight_from_user_func =
                    (editor->HighlightFn && editor->HighlightFn(mem_data, addr));
                bool is_highlight_from_preview =
                    (addr >= editor->DataPreviewAddr &&
                     addr < editor->DataPreviewAddr + preview_data_type_size);
                if (is_highlight_from_user_range || is_highlight_from_user_func ||
                    is_highlight_from_preview) {
                    ImVec2 pos;
                    igGetCursorScreenPos(&pos);
                    float highlight_width = s.GlyphWidth * 2;
                    bool is_next_byte_highlighted =
                        (addr + 1 < mem_size) &&
                        ((editor->HighlightMax != (size_t)-1 && addr + 1 < editor->HighlightMax) ||
                         (editor->HighlightFn && editor->HighlightFn(mem_data, addr + 1)));
                    if (is_next_byte_highlighted || (n + 1 == editor->Cols)) {
                        highlight_width = s.HexCellWidth;
                        if (editor->OptMidColsCount > 0 && n > 0 && (n + 1) < editor->Cols &&
                            ((n + 1) % editor->OptMidColsCount) == 0)
                            highlight_width += s.SpacingBetweenMidCols;
                    }

                    vec1.x = pos.x = highlight_width;
                    vec1.y = pos.y + s.LineHeight;
                    ImDrawList_AddRectFilled(draw_list, pos, vec1, editor->HighlightColor, 0, 0);
                }

                if (editor->DataEditingAddr == addr) {
                    // Display text input on current byte
                    bool data_write = false;
                    igPushID_Ptr((void*)addr);
                    if (editor->DataEditingTakeFocus) {
                        igSetKeyboardFocusHere(0);
                        sprintf(editor->AddrInputBuf, format_data, s.AddrDigitsCount,
                                base_display_addr + addr);
                        sprintf(editor->DataInputBuf, format_byte,
                                editor->ReadFn ? editor->ReadFn(mem_data, addr) : mem_data[addr]);
                    }
                    UserData user_data;
                    user_data.CursorPos = -1;
                    sprintf(user_data.CurrentBufOverwrite, format_byte,
                            editor->ReadFn ? editor->ReadFn(mem_data, addr) : mem_data[addr]);
                    ImGuiInputTextFlags flags =
                        ImGuiInputTextFlags_CharsHexadecimal |
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll |
                        ImGuiInputTextFlags_NoHorizontalScroll |
                        ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AlwaysOverwrite;
                    igSetNextItemWidth(s.GlyphWidth * 2);
                    if (igInputText("##data", editor->DataInputBuf,
                                    IM_ARRAYSIZE(editor->DataInputBuf), flags, UserData_Callback,
                                    &user_data))
                        data_write = data_next = true;
                    else if (!editor->DataEditingTakeFocus && !igIsItemActive())
                        editor->DataEditingAddr = data_editing_addr_next = (size_t)-1;
                    editor->DataEditingTakeFocus = false;
                    if (user_data.CursorPos >= 2) data_write = data_next = true;
                    if (data_editing_addr_next != (size_t)-1) data_write = data_next = false;
                    unsigned int data_input_value = 0;
                    if (data_write && sscanf(editor->DataInputBuf, "%X", &data_input_value) == 1) {
                        if (editor->WriteFn)
                            editor->WriteFn(mem_data, addr, (ImU8)data_input_value);
                        else
                            mem_data[addr] = (ImU8)data_input_value;
                    }
                    igPopID();
                } else {
                    // NB: The trailing space is not visible but ensure there's no gap that the
                    // mouse cannot click on.
                    ImU8 b = editor->ReadFn ? editor->ReadFn(mem_data, addr) : mem_data[addr];

                    if (editor->OptShowHexII) {
                        if ((b >= 32 && b < 128))
                            igText(".%c ", b);
                        else if (b == 0xFF && editor->OptGreyOutZeroes)
                            igTextDisabled("## ");
                        else if (b == 0x00)
                            igText("   ");
                        else
                            igText(format_byte_space, b);
                    } else {
                        if (b == 0 && editor->OptGreyOutZeroes)
                            igTextDisabled("00 ");
                        else
                            igText(format_byte_space, b);
                    }
                    if (!editor->ReadOnly && igIsItemHovered(0) && igIsMouseClicked_Bool(0, false)) {
                        editor->DataEditingTakeFocus = true;
                        data_editing_addr_next = addr;
                    }
                }
            }

            if (editor->OptShowAscii) {
                // Draw ASCII values
                igSameLine(s.PosAsciiStart, -1.0f);
                ImVec2 pos;
                igGetCursorScreenPos(&pos);
                addr = line_i * editor->Cols;
                igPushID_Int(line_i);
                vec1.x = s.PosAsciiEnd - s.PosAsciiStart;
                vec1.y = s.LineHeight;
                if (igInvisibleButton("igInvisibleButtonascii", vec1, 0)) {
                    editor->DataEditingAddr = editor->DataPreviewAddr =
                        addr + (size_t)((igGetIO()->MousePos.x - pos.x) / s.GlyphWidth);
                    editor->DataEditingTakeFocus = true;
                }
                igPopID();
                for (int n = 0; n < editor->Cols && addr < mem_size; n++, addr++) {
                    if (addr == editor->DataEditingAddr) {
                        vec1.x = pos.x + s.GlyphWidth;
                        vec1.y = pos.y + s.LineHeight;
                        ImDrawList_AddRectFilled(draw_list, pos, vec1,
                                                 igGetColorU32_Col(ImGuiCol_FrameBg, 1.0f), 0, 0);
                        ImDrawList_AddRectFilled(draw_list, pos, vec1,
                                                 igGetColorU32_Col(ImGuiCol_TextSelectedBg, 1.0f), 0, 0);
                    }
                    unsigned char c =
                        editor->ReadFn ? editor->ReadFn(mem_data, addr) : mem_data[addr];
                    char display_c = (c < 32 || c >= 128) ? '.' : c;

                    ImDrawList_AddText_Vec2(draw_list, pos,
                                            (display_c == c) ? color_text : color_disabled,
                                            &display_c, &display_c + 1);
                    pos.x += s.GlyphWidth;
                }
            }
        }
    igPopStyleVar(2);
    igEndChild();

    // Notify the main window of our ideal child content size (FIXME: we are missing an API to get
    // the contents size from the child)
    igSetCursorPosX(s.WindowWidth);

    if (data_next && editor->DataEditingAddr + 1 < mem_size) {
        editor->DataEditingAddr = editor->DataPreviewAddr = editor->DataEditingAddr + 1;
        editor->DataEditingTakeFocus = true;
    } else if (data_editing_addr_next != (size_t)-1) {
        editor->DataEditingAddr = editor->DataPreviewAddr = data_editing_addr_next;
        editor->DataEditingTakeFocus = true;
    }

    const bool lock_show_data_preview = editor->OptShowDataPreview;
    if (editor->OptShowOptions) {
        igSeparator();
        MemoryEditor_DrawOptionsLine(editor, &s, mem_data, mem_size, base_display_addr);
    }

    if (lock_show_data_preview) {
        igSeparator();
        MemoryEditor_DrawPreviewLine(editor, &s, mem_data, mem_size, base_display_addr);
    }
}

void MemoryEditor_DrawOptionsLine(MemoryEditor* editor, const Sizes* s, void* mem_data,
                                  size_t mem_size, size_t base_display_addr) {
    IM_UNUSED(mem_data);
    ImGuiStyle* style = igGetStyle();
    const char* format_range = editor->OptUpperCaseHex
                                   ? "Range %0*" _PRISizeT "X..%0*" _PRISizeT "X"
                                   : "Range %0*" _PRISizeT "x..%0*" _PRISizeT "x";

    ImVec2 vec1 = { 0 };
    // Options menu
    if (igButton("Options", vec1)) igOpenPopup_Str("context", 0);
    if (igBeginPopup("context", 0)) {
        igSetNextItemWidth(s->GlyphWidth * 7 + style->FramePadding.x * 2.0f);
        if (igDragInt("##cols", &editor->Cols, 0.2f, 4, 32, "%d cols", 0)) {
            editor->ContentsWidthChanged = true;
            if (editor->Cols < 1) editor->Cols = 1;
        }
        igCheckbox("Show Data Preview", &editor->OptShowDataPreview);
        igCheckbox("Show HexII", &editor->OptShowHexII);
        if (igCheckbox("Show Ascii", &editor->OptShowAscii)) {
            editor->ContentsWidthChanged = true;
        }
        igCheckbox("Grey out zeroes", &editor->OptGreyOutZeroes);
        igCheckbox("Uppercase Hex", &editor->OptUpperCaseHex);

        igEndPopup();
    }

    igSameLine(0, -1);
    igText(format_range, s->AddrDigitsCount, base_display_addr, s->AddrDigitsCount,
           base_display_addr + mem_size - 1);
    igSameLine(0, -1);
    igSetNextItemWidth((s->AddrDigitsCount + 1) * s->GlyphWidth + style->FramePadding.x * 2.0f);
    if (igInputText("##addr", editor->AddrInputBuf, IM_ARRAYSIZE(editor->AddrInputBuf),
                    ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue,
                    NULL, NULL)) {
        size_t goto_addr;
        if (sscanf(editor->AddrInputBuf, "%" _PRISizeT "X", &goto_addr) == 1) {
            editor->GotoAddr = goto_addr - base_display_addr;
            editor->HighlightMin = editor->HighlightMax = (size_t)-1;
        }
    }

    if (editor->GotoAddr != (size_t)-1) {
        if (editor->GotoAddr < mem_size) {
            vec1.x = 0;
            vec1.y = 0;
            igBeginChild_Str("##scrolling", vec1, false, 0);
            igGetCursorStartPos(&vec1);
            igSetScrollFromPosY_Float(
                vec1.y + (editor->GotoAddr / editor->Cols) * igGetTextLineHeight(), 0.5f);
            igEndChild();
            editor->DataEditingAddr = editor->DataPreviewAddr = editor->GotoAddr;
            editor->DataEditingTakeFocus = true;
        }
        editor->GotoAddr = (size_t)-1;
    }
}

void MemoryEditor_DrawPreviewLine(MemoryEditor* editor, const Sizes* s, void* mem_data_void,
                                  size_t mem_size, size_t base_display_addr) {
    IM_UNUSED(base_display_addr);
    ImVec2 vec1 = { 0 };
    ImU8* mem_data = (ImU8*)mem_data_void;
    ImGuiStyle* style = igGetStyle();
    igAlignTextToFramePadding();
    igText("Preview as:");
    igSameLine(0, -1);
    igSetNextItemWidth((s->GlyphWidth * 10.0f) + style->FramePadding.x * 2.0f +
                       style->ItemInnerSpacing.x);
    if (igBeginCombo("##combo_type", MemoryEditor_DataTypeGetDesc(editor->PreviewDataType),
                     ImGuiComboFlags_HeightLargest)) {
        for (int n = 0; n < ImGuiDataType_COUNT; n++)
            if (igSelectable_Bool(MemoryEditor_DataTypeGetDesc((ImGuiDataType)n),
                                  editor->PreviewDataType == n, 0, vec1))
                editor->PreviewDataType = (ImGuiDataType)n;
        igEndCombo();
    }
    igSameLine(0, -1);
    igSetNextItemWidth((s->GlyphWidth * 6.0f) + style->FramePadding.x * 2.0f +
                       style->ItemInnerSpacing.x);
    igCombo_Str("##combo_endianess", &editor->PreviewEndianess, "LE\0BE\0\0", -1);

    char buf[128] = "";
    float x = s->GlyphWidth * 6.0f;
    bool has_value = editor->DataPreviewAddr != (size_t)-1;
    if (has_value)
        MemoryEditor_DrawPreviewData(editor, editor->DataPreviewAddr, mem_data, mem_size,
                                     editor->PreviewDataType, DataFormat_Dec, buf,
                                     (size_t)IM_ARRAYSIZE(buf));
    igText("Dec");
    igSameLine(x, -1);
    igTextUnformatted(has_value ? buf : "N/A", NULL);
    if (has_value)
        MemoryEditor_DrawPreviewData(editor, editor->DataPreviewAddr, mem_data, mem_size,
                                     editor->PreviewDataType, DataFormat_Hex, buf,
                                     (size_t)IM_ARRAYSIZE(buf));
    igText("Hex");
    igSameLine(x, -1);
    igTextUnformatted(has_value ? buf : "N/A", NULL);
    if (has_value)
        MemoryEditor_DrawPreviewData(editor, editor->DataPreviewAddr, mem_data, mem_size,
                                     editor->PreviewDataType, DataFormat_Bin, buf,
                                     (size_t)IM_ARRAYSIZE(buf));
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    igText("Bin");
    igSameLine(x, -1);
    igTextUnformatted(has_value ? buf : "N/A", NULL);
}

// Utilities for Data Preview
const char* MemoryEditor_DataTypeGetDesc(ImGuiDataType data_type) {
    const char* descs[] = { "Int8",   "Uint8", "Int16",  "Uint16", "Int32",
                            "Uint32", "Int64", "Uint64", "Float",  "Double" };
    // IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    return descs[data_type];
}

size_t MemoryEditor_DataTypeGetSize(ImGuiDataType data_type) {
    const size_t sizes[] = { 1, 1, 2, 2, 4, 4, 8, 8, sizeof(float), sizeof(double) };
    // IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    return sizes[data_type];
}

const char* MemoryEditor_DataFormatGetDesc(DataFormat data_format) {
    const char* descs[] = { "Bin", "Dec", "Hex" };
    // IM_ASSERT(data_format >= 0 && data_format < DataFormat_COUNT);
    return descs[data_format];
}

bool MemoryEditor_IsBigEndian() {
    uint16_t x = 1;
    char c[2];
    memcpy(c, &x, 2);
    return c[0] != 0;
}

static void* MemoryEditor_EndianessCopyBigEndian(void* _dst, void* _src, size_t s,
                                                 int is_little_endian) {
    if (is_little_endian) {
        uint8_t* dst = (uint8_t*)_dst;
        uint8_t* src = (uint8_t*)_src + s - 1;
        for (int i = 0, n = (int)s; i < n; ++i) memcpy(dst++, src--, 1);
        return _dst;
    } else {
        return memcpy(_dst, _src, s);
    }
}

static void* MemoryEditor_EndianessCopyLittleEndian(void* _dst, void* _src, size_t s,
                                                    int is_little_endian) {
    if (is_little_endian) {
        return memcpy(_dst, _src, s);
    } else {
        uint8_t* dst = (uint8_t*)_dst;
        uint8_t* src = (uint8_t*)_src + s - 1;
        for (int i = 0, n = (int)s; i < n; ++i) memcpy(dst++, src--, 1);
        return _dst;
    }
}

void* MemoryEditor_EndianessCopy(MemoryEditor* editor, void* dst, void* src, size_t size) {
    static void* (*fp)(void*, void*, size_t, int) = NULL;
    if (fp == NULL)
        fp = MemoryEditor_IsBigEndian() ? MemoryEditor_EndianessCopyBigEndian
                                        : MemoryEditor_EndianessCopyLittleEndian;
    return fp(dst, src, size, editor->PreviewEndianess);
}

const char* MemoryEditor_FormatBinary(const uint8_t* buf, int width) {
    // IM_ASSERT(width <= 64);
    size_t out_n = 0;
    static char out_buf[64 + 8 + 1];
    int n = width / 8;
    for (int j = n - 1; j >= 0; --j) {
        for (int i = 0; i < 8; ++i) out_buf[out_n++] = (buf[j] & (1 << (7 - i))) ? '1' : '0';
        out_buf[out_n++] = ' ';
    }
    // IM_ASSERT(out_n < IM_ARRAYSIZE(out_buf));
    out_buf[out_n] = 0;
    return out_buf;
}

// [Internal]
void MemoryEditor_DrawPreviewData(MemoryEditor* editor, size_t addr, const ImU8* mem_data,
                                  size_t mem_size, ImGuiDataType data_type, DataFormat data_format,
                                  char* out_buf, size_t out_buf_size) {
    uint8_t buf[8];
    size_t elem_size = MemoryEditor_DataTypeGetSize(data_type);
    size_t size = addr + elem_size > mem_size ? mem_size - addr : elem_size;
    if (editor->ReadFn)
        for (int i = 0, n = (int)size; i < n; ++i) buf[i] = editor->ReadFn(mem_data, addr + i);
    else
        memcpy(buf, mem_data + addr, size);

    if (data_format == DataFormat_Bin) {
        uint8_t binbuf[8];
        MemoryEditor_EndianessCopy(editor, binbuf, buf, size);
        ImSnprintf(out_buf, out_buf_size, "%s", MemoryEditor_FormatBinary(binbuf, (int)size * 8));
        return;
    }

    out_buf[0] = 0;
    switch (data_type) {
        case ImGuiDataType_S8: {
            int8_t int8 = 0;
            MemoryEditor_EndianessCopy(editor, &int8, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%hhd", int8);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%02x", int8 & 0xFF);
                return;
            }
            break;
        }
        case ImGuiDataType_U8: {
            uint8_t uint8 = 0;
            MemoryEditor_EndianessCopy(editor, &uint8, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%hhu", uint8);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%02x", uint8 & 0XFF);
                return;
            }
            break;
        }
        case ImGuiDataType_S16: {
            int16_t int16 = 0;
            MemoryEditor_EndianessCopy(editor, &int16, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%hd", int16);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%04x", int16 & 0xFFFF);
                return;
            }
            break;
        }
        case ImGuiDataType_U16: {
            uint16_t uint16 = 0;
            MemoryEditor_EndianessCopy(editor, &uint16, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%hu", uint16);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%04x", uint16 & 0xFFFF);
                return;
            }
            break;
        }
        case ImGuiDataType_S32: {
            int32_t int32 = 0;
            MemoryEditor_EndianessCopy(editor, &int32, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%d", int32);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%08x", int32);
                return;
            }
            break;
        }
        case ImGuiDataType_U32: {
            uint32_t uint32 = 0;
            MemoryEditor_EndianessCopy(editor, &uint32, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%u", uint32);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%08x", uint32);
                return;
            }
            break;
        }
        case ImGuiDataType_S64: {
            int64_t int64 = 0;
            MemoryEditor_EndianessCopy(editor, &int64, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%lld", (long long)int64);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)int64);
                return;
            }
            break;
        }
        case ImGuiDataType_U64: {
            uint64_t uint64 = 0;
            MemoryEditor_EndianessCopy(editor, &uint64, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%llu", (long long)uint64);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)uint64);
                return;
            }
            break;
        }
        case ImGuiDataType_Float: {
            float float32 = 0.0f;
            MemoryEditor_EndianessCopy(editor, &float32, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%f", float32);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "%a", float32);
                return;
            }
            break;
        }
        case ImGuiDataType_Double: {
            double float64 = 0.0;
            MemoryEditor_EndianessCopy(editor, &float64, buf, size);
            if (data_format == DataFormat_Dec) {
                ImSnprintf(out_buf, out_buf_size, "%f", float64);
                return;
            }
            if (data_format == DataFormat_Hex) {
                ImSnprintf(out_buf, out_buf_size, "%a", float64);
                return;
            }
            break;
        }
        case ImGuiDataType_COUNT:
            break;
    }
}