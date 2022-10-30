#include <Windows.h>
#include <stdio.h>

#define P2A_PATH "C:\\ProgramData\\boost_interprocess\\ALLNetComP2A"
#define A2P_PATH "C:\\ProgramData\\boost_interprocess\\ALLNetComA2P"

int main(int argc, char* argv) {
    FILE* f;
    fopen_s(&f, "./test.txt", "w");
    fprintf(f, "count: %d", argc);
    fclose(f);

    return 0;
}
