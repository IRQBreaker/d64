#ifndef DISASM_H_
#define DISASM_H_

#include <stdint.h>

void disasm(uint8_t *buffer, int size, int illegal);

#endif // DISASM_H_
