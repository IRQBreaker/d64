#include "disasm.h"

#include <stdio.h>
#include <stdint.h>

/* Addressing modes */
enum {
    IMPLIED,      // RTS
    IMMEDIATE,    // LDA #$00
    ABSOLUTE,     // JMP $C000
    ABSOLUTE_X,   // LDY $8000,X
    ABSOLUTE_Y,   // LDX $9000,Y
    ZEROPAGE,     // LDA $80
    INDIRECT_X,   // STA $(02,X)
    INDIRECT_Y,   // STA $(40),Y
    ZEROPAGE_X,   // LDA $20,X
    ZEROPAGE_Y,   // LDA $30,Y
    INDIRECT,     // JMP ($8000)
    RELATIVE      // BNE $9000
};

// Length of addressing modes
int op_length[] = {1, 2, 3, 3, 3, 2, 2, 2, 2, 2, 3, 2};

typedef struct
{
    char *mnemonic;
    int type;
} mnemonics;

/* name, type */
mnemonics mne_legal[] = {
    {"BRK", IMPLIED},    {"ORA", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ORA", ZEROPAGE},   {"ASL", ZEROPAGE},   {"???", IMPLIED},
    {"PHP", IMPLIED},    {"ORA", IMMEDIATE},  {"ASL", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ORA", ABSOLUTE},   {"ASL", ABSOLUTE},   {"???", IMPLIED},
    {"BPL", RELATIVE},   {"ORA", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ORA", ZEROPAGE_X}, {"ASL", ZEROPAGE_X}, {"???", IMPLIED},
    {"CLC", IMPLIED},    {"ORA", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ORA", ABSOLUTE_X}, {"ASL", ABSOLUTE_X}, {"???", IMPLIED},
    {"JSR", ABSOLUTE},   {"AND", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"BIT", ZEROPAGE},   {"AND", ZEROPAGE},   {"ROL", ZEROPAGE},   {"???", IMPLIED},
    {"PLP", IMPLIED},    {"AND", IMMEDIATE},  {"ROL", IMPLIED},    {"???", IMPLIED},
    {"BIT", ABSOLUTE},   {"AND", ABSOLUTE},   {"ROL", ABSOLUTE},   {"???", IMPLIED},
    {"BMI", RELATIVE},   {"AND", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"AND", ZEROPAGE_X}, {"ROL", ZEROPAGE_X}, {"???", IMPLIED},
    {"SEC", IMPLIED},    {"AND", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"AND", ABSOLUTE_X}, {"ROL", ABSOLUTE_X}, {"???", IMPLIED},
    {"RTI", IMPLIED},    {"EOR", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"EOR", ZEROPAGE},   {"LSR", ZEROPAGE},   {"???", IMPLIED},
    {"PHA", IMPLIED},    {"EOR", IMMEDIATE},  {"LSR", IMPLIED},    {"???", IMPLIED},
    {"JMP", ABSOLUTE},   {"EOR", ABSOLUTE},   {"LSR", ABSOLUTE},   {"???", IMPLIED},
    {"BVC", RELATIVE},   {"EOR", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"EOR", ZEROPAGE_X}, {"LSR", ZEROPAGE_X}, {"???", IMPLIED},
    {"CLI", IMPLIED},    {"EOR", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"EOR", ABSOLUTE_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"RTS", IMPLIED},    {"ADC", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ADC", ZEROPAGE},   {"ROR", ZEROPAGE},   {"???", IMPLIED},
    {"PLA", IMPLIED},    {"ADC", IMMEDIATE},  {"ROR", IMPLIED},    {"???", IMPLIED},
    {"JMP", INDIRECT},   {"ADC", ABSOLUTE},   {"ROR", ABSOLUTE},   {"???", IMPLIED},
    {"BVS", RELATIVE},   {"ADC", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ADC", ZEROPAGE_X}, {"ROR", ZEROPAGE_X}, {"???", IMPLIED},
    {"SEI", IMPLIED},    {"ADC", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ADC", ABSOLUTE_X}, {"ROR", ABSOLUTE_X}, {"???", IMPLIED},
    {"???", IMPLIED},    {"STA", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"STY", ZEROPAGE},   {"STA", ZEROPAGE},   {"STX", ZEROPAGE},   {"???", IMPLIED},
    {"DEY", IMPLIED},    {"???", IMPLIED},    {"TXA", IMPLIED},    {"???", IMPLIED},
    {"STY", ABSOLUTE},   {"STA", ABSOLUTE},   {"STX", ABSOLUTE},   {"???", IMPLIED},
    {"BCC", RELATIVE},   {"STA", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"STY", ZEROPAGE_X}, {"STA", ZEROPAGE_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"TYA", IMPLIED},    {"STA", ABSOLUTE_Y}, {"TXS", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"STA", ABSOLUTE_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"LDY", IMMEDIATE},  {"LDA", INDIRECT_X}, {"LDX", IMMEDIATE},  {"???", IMPLIED},
    {"LDY", ZEROPAGE},   {"LDA", ZEROPAGE},   {"LDX", ZEROPAGE},   {"???", IMPLIED},
    {"TAY", IMPLIED},    {"LDA", IMMEDIATE},  {"TAX", IMPLIED},    {"???", IMPLIED},
    {"LDY", ABSOLUTE},   {"LDA", ABSOLUTE},   {"LDX", ABSOLUTE},   {"???", IMPLIED},
    {"BCS", RELATIVE},   {"LDA", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"LDY", ZEROPAGE_X}, {"LDA", ZEROPAGE_X}, {"LDX", ZEROPAGE_Y}, {"???", IMPLIED},
    {"CLV", IMPLIED},    {"LDA", ABSOLUTE_Y}, {"TSX", IMPLIED},    {"???", IMPLIED},
    {"LDY", ABSOLUTE_X}, {"LDA", ABSOLUTE_X}, {"LDX", ABSOLUTE_Y}, {"???", IMPLIED},
    {"CPY", IMMEDIATE},  {"CMP", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"CPY", ZEROPAGE},   {"CMP", ZEROPAGE},   {"DEC", ZEROPAGE},   {"???", IMPLIED},
    {"INY", IMPLIED},    {"CMP", IMMEDIATE},  {"DEX", IMPLIED},    {"???", IMPLIED},
    {"CPY", ABSOLUTE},   {"CMP", ABSOLUTE},   {"DEC", ABSOLUTE},   {"???", IMPLIED},
    {"BNE", RELATIVE},   {"CMP", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"CMP", ZEROPAGE_X}, {"DEC", ZEROPAGE_X}, {"???", IMPLIED},
    {"CLD", IMPLIED},    {"CMP", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"CMP", ABSOLUTE_X}, {"DEC", ABSOLUTE_X}, {"???", IMPLIED},
    {"CPX", IMMEDIATE},  {"SBC", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"CPX", ZEROPAGE},   {"SBC", ZEROPAGE},   {"INC", ZEROPAGE},   {"???", IMPLIED},
    {"INX", IMPLIED},    {"SBC", IMMEDIATE},  {"NOP", IMPLIED},    {"???", IMPLIED},
    {"CPX", ABSOLUTE},   {"SBC", ABSOLUTE},   {"INC", ABSOLUTE},   {"???", IMPLIED},
    {"BEQ", RELATIVE},   {"SBC", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"SBC", ZEROPAGE_X}, {"INC", ZEROPAGE_X}, {"???", IMPLIED},
    {"SED", IMPLIED},    {"SBC", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"SBC", ABSOLUTE_X}, {"INC", ABSOLUTE_X}, {"???", IMPLIED}
};

/* name, type */
mnemonics mne_illegal[] = {
    {"BRK", IMPLIED},    {"ORA", INDIRECT_X}, {"KIL", IMPLIED},    {"SLO", INDIRECT_X},
    {"NOP", ZEROPAGE},   {"ORA", ZEROPAGE},   {"ASL", ZEROPAGE},   {"SLO", ZEROPAGE},
    {"PHP", IMPLIED},    {"ORA", IMMEDIATE},  {"ASL", IMPLIED},    {"ANC", IMMEDIATE},
    {"NOP", ABSOLUTE},   {"ORA", ABSOLUTE},   {"ASL", ABSOLUTE},   {"SLO", ABSOLUTE},
    {"BPL", RELATIVE},   {"ORA", INDIRECT_Y}, {"KIL", IMPLIED},    {"SLO", INDIRECT_Y},
    {"NOP", ZEROPAGE_X}, {"ORA", ZEROPAGE_X}, {"ASL", ZEROPAGE_X}, {"SLO", ZEROPAGE_X},
    {"CLC", IMPLIED},    {"ORA", ABSOLUTE_Y}, {"NOP", IMPLIED},    {"SLO", ABSOLUTE_Y},
    {"NOP", ABSOLUTE_X}, {"ORA", ABSOLUTE_X}, {"ASL", ABSOLUTE_X}, {"SLO", ABSOLUTE_X},
    {"JSR", ABSOLUTE},   {"AND", INDIRECT_X}, {"KIL", IMPLIED},    {"RLA", INDIRECT_X},
    {"BIT", ZEROPAGE},   {"AND", ZEROPAGE},   {"ROL", ZEROPAGE},   {"RLA", ZEROPAGE},
    {"PLP", IMPLIED},    {"AND", IMMEDIATE},  {"ROL", IMPLIED},    {"ANC", IMMEDIATE},
    {"BIT", ABSOLUTE},   {"AND", ABSOLUTE},   {"ROL", ABSOLUTE},   {"RLA", ABSOLUTE},
    {"BMI", RELATIVE},   {"AND", INDIRECT_Y}, {"KIL", IMPLIED},    {"RLA", INDIRECT_Y},
    {"NOP", ZEROPAGE_X}, {"AND", ZEROPAGE_X}, {"ROL", ZEROPAGE_X}, {"RLA", ZEROPAGE_X},
    {"SEC", IMPLIED},    {"AND", ABSOLUTE_Y}, {"NOP", IMPLIED},    {"RLA", ABSOLUTE_Y},
    {"NOP", ABSOLUTE_X}, {"AND", ABSOLUTE_X}, {"ROL", ABSOLUTE_X}, {"RLA", ABSOLUTE_X},
    {"RTI", IMPLIED},    {"EOR", INDIRECT_X}, {"KIL", IMPLIED},    {"SRE", INDIRECT_X},
    {"NOP", ZEROPAGE},   {"EOR", ZEROPAGE},   {"LSR", ZEROPAGE},   {"SRE", ZEROPAGE},
    {"PHA", IMPLIED},    {"EOR", IMMEDIATE},  {"LSR", IMPLIED},    {"ALR", IMMEDIATE},
    {"JMP", ABSOLUTE},   {"EOR", ABSOLUTE},   {"LSR", ABSOLUTE},   {"SRE", ABSOLUTE},
    {"BVC", RELATIVE},   {"EOR", INDIRECT_Y}, {"KIL", IMPLIED},    {"SRE", INDIRECT_Y},
    {"NOP", ZEROPAGE_X}, {"EOR", ZEROPAGE_X}, {"LSR", ZEROPAGE_X}, {"SRE", ZEROPAGE_X},
    {"CLI", IMPLIED},    {"EOR", ABSOLUTE_Y}, {"NOP", IMPLIED},    {"SRE", ABSOLUTE_Y},
    {"NOP", ABSOLUTE_X}, {"EOR", ABSOLUTE_X}, {"LSR", ABSOLUTE_X}, {"SRE", ABSOLUTE_X},
    {"RTS", IMPLIED},    {"ADC", INDIRECT_X}, {"KIL", IMPLIED},    {"RRA", INDIRECT_X},
    {"NOP", ZEROPAGE},   {"ADC", ZEROPAGE},   {"ROR", ZEROPAGE},   {"RRA", ZEROPAGE},
    {"PLA", IMPLIED},    {"ADC", IMMEDIATE},  {"ROR", IMPLIED},    {"ARR", IMMEDIATE},
    {"JMP", INDIRECT},   {"ADC", ABSOLUTE},   {"ROR", ABSOLUTE},   {"RRA", ABSOLUTE},
    {"BVS", RELATIVE},   {"ADC", INDIRECT_Y}, {"KIL", IMPLIED},    {"RRA", INDIRECT_Y},
    {"NOP", ZEROPAGE_X}, {"ADC", ZEROPAGE_X}, {"ROR", ZEROPAGE_X}, {"RRA", ZEROPAGE_X},
    {"SEI", IMPLIED},    {"ADC", ABSOLUTE_Y}, {"NOP", IMPLIED},    {"RRA", ABSOLUTE_Y},
    {"NOP", ABSOLUTE_X}, {"ADC", ABSOLUTE_X}, {"ROR", ABSOLUTE_X}, {"RRA", ABSOLUTE_X},
    {"NOP", IMMEDIATE},  {"STA", INDIRECT_X}, {"NOP", IMMEDIATE},  {"SAX", INDIRECT_X},
    {"STY", ZEROPAGE},   {"STA", ZEROPAGE},   {"STX", ZEROPAGE},   {"SAX", ZEROPAGE},
    {"DEY", IMPLIED},    {"NOP", IMMEDIATE},  {"TXA", IMPLIED},    {"XAA", IMMEDIATE},
    {"STY", ABSOLUTE},   {"STA", ABSOLUTE},   {"STX", ABSOLUTE},   {"SAX", ABSOLUTE},
    {"BCC", RELATIVE},   {"STA", INDIRECT_Y}, {"KIL", IMPLIED},    {"AHX", INDIRECT_Y},
    {"STY", ZEROPAGE_X}, {"STA", ZEROPAGE_X}, {"STX", ZEROPAGE_Y}, {"SAX", ZEROPAGE_Y},
    {"TYA", IMPLIED},    {"STA", ABSOLUTE_Y}, {"TXS", IMPLIED},    {"TAS", IMPLIED},
    {"SHY", ABSOLUTE_X}, {"STA", ABSOLUTE_X}, {"SHX", ABSOLUTE_Y}, {"AHX", ABSOLUTE_Y},
    {"LDY", IMMEDIATE},  {"LDA", INDIRECT_X}, {"LDX", IMMEDIATE},  {"LAX", INDIRECT_X},
    {"LDY", ZEROPAGE},   {"LDA", ZEROPAGE},   {"LDX", ZEROPAGE},   {"LAX", ZEROPAGE},
    {"TAY", IMPLIED},    {"LDA", IMMEDIATE},  {"TAX", IMPLIED},    {"LAX", IMMEDIATE},
    {"LDY", ABSOLUTE},   {"LDA", ABSOLUTE},   {"LDX", ABSOLUTE},   {"LAX", ABSOLUTE},
    {"BCS", RELATIVE},   {"LDA", INDIRECT_Y}, {"KIL", IMPLIED},    {"LAX", INDIRECT_Y},
    {"LDY", ZEROPAGE_X}, {"LDA", ZEROPAGE_X}, {"LDX", ZEROPAGE_Y}, {"LAX", ZEROPAGE_Y},
    {"CLV", IMPLIED},    {"LDA", ABSOLUTE_Y}, {"TSX", IMPLIED},    {"LAS", ABSOLUTE_Y},
    {"LDY", ABSOLUTE_X}, {"LDA", ABSOLUTE_X}, {"LDX", ABSOLUTE_Y}, {"LAX", ABSOLUTE_Y},
    {"CPY", IMMEDIATE},  {"CMP", INDIRECT_X}, {"NOP", IMMEDIATE},  {"DCP", INDIRECT_X},
    {"CPY", ZEROPAGE},   {"CMP", ZEROPAGE},   {"DEC", ZEROPAGE},   {"DCP", ZEROPAGE},
    {"INY", IMPLIED},    {"CMP", IMMEDIATE},  {"DEX", IMPLIED},    {"AXS", IMMEDIATE},
    {"CPY", ABSOLUTE},   {"CMP", ABSOLUTE},   {"DEC", ABSOLUTE},   {"DCP", ABSOLUTE},
    {"BNE", RELATIVE},   {"CMP", INDIRECT_Y}, {"KIL", IMPLIED},    {"DCP", INDIRECT_Y},
    {"NOP", ZEROPAGE_X}, {"CMP", ZEROPAGE_X}, {"DEC", ZEROPAGE_X}, {"DCP", ZEROPAGE_X},
    {"CLD", IMPLIED},    {"CMP", ABSOLUTE_Y}, {"NOP", IMPLIED},    {"DCP", ABSOLUTE_Y},
    {"NOP", ABSOLUTE_X}, {"CMP", ABSOLUTE_X}, {"DEC", ABSOLUTE_X}, {"DCP", ABSOLUTE_X},
    {"CPX", IMMEDIATE},  {"SBC", INDIRECT_X}, {"NOP", IMMEDIATE},  {"ISC", INDIRECT_X},
    {"CPX", ZEROPAGE},   {"SBC", ZEROPAGE},   {"INC", ZEROPAGE},   {"ISC", ZEROPAGE},
    {"INX", IMPLIED},    {"SBC", IMMEDIATE},  {"NOP", IMPLIED},    {"SBC", IMMEDIATE},
    {"CPX", ABSOLUTE},   {"SBC", ABSOLUTE},   {"INC", ABSOLUTE},   {"ISC", ABSOLUTE},
    {"BEQ", RELATIVE},   {"SBC", INDIRECT_Y}, {"KIL", IMPLIED},    {"ISC", INDIRECT_Y},
    {"NOP", ZEROPAGE_X}, {"SBC", ZEROPAGE_X}, {"INC", ZEROPAGE_X}, {"ISC", ZEROPAGE_X},
    {"SED", IMPLIED},    {"SBC", ABSOLUTE_Y}, {"NOP", IMPLIED},    {"ISC", ABSOLUTE_Y},
    {"NOP", ABSOLUTE_X}, {"SBC", ABSOLUTE_X}, {"INC", ABSOLUTE_X}, {"ISC", ABSOLUTE_X}
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
        switch (op_length[mne[opcode].type]) {
            case 2:
                low = buffer[index++];
                address += op_length[mne[opcode].type];
                printf("%02X %02X     ", opcode, low);
                break;

            case 3:
                low = buffer[index++];
                high = buffer[index++];
                address += op_length[mne[opcode].type];
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
