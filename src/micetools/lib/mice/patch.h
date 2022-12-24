#pragma once
#include <stdbool.h>
#include <stdio.h>

#include "../json/json.h"
#include "../util/hex.h"

typedef struct {
    size_t offset;
    size_t count;
    char* name;
    unsigned char* from;
    unsigned char* to;
} patch_t;

typedef struct {
    char* name;
    char* description;
    char* binary_name;
    bool apply;
    size_t nopatches;
    patch_t patches[];
} patchset_t;

typedef struct {
    size_t nopatchsets;
    patchset_t** patchsets;
} patches_t;

// Internal JSON helpers
bool fetch(json_value* object, char* name, json_value** value);
bool fetch_string(json_value* object, char* name, char** value);
bool fetch_int(json_value* object, char* name, size_t* value);
bool fetch_bool(json_value* object, char* name, bool* value);
bool parse_patches(patches_t* patches, json_value** set_json, int set_count, char* error,
                   const char* test_binary_name);

// Main two exports
void free_patches(patches_t* patches);
bool load_patches(patches_t* patches, char* path, char* error, const char* test_binary_name);
