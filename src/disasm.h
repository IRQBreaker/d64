#ifndef DISASM_H_
#define DISASM_H_

#include <stdint.h>

void disasm(const uint8_t *buffer, const int size, const int address, const int illegal);

#endif // DISASM_H_
