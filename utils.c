#include "utils.h"

/*
    Returns string length in symbols
    For multibyte strings
*/
int mbstrlen(char *str) {
    int r = 0;
    while (*str) {
        str += mbtowc(0, (const char*)str, 4);
        r++;
    }
    return r;
}

/* Returns EFI Scancode of next pressed key */
efi_input_key_t get_key() {
    efi_event_t events[1];
    efi_input_key_t key;
    key.ScanCode = 0;
    key.UnicodeChar = u'\0';
    events[0] = ST->ConIn->WaitForKey;
    uintn_t index = 0;
    BS->WaitForEvent(1, events, &index);
    if (index == 0) ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
    return key;
}

/* Returns size of file f */
size_t get_file_len(FILE *f) {
    size_t size;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}

/* Writes hex representation of hash string s to string d */
void write_hash(uint8_t *s, char *d) {
    for(unsigned int i = 0; i < 32; ++i){
        sprintf(&d[i>>1], "%02x", s[i]);
    }
}