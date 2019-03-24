#include "disasm.h"

#include <stdio.h>
#include <stdint.h>

/* Addressing modes */
enum { IMPLIED, IMMEDIATE, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, ZEROPAGE,
    INDIRECT_X, INDIRECT_Y, ZEROPAGE_X, INDIRECT, ZEROPAGE_Y, RELATIVE };

typedef struct
{
    char *mnemonic;
    int size;
    int type;
} mnemonics;

/* name, size in bytes, type */
mnemonics mne_legal[] = {
    {"BRK", 1, IMPLIED},    {"ORA", 2, INDIRECT_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"ORA", 2, ZEROPAGE},   {"ASL", 2, ZEROPAGE},   {"???", 1, IMPLIED},
    {"PHP", 1, IMPLIED},    {"ORA", 2, IMMEDIATE},  {"ASL", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"ORA", 3, ABSOLUTE},   {"ASL", 3, ABSOLUTE},   {"???", 1, IMPLIED},
    {"BPL", 2, RELATIVE},   {"ORA", 2, INDIRECT_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"ORA", 2, ZEROPAGE_X}, {"ASL", 2, ZEROPAGE_X}, {"???", 1, IMPLIED},
    {"CLC", 1, IMPLIED},    {"ORA", 3, ABSOLUTE_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"ORA", 3, ABSOLUTE_X}, {"ASL", 3, ABSOLUTE_X}, {"???", 1, IMPLIED},
    {"JSR", 3, ABSOLUTE},   {"AND", 2, INDIRECT_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"BIT", 2, ZEROPAGE},   {"AND", 2, ZEROPAGE},   {"ROL", 2, ZEROPAGE},   {"???", 1, IMPLIED},
    {"PLP", 1, IMPLIED},    {"AND", 2, IMMEDIATE},  {"ROL", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"BIT", 3, ABSOLUTE},   {"AND", 3, ABSOLUTE},   {"ROL", 3, ABSOLUTE},   {"???", 1, IMPLIED},
    {"BMI", 2, RELATIVE},   {"AND", 2, INDIRECT_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"AND", 2, ZEROPAGE_X}, {"ROL", 2, ZEROPAGE_X}, {"???", 1, IMPLIED},
    {"SEC", 1, IMPLIED},    {"AND", 3, ABSOLUTE_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"AND", 3, ABSOLUTE_X}, {"ROL", 3, ABSOLUTE_X}, {"???", 1, IMPLIED},
    {"RTI", 1, IMPLIED},    {"EOR", 2, INDIRECT_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"EOR", 2, ZEROPAGE},   {"LSR", 2, ZEROPAGE},   {"???", 1, IMPLIED},
    {"PHA", 1, IMPLIED},    {"EOR", 2, IMMEDIATE},  {"LSR", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"JMP", 3, ABSOLUTE},   {"EOR", 3, ABSOLUTE},   {"LSR", 3, ABSOLUTE},   {"???", 1, IMPLIED},
    {"BVC", 2, RELATIVE},   {"EOR", 2, INDIRECT_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"EOR", 2, ZEROPAGE_X}, {"LSR", 2, ZEROPAGE_X}, {"???", 1, IMPLIED},
    {"CLI", 1, IMPLIED},    {"EOR", 3, ABSOLUTE_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"EOR", 3, ABSOLUTE_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"RTS", 1, IMPLIED},    {"ADC", 2, INDIRECT_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"ADC", 2, ZEROPAGE},   {"ROR", 2, ZEROPAGE},   {"???", 1, IMPLIED},
    {"PLA", 1, IMPLIED},    {"ADC", 2, IMMEDIATE},  {"ROR", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"JMP", 3, INDIRECT},   {"ADC", 3, ABSOLUTE},   {"ROR", 3, ABSOLUTE},   {"???", 1, IMPLIED},
    {"BVS", 2, RELATIVE},   {"ADC", 2, INDIRECT_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"ADC", 2, ZEROPAGE_X}, {"ROR", 2, ZEROPAGE_X}, {"???", 1, IMPLIED},
    {"SEI", 1, IMPLIED},    {"ADC", 3, ABSOLUTE_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"ADC", 3, ABSOLUTE_X}, {"ROR", 3, ABSOLUTE_X}, {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"STA", 2, INDIRECT_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"STY", 2, ZEROPAGE},   {"STA", 2, ZEROPAGE},   {"STX", 2, ZEROPAGE},   {"???", 1, IMPLIED},
    {"DEY", 1, IMPLIED},    {"???", 1, IMPLIED},    {"TXA", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"STY", 3, ABSOLUTE},   {"STA", 3, ABSOLUTE},   {"STX", 3, ABSOLUTE},   {"???", 1, IMPLIED},
    {"BCC", 2, RELATIVE},   {"STA", 2, INDIRECT_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"STY", 2, ZEROPAGE_X}, {"STA", 2, ZEROPAGE_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"TYA", 1, IMPLIED},    {"STA", 3, ABSOLUTE_Y}, {"TXS", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"STA", 3, ABSOLUTE_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"LDY", 2, IMMEDIATE},  {"LDA", 2, INDIRECT_X}, {"LDX", 2, IMMEDIATE},  {"???", 1, IMPLIED},
    {"LDY", 2, ZEROPAGE},   {"LDA", 2, ZEROPAGE},   {"LDX", 2, ZEROPAGE},   {"???", 1, IMPLIED},
    {"TAY", 1, IMPLIED},    {"LDA", 2, IMMEDIATE},  {"TAX", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"LDY", 3, ABSOLUTE},   {"LDA", 3, ABSOLUTE},   {"LDX", 3, ABSOLUTE},   {"???", 1, IMPLIED},
    {"BCS", 2, RELATIVE},   {"LDA", 2, INDIRECT_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"LDY", 2, ZEROPAGE_X}, {"LDA", 2, ZEROPAGE_X}, {"LDX", 2, ZEROPAGE_Y}, {"???", 1, IMPLIED},
    {"CLV", 1, IMPLIED},    {"LDA", 3, ABSOLUTE_Y}, {"TSX", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"LDY", 3, ABSOLUTE_X}, {"LDA", 3, ABSOLUTE_X}, {"LDX", 3, ABSOLUTE_Y}, {"???", 1, IMPLIED},
    {"CPY", 2, IMMEDIATE},  {"CMP", 2, INDIRECT_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"CPY", 2, ZEROPAGE},   {"CMP", 2, ZEROPAGE},   {"DEC", 2, ZEROPAGE},   {"???", 1, IMPLIED},
    {"INY", 1, IMPLIED},    {"CMP", 2, IMMEDIATE},  {"DEX", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"CPY", 3, ABSOLUTE},   {"CMP", 3, ABSOLUTE},   {"DEC", 3, ABSOLUTE},   {"???", 1, IMPLIED},
    {"BNE", 2, RELATIVE},   {"CMP", 2, INDIRECT_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"CMP", 2, ZEROPAGE_X}, {"DEC", 2, ZEROPAGE_X}, {"???", 1, IMPLIED},
    {"CLD", 1, IMPLIED},    {"CMP", 3, ABSOLUTE_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"CMP", 3, ABSOLUTE_X}, {"DEC", 3, ABSOLUTE_X}, {"???", 1, IMPLIED},
    {"CPX", 2, IMMEDIATE},  {"SBC", 2, INDIRECT_X}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"CPX", 2, ZEROPAGE},   {"SBC", 2, ZEROPAGE},   {"INC", 2, ZEROPAGE},   {"???", 1, IMPLIED},
    {"INX", 1, IMPLIED},    {"SBC", 2, IMMEDIATE},  {"NOP", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"CPX", 3, ABSOLUTE},   {"SBC", 3, ABSOLUTE},   {"INC", 3, ABSOLUTE},   {"???", 1, IMPLIED},
    {"BEQ", 2, RELATIVE},   {"SBC", 2, INDIRECT_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"SBC", 2, ZEROPAGE_X}, {"INC", 2, ZEROPAGE_X}, {"???", 1, IMPLIED},
    {"SED", 1, IMPLIED},    {"SBC", 3, ABSOLUTE_Y}, {"???", 1, IMPLIED},    {"???", 1, IMPLIED},
    {"???", 1, IMPLIED},    {"SBC", 3, ABSOLUTE_X}, {"INC", 3, ABSOLUTE_X}, {"???", 1, IMPLIED}
};

/* name, size in bytes, type */
mnemonics mne_illegal[] = {
    {"BRK", 1, IMPLIED},    {"ORA", 2, INDIRECT_X}, {"KIL", 1, IMPLIED},    {"SLO", 2, INDIRECT_X},
    {"NOP", 2, ZEROPAGE},   {"ORA", 2, ZEROPAGE},   {"ASL", 2, ZEROPAGE},   {"SLO", 2, ZEROPAGE},
    {"PHP", 1, IMPLIED},    {"ORA", 2, IMMEDIATE},  {"ASL", 2, IMPLIED},    {"ANC", 2, IMMEDIATE},
    {"NOP", 3, ABSOLUTE},   {"ORA", 3, ABSOLUTE},   {"ASL", 3, ABSOLUTE},   {"SLO", 3, ABSOLUTE},
    {"BPL", 2, RELATIVE},   {"ORA", 2, INDIRECT_Y}, {"KIL", 1, IMPLIED},    {"SLO", 2, INDIRECT_Y},
    {"NOP", 2, ZEROPAGE_X}, {"ORA", 2, ZEROPAGE_X}, {"ASL", 2, ZEROPAGE_X}, {"SLO", 2, ZEROPAGE_X},
    {"CLC", 1, IMPLIED},    {"ORA", 3, ABSOLUTE_Y}, {"NOP", 1, IMPLIED},    {"SLO", 3, ABSOLUTE_Y},
    {"NOP", 3, ABSOLUTE_X}, {"ORA", 3, ABSOLUTE_X}, {"ASL", 3, ABSOLUTE_X}, {"SLO", 3, ABSOLUTE_X},
    {"JSR", 3, ABSOLUTE},   {"AND", 2, INDIRECT_X}, {"KIL", 1, IMPLIED},    {"RLA", 2, INDIRECT_X},
    {"BIT", 2, ZEROPAGE},   {"AND", 2, ZEROPAGE},   {"ROL", 2, ZEROPAGE},   {"RLA", 2, ZEROPAGE},
    {"PLP", 1, IMPLIED},    {"AND", 2, IMMEDIATE},  {"ROL", 1, IMPLIED},    {"ANC", 2, IMMEDIATE},
    {"BIT", 3, ABSOLUTE},   {"AND", 3, ABSOLUTE},   {"ROL", 3, ABSOLUTE},   {"RLA", 3, ABSOLUTE},
    {"BMI", 2, RELATIVE},   {"AND", 2, INDIRECT_Y}, {"KIL", 1, IMPLIED},    {"RLA", 2, INDIRECT_Y},
    {"NOP", 2, ZEROPAGE_X}, {"AND", 2, ZEROPAGE_X}, {"ROL", 2, ZEROPAGE_X}, {"RLA", 2, ZEROPAGE_X},
    {"SEC", 1, IMPLIED},    {"AND", 3, ABSOLUTE_Y}, {"NOP", 1, IMPLIED},    {"RLA", 3, ABSOLUTE_Y},
    {"NOP", 3, ABSOLUTE_X}, {"AND", 3, ABSOLUTE_X}, {"ROL", 3, ABSOLUTE_X}, {"RLA", 3, ABSOLUTE_X},
    {"RTI", 1, IMPLIED},    {"EOR", 2, INDIRECT_X}, {"KIL", 1, IMPLIED},    {"SRE", 2, INDIRECT_X},
    {"NOP", 2, ZEROPAGE},   {"EOR", 2, ZEROPAGE},   {"LSR", 2, ZEROPAGE},   {"SRE", 2, ZEROPAGE},
    {"PHA", 1, IMPLIED},    {"EOR", 2, IMMEDIATE},  {"LSR", 1, IMPLIED},    {"ALR", 2, IMMEDIATE},
    {"JMP", 3, ABSOLUTE},   {"EOR", 3, ABSOLUTE},   {"LSR", 3, ABSOLUTE},   {"SRE", 3, ABSOLUTE},
    {"BVC", 2, RELATIVE},   {"EOR", 2, INDIRECT_Y}, {"KIL", 1, IMPLIED},    {"SRE", 2, INDIRECT_Y},
    {"NOP", 2, ZEROPAGE_X}, {"EOR", 2, ZEROPAGE_X}, {"LSR", 2, ZEROPAGE_X}, {"SRE", 2, ZEROPAGE_X},
    {"CLI", 1, IMPLIED},    {"EOR", 3, ABSOLUTE_Y}, {"NOP", 1, IMPLIED},    {"SRE", 3, ABSOLUTE_Y},
    {"NOP", 3, ABSOLUTE_X}, {"EOR", 3, ABSOLUTE_X}, {"LSR", 3, ABSOLUTE_X}, {"SRE", 3, ABSOLUTE_X},
    {"RTS", 1, IMPLIED},    {"ADC", 2, INDIRECT_X}, {"KIL", 1, IMPLIED},    {"RRA", 2, INDIRECT_X},
    {"NOP", 2, ZEROPAGE},   {"ADC", 2, ZEROPAGE},   {"ROR", 2, ZEROPAGE},   {"RRA", 2, ZEROPAGE},
    {"PLA", 1, IMPLIED},    {"ADC", 2, IMMEDIATE},  {"ROR", 1, IMPLIED},    {"ARR", 2, IMMEDIATE},
    {"JMP", 3, INDIRECT},   {"ADC", 3, ABSOLUTE},   {"ROR", 3, ABSOLUTE},   {"RRA", 3, ABSOLUTE},
    {"BVS", 2, RELATIVE},   {"ADC", 2, INDIRECT_Y}, {"KIL", 1, IMPLIED},    {"RRA", 2, INDIRECT_Y},
    {"NOP", 2, ZEROPAGE_X}, {"ADC", 2, ZEROPAGE_X}, {"ROR", 2, ZEROPAGE_X}, {"RRA", 2, ZEROPAGE_X},
    {"SEI", 1, IMPLIED},    {"ADC", 3, ABSOLUTE_Y}, {"NOP", 1, IMPLIED},    {"RRA", 3, ABSOLUTE_Y},
    {"NOP", 3, ABSOLUTE_X}, {"ADC", 3, ABSOLUTE_X}, {"ROR", 3, ABSOLUTE_X}, {"RRA", 3, ABSOLUTE_X},
    {"NOP", 2, IMMEDIATE},  {"STA", 2, INDIRECT_X}, {"NOP", 2, IMMEDIATE},  {"SAX", 2, INDIRECT_X},
    {"STY", 2, ZEROPAGE},   {"STA", 2, ZEROPAGE},   {"STX", 2, ZEROPAGE},   {"SAX", 2, ZEROPAGE},
    {"DEY", 1, IMPLIED},    {"NOP", 2, IMMEDIATE},  {"TXA", 1, IMPLIED},    {"XAA", 2, IMMEDIATE},
    {"STY", 3, ABSOLUTE},   {"STA", 3, ABSOLUTE},   {"STX", 3, ABSOLUTE},   {"SAX", 3, ABSOLUTE},
    {"BCC", 2, RELATIVE},   {"STA", 2, INDIRECT_Y}, {"KIL", 1, IMPLIED},    {"AHX", 2, INDIRECT_Y},
    {"STY", 2, ZEROPAGE_X}, {"STA", 2, ZEROPAGE_X}, {"STX", 2, ZEROPAGE_Y}, {"SAX", 2, ZEROPAGE_Y},
    {"TYA", 1, IMPLIED},    {"STA", 3, ABSOLUTE_Y}, {"TXS", 1, IMPLIED},    {"TAS", 1, IMPLIED},
    {"SHY", 3, ABSOLUTE_X}, {"STA", 3, ABSOLUTE_X}, {"SHX", 3, ABSOLUTE_Y}, {"AHX", 3, ABSOLUTE_Y},
    {"LDY", 2, IMMEDIATE},  {"LDA", 2, INDIRECT_X}, {"LDX", 2, IMMEDIATE},  {"LAX", 2, INDIRECT_X},
    {"LDY", 2, ZEROPAGE},   {"LDA", 2, ZEROPAGE},   {"LDX", 2, ZEROPAGE},   {"LAX", 2, ZEROPAGE},
    {"TAY", 1, IMPLIED},    {"LDA", 2, IMMEDIATE},  {"TAX", 1, IMPLIED},    {"LAX", 2, IMMEDIATE},
    {"LDY", 3, ABSOLUTE},   {"LDA", 3, ABSOLUTE},   {"LDX", 3, ABSOLUTE},   {"LAX", 3, ABSOLUTE},
    {"BCS", 2, RELATIVE},   {"LDA", 2, INDIRECT_Y}, {"KIL", 1, IMPLIED},    {"LAX", 2, INDIRECT_Y},
    {"LDY", 2, ZEROPAGE_X}, {"LDA", 2, ZEROPAGE_X}, {"LDX", 2, ZEROPAGE_Y}, {"LAX", 2, ZEROPAGE_Y},
    {"CLV", 1, IMPLIED},    {"LDA", 3, ABSOLUTE_Y}, {"TSX", 1, IMPLIED},    {"LAS", 3, ABSOLUTE_Y},
    {"LDY", 3, ABSOLUTE_X}, {"LDA", 3, ABSOLUTE_X}, {"LDX", 3, ABSOLUTE_Y}, {"LAX", 3, ABSOLUTE_Y},
    {"CPY", 2, IMMEDIATE},  {"CMP", 2, INDIRECT_X}, {"NOP", 2, IMMEDIATE},  {"DCP", 2, INDIRECT_X},
    {"CPY", 2, ZEROPAGE},   {"CMP", 2, ZEROPAGE},   {"DEC", 2, ZEROPAGE},   {"DCP", 2, ZEROPAGE},
    {"INY", 1, IMPLIED},    {"CMP", 2, IMMEDIATE},  {"DEX", 1, IMPLIED},    {"AXS", 2, IMMEDIATE},
    {"CPY", 3, ABSOLUTE},   {"CMP", 3, ABSOLUTE},   {"DEC", 3, ABSOLUTE},   {"DCP", 3, ABSOLUTE},
    {"BNE", 2, RELATIVE},   {"CMP", 2, INDIRECT_Y}, {"KIL", 1, IMPLIED},    {"DCP", 2, INDIRECT_Y},
    {"NOP", 2, ZEROPAGE_X}, {"CMP", 2, ZEROPAGE_X}, {"DEC", 2, ZEROPAGE_X}, {"DCP", 2, ZEROPAGE_X},
    {"CLD", 1, IMPLIED},    {"CMP", 3, ABSOLUTE_Y}, {"NOP", 1, IMPLIED},    {"DCP", 3, ABSOLUTE_Y},
    {"NOP", 3, ABSOLUTE_X}, {"CMP", 3, ABSOLUTE_X}, {"DEC", 3, ABSOLUTE_X}, {"DCP", 3, ABSOLUTE_X},
    {"CPX", 2, IMMEDIATE},  {"SBC", 2, INDIRECT_X}, {"NOP", 2, IMMEDIATE},  {"ISC", 2, INDIRECT_X},
    {"CPX", 2, ZEROPAGE},   {"SBC", 2, ZEROPAGE},   {"INC", 2, ZEROPAGE},   {"ISC", 2, ZEROPAGE},
    {"INX", 1, IMPLIED},    {"SBC", 2, IMMEDIATE},  {"NOP", 1, IMPLIED},    {"SBC", 2, IMMEDIATE},
    {"CPX", 3, ABSOLUTE},   {"SBC", 3, ABSOLUTE},   {"INC", 3, ABSOLUTE},   {"ISC", 3, ABSOLUTE},
    {"BEQ", 2, RELATIVE},   {"SBC", 2, INDIRECT_Y}, {"KIL", 1, IMPLIED},    {"ISC", 2, INDIRECT_Y},
    {"NOP", 2, ZEROPAGE_X}, {"SBC", 2, ZEROPAGE_X}, {"INC", 2, ZEROPAGE_X}, {"ISC", 2, ZEROPAGE_X},
    {"SED", 1, IMPLIED},    {"SBC", 3, ABSOLUTE_Y}, {"NOP", 1, IMPLIED},    {"ISC", 3, ABSOLUTE_Y},
    {"NOP", 3, ABSOLUTE_X}, {"SBC", 3, ABSOLUTE_X}, {"INC", 3, ABSOLUTE_X}, {"ISC", 3, ABSOLUTE_X}
};

void disasm(char *buffer, int size, int illegal)
{
    mnemonics *mne = illegal ? (mnemonics*)&mne_illegal : (mnemonics*)&mne_legal;

    // Get loading address and advance index
    uint16_t address = buffer[0] + ((buffer[1] & 0xff) << 8);
    int index = 2;

    while (index < size) {
        uint8_t low = 0;
        uint8_t high = 0;
        uint8_t opcode = buffer[index++];

        // address
        printf("%04X ", address);

        // hexdump
        switch (mne[opcode].size) {
            case 2:
                low = buffer[index++];
                address += mne[opcode].size;
                printf("%02X %02X     ", opcode, low);
                break;

            case 3:
                low = buffer[index++];
                high = buffer[index++];
                address += mne[opcode].size;
                printf("%02X %02X %02X  ", opcode, low, high);
                break;

            default:
                address++;
                printf("%02X        ", opcode);
                break;
        }

        // mnemonic
        printf("%s ", mne[opcode].mnemonic);

        // Type
        switch (mne[opcode].type) {
            case IMMEDIATE:
                printf("#$%02X", low);
                break;
            case ABSOLUTE:
                printf("$%02X%02X", high, low);
                break;
            case ABSOLUTE_X:
                printf("$%02X%02X,X", high, low);
                break;
            case ABSOLUTE_Y:
                printf("$%02X%02X,Y", high, low);
                break;
            case ZEROPAGE:
                printf("$%02X", low);
                break;
            case INDIRECT_X:
                printf("($%02X,X)", low);
                break;
            case INDIRECT_Y:
                printf("($%02X),Y", low);
                break;
            case ZEROPAGE_X:
                printf("$%02X,X", low);
                break;
            case INDIRECT:
                printf("($%02X%02X)", high, low);
                break;
            case ZEROPAGE_Y:
                printf("$%02X,Y", low);
                break;
            case RELATIVE:
                printf("$%04X", (low <= 127) ? address + low : address - (256 - low));
                break;
            default:
                /* IMPLIED */
                break;
        }

        printf("\n");
    }

    return;
}
