#include "utils.h"

int mbstrlen(char *str) {
    /*
        Returns string length in symbols
        For multibyte strings
    */
    int r = 0;
    while (*str) {
        str += mbtowc(0, (const char*)str, 4);
        r++;
    }
    return r;
}

efi_input_key_t get_key() {
    /* Returns EFI Scancode of next pressed key */
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

size_t get_file_len(FILE *f) {
    /* Returns size of file f */
    size_t size;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}

//writes hex representation of hash string s to string d
void write_hash(uint8_t *s, char *d){
    for(unsigned int i = 0; i < 32; ++i){
        sprintf(&d[i*2], "%02x", s[i]);
    }
}

void file_write_datetime(FILE* f) {
    efi_time_t t;
    efi_time_capabilities_t cap;
    RT->GetTime(&t, &cap);
    fprintf(f, "[%04u-%02u-%02u %02u:%02u:%02u] ",
            (uint64_t)t.Year, (uint64_t)t.Month, (uint64_t)t.Day,
            (uint64_t)t.Hour, (uint64_t)t.Minute, (uint64_t)t.Second);
}


//switch to strncmp
/*

int hashcmp(uint8_t *src, uint8_t *dest, int len) {
    //print_hash(src);
    //print_hash(dest);
    for (int i = 0; i < len; i++)
        if (src[i] != dest[i])
            return 1;
    return 0;
}
*/