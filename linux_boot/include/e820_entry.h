#ifndef E820ENTRY
#define E820ENTRY

#include <Base.h>

typedef struct {
    UINT64 addr; /* start of memory segment */
    UINT64 size; /* size of memory segment */
    UINT32 type; /* type of memory segment */
} e820entry;

#endif