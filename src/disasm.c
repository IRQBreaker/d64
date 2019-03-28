#include "disasm.h"

#include <stdio.h>

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
const int op_length[] = {1, 2, 3, 3, 3, 2, 2, 2, 2, 2, 3, 2};

typedef struct
{
    char *mnemonic;
    int type;
} mnemonics;

/* name, type */
const mnemonics mne_legal[] = {
    {"brk", IMPLIED},    {"ora", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ora", ZEROPAGE},   {"asl", ZEROPAGE},   {"???", IMPLIED},
    {"php", IMPLIED},    {"ora", IMMEDIATE},  {"asl", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ora", ABSOLUTE},   {"asl", ABSOLUTE},   {"???", IMPLIED},
    {"bpl", RELATIVE},   {"ora", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ora", ZEROPAGE_X}, {"asl", ZEROPAGE_X}, {"???", IMPLIED},
    {"clc", IMPLIED},    {"ora", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"ora", ABSOLUTE_X}, {"asl", ABSOLUTE_X}, {"???", IMPLIED},
    {"jsr", ABSOLUTE},   {"and", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"bit", ZEROPAGE},   {"and", ZEROPAGE},   {"rol", ZEROPAGE},   {"???", IMPLIED},
    {"plp", IMPLIED},    {"and", IMMEDIATE},  {"rol", IMPLIED},    {"???", IMPLIED},
    {"bit", ABSOLUTE},   {"and", ABSOLUTE},   {"rol", ABSOLUTE},   {"???", IMPLIED},
    {"bmi", RELATIVE},   {"and", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"and", ZEROPAGE_X}, {"rol", ZEROPAGE_X}, {"???", IMPLIED},
    {"sec", IMPLIED},    {"and", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"and", ABSOLUTE_X}, {"rol", ABSOLUTE_X}, {"???", IMPLIED},
    {"rti", IMPLIED},    {"eor", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"eor", ZEROPAGE},   {"lsr", ZEROPAGE},   {"???", IMPLIED},
    {"pha", IMPLIED},    {"eor", IMMEDIATE},  {"lsr", IMPLIED},    {"???", IMPLIED},
    {"jmp", ABSOLUTE},   {"eor", ABSOLUTE},   {"lsr", ABSOLUTE},   {"???", IMPLIED},
    {"bvc", RELATIVE},   {"eor", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"eor", ZEROPAGE_X}, {"lsr", ZEROPAGE_X}, {"???", IMPLIED},
    {"cli", IMPLIED},    {"eor", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"eor", ABSOLUTE_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"rts", IMPLIED},    {"adc", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"adc", ZEROPAGE},   {"ror", ZEROPAGE},   {"???", IMPLIED},
    {"pla", IMPLIED},    {"adc", IMMEDIATE},  {"ror", IMPLIED},    {"???", IMPLIED},
    {"jmp", INDIRECT},   {"adc", ABSOLUTE},   {"ror", ABSOLUTE},   {"???", IMPLIED},
    {"bvs", RELATIVE},   {"adc", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"adc", ZEROPAGE_X}, {"ror", ZEROPAGE_X}, {"???", IMPLIED},
    {"sei", IMPLIED},    {"adc", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"adc", ABSOLUTE_X}, {"ror", ABSOLUTE_X}, {"???", IMPLIED},
    {"???", IMPLIED},    {"sta", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"sty", ZEROPAGE},   {"sta", ZEROPAGE},   {"stx", ZEROPAGE},   {"???", IMPLIED},
    {"dey", IMPLIED},    {"???", IMPLIED},    {"txa", IMPLIED},    {"???", IMPLIED},
    {"sty", ABSOLUTE},   {"sta", ABSOLUTE},   {"stx", ABSOLUTE},   {"???", IMPLIED},
    {"bcc", RELATIVE},   {"sta", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"sty", ZEROPAGE_X}, {"sta", ZEROPAGE_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"tya", IMPLIED},    {"sta", ABSOLUTE_Y}, {"txs", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"sta", ABSOLUTE_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"ldy", IMMEDIATE},  {"lda", INDIRECT_X}, {"ldx", IMMEDIATE},  {"???", IMPLIED},
    {"ldy", ZEROPAGE},   {"lda", ZEROPAGE},   {"ldx", ZEROPAGE},   {"???", IMPLIED},
    {"tay", IMPLIED},    {"lda", IMMEDIATE},  {"tax", IMPLIED},    {"???", IMPLIED},
    {"ldy", ABSOLUTE},   {"lda", ABSOLUTE},   {"ldx", ABSOLUTE},   {"???", IMPLIED},
    {"bcs", RELATIVE},   {"lda", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"ldy", ZEROPAGE_X}, {"lda", ZEROPAGE_X}, {"ldx", ZEROPAGE_Y}, {"???", IMPLIED},
    {"clv", IMPLIED},    {"lda", ABSOLUTE_Y}, {"tsx", IMPLIED},    {"???", IMPLIED},
    {"ldy", ABSOLUTE_X}, {"lda", ABSOLUTE_X}, {"ldx", ABSOLUTE_Y}, {"???", IMPLIED},
    {"cpy", IMMEDIATE},  {"cmp", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"cpy", ZEROPAGE},   {"cmp", ZEROPAGE},   {"dec", ZEROPAGE},   {"???", IMPLIED},
    {"iny", IMPLIED},    {"cmp", IMMEDIATE},  {"dex", IMPLIED},    {"???", IMPLIED},
    {"cpy", ABSOLUTE},   {"cmp", ABSOLUTE},   {"dec", ABSOLUTE},   {"???", IMPLIED},
    {"bne", RELATIVE},   {"cmp", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"cmp", ZEROPAGE_X}, {"dec", ZEROPAGE_X}, {"???", IMPLIED},
    {"cld", IMPLIED},    {"cmp", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"cmp", ABSOLUTE_X}, {"dec", ABSOLUTE_X}, {"???", IMPLIED},
    {"cpx", IMMEDIATE},  {"sbc", INDIRECT_X}, {"???", IMPLIED},    {"???", IMPLIED},
    {"cpx", ZEROPAGE},   {"sbc", ZEROPAGE},   {"inc", ZEROPAGE},   {"???", IMPLIED},
    {"inx", IMPLIED},    {"sbc", IMMEDIATE},  {"nop", IMPLIED},    {"???", IMPLIED},
    {"cpx", ABSOLUTE},   {"sbc", ABSOLUTE},   {"inc", ABSOLUTE},   {"???", IMPLIED},
    {"beq", RELATIVE},   {"sbc", INDIRECT_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"sbc", ZEROPAGE_X}, {"inc", ZEROPAGE_X}, {"???", IMPLIED},
    {"sed", IMPLIED},    {"sbc", ABSOLUTE_Y}, {"???", IMPLIED},    {"???", IMPLIED},
    {"???", IMPLIED},    {"sbc", ABSOLUTE_X}, {"inc", ABSOLUTE_X}, {"???", IMPLIED}
};

/* name, type */
const mnemonics mne_illegal[] = {
    {"brk", IMPLIED},    {"ora", INDIRECT_X}, {"kil", IMPLIED},    {"slo", INDIRECT_X},
    {"nop", ZEROPAGE},   {"ora", ZEROPAGE},   {"asl", ZEROPAGE},   {"slo", ZEROPAGE},
    {"php", IMPLIED},    {"ora", IMMEDIATE},  {"asl", IMPLIED},    {"anc", IMMEDIATE},
    {"nop", ABSOLUTE},   {"ora", ABSOLUTE},   {"asl", ABSOLUTE},   {"slo", ABSOLUTE},
    {"bpl", RELATIVE},   {"ora", INDIRECT_Y}, {"kil", IMPLIED},    {"slo", INDIRECT_Y},
    {"nop", ZEROPAGE_X}, {"ora", ZEROPAGE_X}, {"asl", ZEROPAGE_X}, {"slo", ZEROPAGE_X},
    {"clc", IMPLIED},    {"ora", ABSOLUTE_Y}, {"nop", IMPLIED},    {"slo", ABSOLUTE_Y},
    {"nop", ABSOLUTE_X}, {"ora", ABSOLUTE_X}, {"asl", ABSOLUTE_X}, {"slo", ABSOLUTE_X},
    {"jsr", ABSOLUTE},   {"and", INDIRECT_X}, {"kil", IMPLIED},    {"rla", INDIRECT_X},
    {"bit", ZEROPAGE},   {"and", ZEROPAGE},   {"rol", ZEROPAGE},   {"rla", ZEROPAGE},
    {"plp", IMPLIED},    {"and", IMMEDIATE},  {"rol", IMPLIED},    {"anc", IMMEDIATE},
    {"bit", ABSOLUTE},   {"and", ABSOLUTE},   {"rol", ABSOLUTE},   {"rla", ABSOLUTE},
    {"bmi", RELATIVE},   {"and", INDIRECT_Y}, {"kil", IMPLIED},    {"rla", INDIRECT_Y},
    {"nop", ZEROPAGE_X}, {"and", ZEROPAGE_X}, {"rol", ZEROPAGE_X}, {"rla", ZEROPAGE_X},
    {"sec", IMPLIED},    {"and", ABSOLUTE_Y}, {"nop", IMPLIED},    {"rla", ABSOLUTE_Y},
    {"nop", ABSOLUTE_X}, {"and", ABSOLUTE_X}, {"rol", ABSOLUTE_X}, {"rla", ABSOLUTE_X},
    {"rti", IMPLIED},    {"eor", INDIRECT_X}, {"kil", IMPLIED},    {"sre", INDIRECT_X},
    {"nop", ZEROPAGE},   {"eor", ZEROPAGE},   {"lsr", ZEROPAGE},   {"sre", ZEROPAGE},
    {"pha", IMPLIED},    {"eor", IMMEDIATE},  {"lsr", IMPLIED},    {"alr", IMMEDIATE},
    {"jmp", ABSOLUTE},   {"eor", ABSOLUTE},   {"lsr", ABSOLUTE},   {"sre", ABSOLUTE},
    {"bvc", RELATIVE},   {"eor", INDIRECT_Y}, {"kil", IMPLIED},    {"sre", INDIRECT_Y},
    {"nop", ZEROPAGE_X}, {"eor", ZEROPAGE_X}, {"lsr", ZEROPAGE_X}, {"sre", ZEROPAGE_X},
    {"cli", IMPLIED},    {"eor", ABSOLUTE_Y}, {"nop", IMPLIED},    {"sre", ABSOLUTE_Y},
    {"nop", ABSOLUTE_X}, {"eor", ABSOLUTE_X}, {"lsr", ABSOLUTE_X}, {"sre", ABSOLUTE_X},
    {"rts", IMPLIED},    {"adc", INDIRECT_X}, {"kil", IMPLIED},    {"rra", INDIRECT_X},
    {"nop", ZEROPAGE},   {"adc", ZEROPAGE},   {"ror", ZEROPAGE},   {"rra", ZEROPAGE},
    {"pla", IMPLIED},    {"adc", IMMEDIATE},  {"ror", IMPLIED},    {"arr", IMMEDIATE},
    {"jmp", INDIRECT},   {"adc", ABSOLUTE},   {"ror", ABSOLUTE},   {"rra", ABSOLUTE},
    {"bvs", RELATIVE},   {"adc", INDIRECT_Y}, {"kil", IMPLIED},    {"rra", INDIRECT_Y},
    {"nop", ZEROPAGE_X}, {"adc", ZEROPAGE_X}, {"ror", ZEROPAGE_X}, {"rra", ZEROPAGE_X},
    {"sei", IMPLIED},    {"adc", ABSOLUTE_Y}, {"nop", IMPLIED},    {"rra", ABSOLUTE_Y},
    {"nop", ABSOLUTE_X}, {"adc", ABSOLUTE_X}, {"ror", ABSOLUTE_X}, {"rra", ABSOLUTE_X},
    {"nop", IMMEDIATE},  {"sta", INDIRECT_X}, {"nop", IMMEDIATE},  {"sax", INDIRECT_X},
    {"sty", ZEROPAGE},   {"sta", ZEROPAGE},   {"stx", ZEROPAGE},   {"sax", ZEROPAGE},
    {"dey", IMPLIED},    {"nop", IMMEDIATE},  {"txa", IMPLIED},    {"xaa", IMMEDIATE},
    {"sty", ABSOLUTE},   {"sta", ABSOLUTE},   {"stx", ABSOLUTE},   {"sax", ABSOLUTE},
    {"bcc", RELATIVE},   {"sta", INDIRECT_Y}, {"kil", IMPLIED},    {"ahx", INDIRECT_Y},
    {"sty", ZEROPAGE_X}, {"sta", ZEROPAGE_X}, {"stx", ZEROPAGE_Y}, {"sax", ZEROPAGE_Y},
    {"tya", IMPLIED},    {"sta", ABSOLUTE_Y}, {"txs", IMPLIED},    {"tas", IMPLIED},
    {"shy", ABSOLUTE_X}, {"sta", ABSOLUTE_X}, {"shx", ABSOLUTE_Y}, {"ahx", ABSOLUTE_Y},
    {"ldy", IMMEDIATE},  {"lda", INDIRECT_X}, {"ldx", IMMEDIATE},  {"lax", INDIRECT_X},
    {"ldy", ZEROPAGE},   {"lda", ZEROPAGE},   {"ldx", ZEROPAGE},   {"lax", ZEROPAGE},
    {"tay", IMPLIED},    {"lda", IMMEDIATE},  {"tax", IMPLIED},    {"lax", IMMEDIATE},
    {"ldy", ABSOLUTE},   {"lda", ABSOLUTE},   {"ldx", ABSOLUTE},   {"lax", ABSOLUTE},
    {"bcs", RELATIVE},   {"lda", INDIRECT_Y}, {"kil", IMPLIED},    {"lax", INDIRECT_Y},
    {"ldy", ZEROPAGE_X}, {"lda", ZEROPAGE_X}, {"ldx", ZEROPAGE_Y}, {"lax", ZEROPAGE_Y},
    {"clv", IMPLIED},    {"lda", ABSOLUTE_Y}, {"tsx", IMPLIED},    {"las", ABSOLUTE_Y},
    {"ldy", ABSOLUTE_X}, {"lda", ABSOLUTE_X}, {"ldx", ABSOLUTE_Y}, {"lax", ABSOLUTE_Y},
    {"cpy", IMMEDIATE},  {"cmp", INDIRECT_X}, {"nop", IMMEDIATE},  {"dcp", INDIRECT_X},
    {"cpy", ZEROPAGE},   {"cmp", ZEROPAGE},   {"dec", ZEROPAGE},   {"dcp", ZEROPAGE},
    {"iny", IMPLIED},    {"cmp", IMMEDIATE},  {"dex", IMPLIED},    {"axs", IMMEDIATE},
    {"cpy", ABSOLUTE},   {"cmp", ABSOLUTE},   {"dec", ABSOLUTE},   {"dcp", ABSOLUTE},
    {"bne", RELATIVE},   {"cmp", INDIRECT_Y}, {"kil", IMPLIED},    {"dcp", INDIRECT_Y},
    {"nop", ZEROPAGE_X}, {"cmp", ZEROPAGE_X}, {"dec", ZEROPAGE_X}, {"dcp", ZEROPAGE_X},
    {"cld", IMPLIED},    {"cmp", ABSOLUTE_Y}, {"nop", IMPLIED},    {"dcp", ABSOLUTE_Y},
    {"nop", ABSOLUTE_X}, {"cmp", ABSOLUTE_X}, {"dec", ABSOLUTE_X}, {"dcp", ABSOLUTE_X},
    {"cpx", IMMEDIATE},  {"sbc", INDIRECT_X}, {"nop", IMMEDIATE},  {"isc", INDIRECT_X},
    {"cpx", ZEROPAGE},   {"sbc", ZEROPAGE},   {"inc", ZEROPAGE},   {"isc", ZEROPAGE},
    {"inx", IMPLIED},    {"sbc", IMMEDIATE},  {"nop", IMPLIED},    {"sbc", IMMEDIATE},
    {"cpx", ABSOLUTE},   {"sbc", ABSOLUTE},   {"inc", ABSOLUTE},   {"isc", ABSOLUTE},
    {"beq", RELATIVE},   {"sbc", INDIRECT_Y}, {"kil", IMPLIED},    {"isc", INDIRECT_Y},
    {"nop", ZEROPAGE_X}, {"sbc", ZEROPAGE_X}, {"inc", ZEROPAGE_X}, {"isc", ZEROPAGE_X},
    {"sed", IMPLIED},    {"sbc", ABSOLUTE_Y}, {"nop", IMPLIED},    {"isc", ABSOLUTE_Y},
    {"nop", ABSOLUTE_X}, {"sbc", ABSOLUTE_X}, {"inc", ABSOLUTE_X}, {"isc", ABSOLUTE_X}
};

void disasm(const uint8_t *buffer, const int size, const int illegal)
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
        printf("%04x ", address);

        // hexdump
        switch (op_length[mne[opcode].type]) {
            case 2:
                low = buffer[index++];
                printf("%02x %02x     ", opcode, low);
                break;

            case 3:
                low = buffer[index++];
                high = buffer[index++];
                printf("%02x %02x %02x  ", opcode, low, high);
                break;

            default:
                printf("%02x        ", opcode);
                break;
        }
        address += op_length[mne[opcode].type];

        // mnemonic
        printf("%s ", mne[opcode].mnemonic);

        // Type
        switch (mne[opcode].type) {
            case IMMEDIATE:
                printf("#$%02x", low);
                break;
            case ABSOLUTE:
                printf("$%02x%02x", high, low);
                break;
            case ABSOLUTE_X:
                printf("$%02x%02x,x", high, low);
                break;
            case ABSOLUTE_Y:
                printf("$%02x%02x,y", high, low);
                break;
            case ZEROPAGE:
                printf("$%02x", low);
                break;
            case INDIRECT_X:
                printf("($%02x,x)", low);
                break;
            case INDIRECT_Y:
                printf("($%02x),y", low);
                break;
            case ZEROPAGE_X:
                printf("$%02x,x", low);
                break;
            case ZEROPAGE_Y:
                printf("$%02x,y", low);
                break;
            case INDIRECT:
                printf("($%02x%02x)", high, low);
                break;
            case RELATIVE:
                printf("$%04x", (low <= 127) ? address + low : address - (256 - low));
                break;
            default:
                /* IMPLIED */
                break;
        }

        printf("\n");
    }

    return;
}
