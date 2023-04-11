#include "patch.h"

#include <Windows.h>
#include <stdlib.h>
#include <string.h>

bool is_hex_char(char chr) {
    return ('0' <= chr && chr <= '9') || ('a' <= chr && chr <= 'f') || ('A' <= chr && chr <= 'F');
}

patch_t patch_sentinel = { .count = 0 };
patch_t* patch_list = &patch_sentinel;

void commit_patch(patch_t* patch) {
    if (!patch->count) return;

    patch_t* head = patch_list;
    while (head->next) head = head->next;
    head->next = patch;
    patch->next = NULL;
}

bool skip_whitespace(FILE* fp, char* cChr) {
    while (*cChr == ' ')
        if (!fread(cChr, 1, 1, fp)) return false;
    return true;
}

bool load_patches_file(const char* filename) {
    FILE* fp;
    if (fopen_s(&fp, filename, "r")) {
        printf("Failed to open %s\n", filename);
        return false;
    }

    parse_patches_state state = parse_patches_state_start;
    char readBuffer[256];
    size_t rbPtr = 0;

    char cChr;
    patch_t* patch = NULL;
    bool sol = true;
    while (1) {
        if (!fread(&cChr, 1, 1, fp)) goto patch_parser_eof;

        switch (state) {
            case parse_patches_state_start:
                if (cChr != '*' || !sol) {
                    if (cChr == '\r' || cChr == '\n')
                        sol = true;
                    else
                        sol = false;
                    continue;
                }

                patch = (patch_t*)malloc(sizeof *patch);
                ZeroMemory(patch, sizeof *patch);
                rbPtr = 0;

                if (!fread(&cChr, 1, 1, fp)) goto patch_parser_eof;
                state = parse_patches_state_addr;
                if (!skip_whitespace(fp, &cChr)) goto patch_parser_eof;
                // Falls through

            case parse_patches_state_addr:
                if (is_hex_char(cChr)) {
                    readBuffer[rbPtr++] = cChr;
                    break;
                }
                if (rbPtr != 8) {
                    printf("Invalid address length: %d\n", rbPtr);
                    return false;
                }
                hex_to_int(&(patch->address), readBuffer);

                state = parse_patches_state_colon;
                rbPtr = 0;
                if (!skip_whitespace(fp, &cChr)) goto patch_parser_eof;
                // Falls through

            case parse_patches_state_colon:
                if (cChr != ':') {
                    printf("Syntax Error: expected : saw %c\n", cChr);
                    return false;
                }
                if (!fread(&cChr, 1, 1, fp)) goto patch_parser_eof;
                state = parse_patches_state_match;
                if (!skip_whitespace(fp, &cChr)) goto patch_parser_eof;
                // Falls through

            case parse_patches_state_match:
                if (is_hex_char(cChr)) {
                    readBuffer[rbPtr++] = cChr;
                    break;
                }

                if (rbPtr % 2 != 0) {
                    printf("Match length must be even: saw %d\n", rbPtr);
                    return false;
                }

                if (rbPtr != 0) {
                    patch->match = (unsigned char*)malloc(rbPtr / 2);
                    hex_to_bin(readBuffer, patch->match, rbPtr);
                }
                patch->count = rbPtr / 2;

                state = parse_patches_state_gt;
                rbPtr = 0;
                if (!skip_whitespace(fp, &cChr)) goto patch_parser_eof;
                // Falls through

            case parse_patches_state_gt:
                if (cChr != '>') {
                    printf("Syntax Error: expected > saw %c\n", cChr);
                    return false;
                }
                if (!fread(&cChr, 1, 1, fp)) goto patch_parser_eof;
                state = parse_patches_state_replace;
                if (!skip_whitespace(fp, &cChr)) goto patch_parser_eof;
                // Falls through

            case parse_patches_state_replace:
                if (is_hex_char(cChr)) {
                    readBuffer[rbPtr++] = cChr;
                    break;
                }

                if (patch->count == 0) {
                    if (rbPtr == 0) {
                        printf("Replace length must be non-zero\n");
                        return false;
                    }
                    if (rbPtr % 2 != 0) {
                        printf("Replace length must be even: saw %d\n", rbPtr);
                        return false;
                    }
                    patch->count = rbPtr / 2;

                    patch->replace = (unsigned char*)malloc(rbPtr / 2);
                    hex_to_bin(readBuffer, patch->replace, rbPtr);
                } else {
                    if (rbPtr != patch->count * 2) {
                        printf("Replace length must match: %d != %d\n", rbPtr, patch->count * 2);
                        return false;
                    }

                    patch->replace = (unsigned char*)malloc(patch->count);
                    hex_to_bin(readBuffer, patch->replace, rbPtr);
                }

                state = parse_patches_state_pound;
                rbPtr = 0;
                if (!skip_whitespace(fp, &cChr)) goto patch_parser_eof;
                // Falls through

            case parse_patches_state_pound:
                if (cChr == '\r' || cChr == '\n') {
                    commit_patch(patch);
                    state = parse_patches_state_start;
                    break;
                }
                if (cChr != '#') {
                    printf("Syntax Error: expected # saw %c\n", cChr);
                    return false;
                }

                if (!fread(&cChr, 1, 1, fp)) goto patch_parser_eof;
                state = parse_patches_state_name;
                rbPtr = 0;
                if (!skip_whitespace(fp, &cChr)) goto patch_parser_eof;
                // Falls through

            case parse_patches_state_name:
                if (cChr == '\r' || cChr == '\n') {
                    patch->name = (char*)malloc(rbPtr + 1);
                    memcpy(patch->name, readBuffer, rbPtr);
                    patch->name[rbPtr] = '\0';

                    commit_patch(patch);
                    state = parse_patches_state_start;
                    break;
                }
                readBuffer[rbPtr++] = cChr;
                break;
        }
    }

patch_parser_eof:
    if (patch) commit_patch(patch);
    fclose(fp);
    return true;
}

bool load_patches(char* indexPath, char* exeName) {
    FILE* fp;
    if (fopen_s(&fp, indexPath, "r")) {
        printf("Failed to open %s\n", indexPath);
        return false;
    }

    char cChr;
    char nameBuffer[256];
    char patchBuffer[256];
    size_t which = 0;
    size_t nPtr = 0;
    size_t pPtr = 0;
    while (fread(&cChr, 1, 1, fp)) {
        if (cChr == '\r' || cChr == '\n') {
            if (nPtr && pPtr) {
                nameBuffer[nPtr] = '\0';
                patchBuffer[pPtr] = '\0';
                if (strcmp(nameBuffer, exeName) == 0) {
                    printf("Loading: %s\n", patchBuffer);
                    if (!load_patches_file(patchBuffer)) return false;
                }
            }
            nPtr = pPtr = which = 0;
            continue;
        }

        if (cChr == ' ') continue;
        if (cChr == ':') {
            which++;
            continue;
        }
        if (which == 0)
            nameBuffer[nPtr++] = cChr;
        else if (which == 1)
            patchBuffer[pPtr++] = cChr;
    }

    fclose(fp);
    return true;
}
