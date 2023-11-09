#ifndef DEBUG_H
#define DEBUG_H

#include <string.h>
#include <time.h>
#include <sys/types.h>

#define D_LEVEL_SILENT  0
#define D_LEVEL_L1      1
#define D_LEVEL_L2      2
#define D_LEVEL_L3      3
#define D_LEVEL_L4      4
#define D_LEVEL_L5      5



#define D_LEVEL 5
#ifdef DEBUG
    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #define debugprintf(level, message...)\
        do {\
        if (level <= D_LEVEL) {\
            printf(message);\
        }\
        } while(0)
    #define debugprintf_ff(level, message...)\
        do {\
        struct timeval tv;\
        if (level <= D_LEVEL) {\
            printf("%ld:%s:%s():", time(NULL), __FILENAME__, __FUNCTION__);\
            printf(message);\
        }\
        } while(0)
    #define debugprintf_ff_null(level, name)\
        do { \
            if (level <= D_LEVEL) {\
                debugprintf_ff(level, "Pointer to (%s) is NULL", name);\
            }\
        } while(0)
#else
#define debugprintf(level, message...) do {;} while(0)
#define debugprintf_ff(level, message...) do {;} while(0)
#define debugprintf_ff_null(level, message...) do {;} while(0)

#endif /* DEBUG */

#endif /* DEBUG_H */