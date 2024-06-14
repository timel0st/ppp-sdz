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
    size_t cur = ftell(f);
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, cur, SEEK_SET);
    return size;
}

/* Writes hex representation of hash string s to string d */
void write_hash(uint8_t *s, char *d) {
    for(unsigned int i = 0; i < 32; ++i){
        sprintf(&d[i>>1], "%02x", s[i]);
    }
}

/* Reverse character order in string s of length len */
void reverse_string(char *s, uint32_t len) {
    for (int i = 0; i < len/2; i++) {
        char c = s[len-1-i];
        s[len-1-i] = s[i];
        s[i] = c;
    }
    s[len] = 0;
}

/* 
    Writes string representation of integer i to string a 
    Works only for unsigned values of i
*/
void itoa(uint32_t i, char *a) {
    int j = 0;
    for (; i > 0; j++, i /= 10)
        a[j] = (i % 10) + 0x30;
    if (!j)
        a[j++] = '0';
    reverse_string(a, j);
}