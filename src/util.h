#pragma once

#include <stdint.h>

#define PACKED __attribute__ ((__packed__))

extern const uint8_t pet_asc[];

const char *get_filetype(int ftype);
