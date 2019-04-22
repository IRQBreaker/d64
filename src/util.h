#pragma once

#include <stdint.h>

#define PACKED __attribute__ ((__packed__))

extern const uint8_t pet_asc[];
extern const char *c64_file_type[];
extern const int c64_file_type_size;

const char *get_filetype(int ftype);
