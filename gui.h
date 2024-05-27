#ifndef GUI_H
#define GUI_H

#include "uefi/uefi.h"

#define FALSE 0
#define TRUE 1

/* GUI color theme */
#define HEADER_COLOR 0x081030 // "Space cadet" background color
#define BG_COLOR 0x0C1940 // slightly darker bg col
#define INPUT_BG_COLOR 0x091133// unselected input field col
#define INPUT_ACTIVE_COLOR 0x354369 // active input col
#define SELECT_COLOR 0x293960 // sligtly lighter select col
#define FONT_COLOR 0xFFFFFF // color of font
#define HEADER_HEIGHT 34 // header and footer height in px

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t *lfb;
} display_t;

boolean_t locate_gop(efi_gop_t **pgop);

#endif