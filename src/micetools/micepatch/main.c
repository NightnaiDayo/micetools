#include "../lib/json/json.h"
#include "../lib/util/hex.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"

typedef struct {
    size_t offset;
    size_t count;
    unsigned char* from;
    unsigned char* to;
} patch_t;

typedef struct {
    char* name;
    char* description;
    bool apply;
    size_t nopatches;
    patch_t patches[];
} patchset_t;

typedef struct {
    size_t nopatchsets;
    patchset_t** patchsets;
} patches_t;

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
    *value = fetched->u.integer;
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
            for (int j = 0; j < patches->patchsets[i]->nopatches; j++) {
                if (patches->patchsets[i]->patches[j].from != NULL) free(patches->patchsets[i]->patches[j].from);
                if (patches->patchsets[i]->patches[j].to != NULL) free(patches->patchsets[i]->patches[j].to);
            }
            free(patches->patchsets[i]);
        }
    }
    patches->nopatchsets = 0;
}

bool parse_patches(patches_t* patches, json_value** set_json, int set_count, char* error) {
    patches->nopatchsets = set_count;
    patches->patchsets = (patchset_t**)malloc(set_count * sizeof(patchset_t*));
    for (int i = 0; i < set_count; i++) patches->patchsets[i] = NULL;

    error[0] = '\0';
    for (int i = 0; i < set_count; i++) {
        char *name, *description;
        if (!fetch_string(set_json[i], "name", &name)) {
            snprintf(error, json_error_max, "'name' missing for patch %d", i);
            goto failed;
        }
        if (!fetch_string(set_json[i], "description", &description)) {
            snprintf(error, json_error_max, "'description' missing for patch %d (%s)", i, name);
            goto failed;
        }
        bool apply;
        if (!fetch_bool(set_json[i], "apply", &apply)) {
            snprintf(error, json_error_max, "'apply' missing for patch %d (%s)", i, name);
            goto failed;
        }
        json_value* set_patches;
        if (!fetch(set_json[i], "patches", &set_patches) || set_patches->type != json_array) {
            snprintf(error, json_error_max, "'patches' missing for patch %d (%s)", i, name);
            goto failed;
        }

        int count = set_patches->u.array.length;
        patchset_t* patchset = (patchset_t*)malloc(sizeof(patchset_t) + (sizeof(patch_t) * count));
        patches->patchsets[i] = patchset;
        patchset->name = name;
        patchset->apply = apply;
        patchset->description = description;
        patchset->nopatches = count;

        for (int j = 0; j < count; j++) {
            json_value* this_patch = set_patches->u.array.values[j];
            size_t at;
            char *from, *to;
            if (!fetch_int(this_patch, "at", &at)) {
                snprintf(error, json_error_max, "'at' missing for patch %s[%d]", name, j);
                goto failed;
            }
            if (!fetch_string(this_patch, "from", &from)) {
                snprintf(error, json_error_max, "'from' missing for patch %s[%d]", name, j);
                goto failed;
            }
            if (!fetch_string(this_patch, "to", &to)) {
                snprintf(error, json_error_max, "'to' missing for patch %s[%d]", name, j);
                goto failed;
            }

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
            char* bin_from = patchset->patches[j].from = (char*)malloc(size / 2);
            char* bin_to = patchset->patches[j].to = (char*)malloc(size / 2);
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

void print_patches(patches_t* patches) {
    for (int i = 0; i < patches->nopatchsets; i++) {
        patchset_t* patchset = patches->patchsets[i];

        printf("Patch: %s (%s)\n", patchset->name, patchset->apply ? "applied" : "unapplied");
        printf("- %s\n", patchset->description);
        for (int i = 0; i < patchset->nopatches; i++) {
            printf(":: %d bytes at %08x\n", patchset->patches[i].count, patchset->patches[i].offset);
        }
        puts("======================");
    }
}

void apply_patches(patches_t* patches, char* filename) {
    FILE* fp = fopen(filename, "r+b");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s for modification\n", filename);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);

    for (int i = 0; i < patches->nopatchsets; i++) {
        patchset_t* patchset = patches->patchsets[i];
        for (int j = 0; j < patchset->nopatches; j++) {
            patch_t patch = patchset->patches[j];
            if (patch.offset + patch.count > sz) {
                fprintf(stderr, "E: Patch %s[%d] outside file bounds\n", patchset->name, j);
                goto done;
            }
            fseek(fp, patch.offset, SEEK_SET);
            bool matches_from = true;
            bool matches_to = true;
            for (int i = 0; i < patch.count; i++) {
                unsigned char seen;
                if (!fread(&seen, 1, 1, fp)) {
                    matches_from = false;
                    matches_to = false;
                    break;
                }
                if (seen != patch.from[i]) matches_from = false;
                if (seen != patch.to[i]) matches_to = false;
                if (!(matches_from || matches_to)) break;
            }

            printf("%s[%d]: ", patchset->name, j);
            if (patchset->apply) {
                if (matches_to) {
                    puts("Already applied");
                    continue;
                }
                if (!matches_from) {
                    puts("From value missmatch! Ignoring");
                    continue;
                }
                fseek(fp, patch.offset, SEEK_SET);
                fwrite(patch.to, 1, patch.count, fp);
                puts("Patch applied");
            } else {
                if (matches_from) {
                    puts("Not applied");
                    continue;
                }
                if (!matches_to) {
                    puts("We didn't perform this patch. Leaving patched");
                    continue;
                }
                fseek(fp, patch.offset, SEEK_SET);
                fwrite(patch.from, 1, patch.count, fp);
                puts("Patch removed");
            }
        }
    }

done:
    fclose(fp);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <patch file> <exe>\n", argc > 0 ? argv[0] : "micepatch.exe");
        return -1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open '%s' (%d)\n", argv[1], errno);
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);
    rewind(fp);

    char* json_buf = (char*)malloc(sz);
    if (json_buf == NULL) {
        fprintf(stderr, "Failed to allocate %d bytes\n", sz, errno);
        fclose(fp);
        return -1;
    }
    if (!(sz = fread(json_buf, 1, sz, fp))) {
        fprintf(stderr, "Failed to read from '%s' (%d)\n", argv[1], errno);
        fclose(fp);
        return -1;
    };
    fclose(fp);

    json_settings settings = {0};
    char error[json_error_max];
    json_value* parsed = json_parse_ex(&settings, json_buf, sz, error);

    if (parsed == NULL) {
        fprintf(stderr, "Parse error: %s\n", error);
        return -1;
    }

    int loaded_patches = 0;
    json_value** patches_json;
    if (parsed->type == json_array) {
        loaded_patches = parsed->u.array.length;
        patches_json = parsed->u.array.values;
    } else if (parsed->type == json_object) {
        loaded_patches = 1;
        patches_json = &parsed;
    } else {
        fprintf(stderr, "Patch file format error\n");
        json_value_free(parsed);
        return;
    }

    patches_t all_patches;
    if (!parse_patches(&all_patches, patches_json, loaded_patches, error)) {
        fprintf(stderr, "Patch file format error: %s\n", error);
        return;
    }

    puts("");
    puts("Loaded patches:");
    puts("---------------");
    print_patches(&all_patches);

    apply_patches(&all_patches, argv[2]);

    free_patches(&all_patches);
    return 0;
}
