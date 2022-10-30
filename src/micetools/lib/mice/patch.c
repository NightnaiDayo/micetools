#include "patch.h"

#include <stdlib.h>
#include <string.h>

bool fetch(json_value* object, char* name, json_value** value) {
    if (object->type != json_object) return false;
    for (size_t j = 0; j < object->u.object.length; j++) {
        json_object_entry* entry = &(object->u.object.values[j]);
        if (strncmp(name, entry->name, entry->name_length) == 0) {
            *value = entry->value;
            return true;
        }
    }
    return false;
}
bool fetch_string(json_value* object, char* name, char** value) {
    json_value* fetched;
    if (!fetch(object, name, &fetched)) return false;
    if (fetched->type != json_string) return false;
    *value = fetched->u.string.ptr;
    return true;
}
bool fetch_int(json_value* object, char* name, size_t* value) {
    json_value* fetched;
    if (!fetch(object, name, &fetched)) return false;
    if (fetched->type != json_integer) return false;
    *value = (size_t)fetched->u.integer;
    return true;
}
bool fetch_bool(json_value* object, char* name, bool* value) {
    json_value* fetched;
    if (!fetch(object, name, &fetched)) return false;
    if (fetched->type != json_boolean) return false;
    *value = fetched->u.boolean;
    return true;
}
void free_patches(patches_t* patches) {
    if (patches->nopatchsets == 0) return;
    for (int i = patches->nopatchsets - 1; i >= 0; i--) {
        if (patches->patchsets[i] != NULL) {
            for (size_t j = 0; j < patches->patchsets[i]->nopatches; j++) {
                if (patches->patchsets[i]->patches[j].from != NULL) free(patches->patchsets[i]->patches[j].from);
                if (patches->patchsets[i]->patches[j].to != NULL) free(patches->patchsets[i]->patches[j].to);
            }
            free(patches->patchsets[i]);
        }
    }
    patches->nopatchsets = 0;
}

json_value* load_json_from_file(char* path, char* error) {
    FILE* fp;
    fopen_s(&fp, path, "r");
    if (fp == NULL) {
        if (error != NULL) snprintf(error, json_error_max, "Failed to open patch file");
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);
    rewind(fp);

    char* json_buf = (char*)malloc(sz);
    if (json_buf == NULL) {
        if (error != NULL) snprintf(error, json_error_max, "Failed to allocate file buffer");
        fclose(fp);
        return NULL;
    }
    if (!(sz = fread(json_buf, 1, sz, fp))) {
        if (error != NULL) snprintf(error, json_error_max, "Failed to read file");
        fclose(fp);
        return NULL;
    };
    fclose(fp);

    json_settings settings = { 0 };
    return json_parse_ex(&settings, json_buf, sz, error);
}

bool parse_patches(patches_t* patches, json_value** set_json, int set_count, char* error) {
    patches->nopatchsets = set_count;
    patches->patchsets = (patchset_t**)malloc(set_count * sizeof(patchset_t*));
    for (int i = 0; i < set_count; i++) patches->patchsets[i] = NULL;

    error[0] = '\0';
    for (int i = 0; i < set_count; i++) {
        patchset_t* patchset = (patchset_t*)malloc(sizeof(patchset_t));
        patches->patchsets[i] = patchset;
        patchset->name = NULL;
        patchset->description = NULL;
        patchset->binary_name = NULL;
        patchset->apply = false;
        patchset->nopatches = 0;

        char *name, *description, *binary_name;
        if (!fetch_string(set_json[i], "name", &name)) {
            snprintf(error, json_error_max, "'name' missing for patch %d", i);
            goto failed;
        }
        if (!fetch_string(set_json[i], "description", &description)) {
            snprintf(error, json_error_max, "'description' missing for patch %d (%s)", i, name);
            goto failed;
        }
        if (!fetch_string(set_json[i], "binary_name", &binary_name)) binary_name = NULL;

        bool apply;
        if (!fetch_bool(set_json[i], "apply", &apply)) {
            snprintf(error, json_error_max, "'apply' missing for patch %d (%s)", i, name);
            goto failed;
        }
        json_value* set_patches;
        if (!fetch(set_json[i], "patches", &set_patches) || set_patches->type != json_array) {
            char* patches_file_path;
            if (fetch_string(set_json[i], "patches_file", &patches_file_path)) {
                char load_error[json_error_max];
                set_patches = load_json_from_file(patches_file_path, load_error);
                if (set_patches == NULL) {
                    fprintf(stderr, "W: patcher: Failed to load '%s': %s\n", patches_file_path, load_error);
                    continue;
                }
                if (set_patches->type != json_array) {
                    fprintf(stderr, "W: patcher: Failed to load '%s': not an array\n", patches_file_path);
                    continue;
                }
            } else {
                snprintf(error, json_error_max, "Neither 'patches' nor 'patchs_file' in patch %d (%s)", i, name);
                goto failed;
            }
        }

        int count = set_patches->u.array.length;
        patchset = (patchset_t*)realloc(patchset, sizeof(patchset_t) + (sizeof(patch_t) * count));
        patches->patchsets[i] = patchset;

        patchset->name = name;
        patchset->description = description;
        patchset->binary_name = binary_name;
        patchset->apply = apply;
        patchset->nopatches = count;
        memset(patchset->patches, 0, count * (sizeof(patch_t)));

        for (int j = 0; j < count; j++) {
            json_value* this_patch = set_patches->u.array.values[j];
            size_t at;
            char *from, *to;
            if (!fetch_int(this_patch, "at", &at)) {
                char* at_str;
                if (!fetch_string(this_patch, "at", &at_str)) {
                    snprintf(error, json_error_max, "'at' missing for patch %s[%d]", name, j);
                    goto failed;
                }

                if (strlen(at_str) > 8) {
                    snprintf(error, json_error_max, "invalid 'at' value for patch %s[%d]", name, j);
                    goto failed;
                }
                hex_to_bin(at_str, (void*)&at, 8);
                at = _byteswap_ulong(at);  // Endianess!
            }
            if (!fetch_string(this_patch, "from", &from)) {
                snprintf(error, json_error_max, "'from' missing for patch %s[%d]", name, j);
                goto failed;
            }
            if (!fetch_string(this_patch, "to", &to)) {
                snprintf(error, json_error_max, "'to' missing for patch %s[%d]", name, j);
                goto failed;
            }

            char* patch_name;
            if (!fetch_string(this_patch, "name", &patch_name)) patch_name = NULL;

            patchset->patches[j].name = patch_name;
            patchset->patches[j].offset = at;
            size_t size = strlen(from);
            if (size != strlen(to)) {
                snprintf(error, json_error_max, "'from' and 'to' lengths differ in patch %s[%d]", name, j);
                goto failed;
            }
            if (size % 2 != 0) {
                snprintf(error, json_error_max, "invalid hex string in patch %s[%d]", name, j);
                goto failed;
            }
            unsigned char* bin_from = patchset->patches[j].from = (unsigned char*)malloc(size / 2);
            unsigned char* bin_to = patchset->patches[j].to = (unsigned char*)malloc(size / 2);
            if (!hex_to_bin(from, bin_from, size)) {
                snprintf(error, json_error_max, "invalid hex string in patch %s[%d].from", name, j);
                goto failed;
            };
            if (!hex_to_bin(to, bin_to, size)) {
                snprintf(error, json_error_max, "invalid hex string in patch %s[%d].to", name, j);
                goto failed;
            };

            patchset->patches[j].count = size / 2;
        }
    }

    return true;

failed:
    free_patches(patches);
    return false;
}

bool load_patches(patches_t* patches, char* path, char* error) {
    patches->nopatchsets = 0;

    json_value* parsed = load_json_from_file(path, error);
    if (parsed == NULL) return false;

    int loaded_patches = 0;
    json_value** patches_json;
    if (parsed->type == json_array) {
        loaded_patches = parsed->u.array.length;
        patches_json = parsed->u.array.values;
    } else if (parsed->type == json_object) {
        loaded_patches = 1;
        patches_json = &parsed;
    } else {
        snprintf(error, json_error_max, "Patch file format error");
        json_value_free(parsed);
        return false;
    }

    if (!parse_patches(patches, patches_json, loaded_patches, error)) return false;
    return true;
}
