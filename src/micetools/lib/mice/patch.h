#pragma once
#include <stdbool.h>
#include <stdio.h>

#include "../util/hex.h"

bool load_patches_file(const char* filename);
bool load_patches(char* indexPath, char* exeName);

typedef struct _patch {
    void* address;
    size_t count;
    unsigned char* match;
    unsigned char* replace;
    char* name;

    struct _patch* next;
} patch_t;

extern patch_t* patch_list;

typedef enum {
    parse_patches_state_start = 0,
    parse_patches_state_addr,
    parse_patches_state_colon,
    parse_patches_state_match,
    parse_patches_state_gt,
    parse_patches_state_replace,
    parse_patches_state_pound,
    parse_patches_state_name,
} parse_patches_state;
