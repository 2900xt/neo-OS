#ifndef TYPES_H
#define TYPES_H

typedef unsigned long long      uint64_t;
typedef unsigned int            uint32_t;
typedef unsigned short          uint16_t;
typedef unsigned char           uint8_t;
typedef long long               int64_t;
typedef int                     int32_t;
typedef short                   int16_t;
typedef char                    int8_t;
typedef uint64_t                size_t;

#ifndef NULL
#ifndef __cplusplus
    #define NULL (void*)0
#else 
    #define NULL 0
#endif
#endif


#endif // !TYPES_H