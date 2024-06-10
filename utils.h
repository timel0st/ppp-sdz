#ifndef UTILS_H
#define UTILS_H
#include "uefi/uefi.h"

int mbstrlen(char *);
efi_input_key_t get_key();
size_t get_file_len(FILE *);
void write_hash(uint8_t *, char *);
void itoa(int i, char *a);

#endif