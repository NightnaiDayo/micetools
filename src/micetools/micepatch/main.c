#include "../lib/mice/mice.h"
#include "string.h"

void print_patches(patches_t* patches, char* filename) {
    for (int i = 0; i < patches->nopatchsets; i++) {
        patchset_t* patchset = patches->patchsets[i];

        bool skip = patchset->name != NULL && strcmp(patchset->name, filename) != 0;

        printf("Patch: %s (%s)\n", patchset->name, skip ? "skipped" : patchset->apply ? "applied" : "unapplied");
        printf("- %s\n", patchset->description);
        if (!skip) {
            for (int i = 0; i < patchset->nopatches; i++) {
                printf(":: %d bytes at %08x\n", patchset->patches[i].count, patchset->patches[i].offset);
            }
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

        if (patchset->name != NULL && strcmp(patchset->name, filename) != 0) {
            continue;
        }

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

    char error[json_error_max];
    patches_t all_patches;
    if (!load_patches(&all_patches, argv[1], error)) {
        fprintf(stderr, "%s", error);
        return -1;
    }

    puts("");
    puts("Loaded patches:");
    puts("---------------");
    print_patches(&all_patches, argv[2]);
    apply_patches(&all_patches, argv[2]);
    free_patches(&all_patches);
    return 0;
}
