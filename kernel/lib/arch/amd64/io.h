#include <types.h>

#ifndef AMD64_IO_H
#define ADM64_IO_H

static inline void outb(uint16_t port, uint8_t val){
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port){
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}


uint64_t readCR3(void);

void sendEOI(unsigned char irq);
void remapPIC(int offset1, int offset2);
void fillIDT(void);


#endif // !AMD64_IO_H
