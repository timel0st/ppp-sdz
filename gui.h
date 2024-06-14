#ifndef GUI_H
#define GUI_H

#include "uefi/uefi.h"
#include "utils.h"

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

typedef enum {
    ITEM_SELECTABLE,
    ITEM_INPUT
} item_type_t;

typedef enum {
    INPUT_DEFAULT,
    INPUT_PASSWORD,
    INPUT_DIGITS
} input_type_t;

/* SSFN Font struct */
typedef struct {
    unsigned char  magic[4];
    unsigned int   size;
    unsigned char  type;
    unsigned char  features;
    unsigned char  width;
    unsigned char  height;
    unsigned char  baseline;
    unsigned char  underline;
    unsigned short fragments_offs;
    unsigned int   characters_offs;
    unsigned int   ligature_offs;
    unsigned int   kerning_offs;
    unsigned int   cmap_offs;
} __attribute__((packed)) ssfn_font_t;

/* display params struct */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t *lfb;
} display_t;

/* menu item struct */
typedef struct {
    int     type;
    boolean_t is_selected;
    int x;
    int y;
    void*   item;
} item_t;

/* whole menu struct */
typedef struct {
    item_t **items;
    int len; //length of items in menu
    int cur; //active selection
} menu_t;

/* input item struct */
typedef struct {
    char*       label;
    char*       buf;
    uint8_t   flag;
    boolean_t   is_active;
    int         len; // of buffer
} input_t;

/* selectable item struct */
typedef struct {
    char*   label;
    void*   action;
    int     arg;
} select_t;

boolean_t locate_gop(efi_gop_t **pgop);
boolean_t set_video_mode(display_t *d);
boolean_t load_font(char* path);
void print_string(int x, int y, const char *s);
void draw_box(uintn_t sx, uintn_t sy, uintn_t w, uintn_t h, uint32_t col); 
void print_hidden(int x, int y, int len);
void draw_input(item_t *inp);
void draw_selectable(item_t *sel);
void process_input(item_t *it);
menu_t create_menu(item_t **items, int len);
item_t* create_input(char* label, char* buf, uint8_t flag, int len, int x, int y);
item_t* create_selectable(char* label, void* action, int arg, int x, int y);
void draw_item(item_t *it);
void select_item(item_t *it, boolean_t sel);
int handle_menu(menu_t *m);
void free_menu(menu_t *m);
int draw_menu(menu_t *m);
void render_menu(menu_t *m);
void free_font();


#endif