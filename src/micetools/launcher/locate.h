#include <Windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <tchar.h>

bool locate_exe(char* path, size_t len, const char* exe);
bool locate_game(char* path, size_t len);
bool locate_library(char* path, size_t len);
