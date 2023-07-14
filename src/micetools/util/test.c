#include <stdio.h>

#include "../lib/mice/mice.h"

MICE_DA_NEW(testDa, int)

int main(int argc, char** argv) {
    printf("length = %d\n", MiceDALength(testDa));
    for (int i = 0; i < 10; i++) MiceDAPush(testDa, &i);
    printf("length = %d\n", MiceDALength(testDa));
    for (int i = 0; i < 10; i++) printf("[%d]: %d\n", i, *(int*)MiceDAGet(testDa, i));

    MICE_DA_ITER(testDa, int, test) {
        printf("iter: %d\n", *test);
    } MICE_DA_ITER_END

    puts("Pop");
    MiceDAPop(testDa, NULL);
    MICE_DA_ITER(testDa, int, test) {
        printf("iter: %d\n", *test);
    } MICE_DA_ITER_END

    puts("Unshift");
    MiceDAUnshift(testDa, NULL);
    MICE_DA_ITER(testDa, int, test) {
        printf("iter: %d\n", *test);
    } MICE_DA_ITER_END

    puts("Remove 1");
    MiceDARemove(testDa, 1);
    MICE_DA_ITER(testDa, int, test) {
        printf("iter: %d\n", *test);
    } MICE_DA_ITER_END

    return 0;
}