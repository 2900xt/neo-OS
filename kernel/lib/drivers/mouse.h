#include <types.h>
#ifndef DRIVERS_MOUSE_H
#define DRIVERS_MOUSE_H

void mouseINIT(void);
uint8_t mouseReadByte(void);
void processMousePacket(void);
#endif