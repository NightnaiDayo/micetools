#include "../mice/version_fallback.h"

#define AM_LIB_C_HEADER(name, storage_type) \
    int name##DebugLevel = 0;               \
    struct _##storage_type name;            \
    char* name##Version = #name " Ver.mice" MICE_VERSION " Build:" __DATE__ " " __TIME__;
#define AM_LIB_H_HEADER(name, storage_type) \
    extern int name##DebugLevel;            \
    extern struct _##storage_type name;     \
    extern char* name##Version;
