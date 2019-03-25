#include "basic.h"
#include "util.h"

#include <stdio.h>
#include <ctype.h>

#define INBUF_SIZE 80
#define LINK_SIZE 5
#define QUOTE '"'
#define LOW 128
#define HIGH 202

/* CBM Basic 2.0 statements */
const char *basic[] = {
    "end",     "for",    "next",   "data",
    "input#",  "input",  "dim",    "read",
    "let",     "goto",   "run",    "if",
    "restore", "gosub",  "return", "rem",
    "stop",    "on",     "wait",   "load",
    "save",    "verify", "def",    "poke",
    "print#",  "print",  "cont",   "list",
    "clr",     "cmd",    "sys",    "open",
    "close",   "get",    "new",    "tab(",
    "to",      "fn",     "spc(",   "then",
    "not",     "step",   "+",      "-",
    "*",       "/",      "^",      "and",
    "or",      ">",      "=",      "<",
    "sgn",     "int",    "abs",    "usr",
    "fre",     "pos",    "sqr",    "rnd",
    "log",     "exp",    "cos",    "sin",
    "tan",     "atn",    "peek",   "len",
    "str$",    "val",    "asc",    "chr$",
    "left$",   "right$", "mid$",   "unknown"
};

void showbasic(uint8_t *buffer, int size)
{
    // Get loading address and advance index
    uint16_t address = buffer[0] + ((buffer[1] & 0xff) << 8);
    int index = 2;

    do {
        // next line link
        uint8_t low = buffer[index++];
        uint8_t high = buffer[index++];

        // last line
        if (low == 0 && high == low)
            break;

        uint16_t next = (low + ((high & 0xff) << 8)) - address;
        address += next;

        // input buffer overflow
        if (next > INBUF_SIZE)
            break;

        // line number
        low = buffer[index++];
        high = buffer[index++];
        uint16_t line = low + ((high & 0xff) << 8);
        printf("%u ", line);

        // current line
        uint8_t quote = 0;
        for (int i = 0; i < next-LINK_SIZE; i++) {
            uint8_t input = buffer[index++];

            // toggle quote mode
            if (input == QUOTE)
              quote ^= input;

            if (quote == 0 && ((input >= LOW) && (input <= HIGH)))
                printf("%s", basic[input-LOW]);
            else
                printf("%c", isprint(pet_asc[input]) ? pet_asc[input] : ' ');
        }

        index++;
        printf("\n");
    } while (index < size);
}
