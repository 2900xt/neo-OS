#ifndef TYPES_H
#define TYPES_H

typedef unsigned long           uint64_t;
typedef unsigned int            uint32_t;
typedef unsigned short          uint16_t;
typedef unsigned char           uint8_t;
typedef long long               int64_t;
typedef int                     int32_t;
typedef short                   int16_t;
typedef char                    int8_t;
typedef uint64_t                size_t;

typedef uint64_t*               uintptr_t;
typedef uint8_t                 byte_t;
typedef uint16_t                word_t;
typedef uint32_t                dword_t;
typedef uint64_t                qword_t;

#ifndef NULL
#ifndef __cplusplus
    #define NULL (void*)0
#else 
    #define NULL 0
#endif
#endif

extern "C" void __cxa_pure_virtual();


#endif // !TYPES_H