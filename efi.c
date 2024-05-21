#include "efi.h"

// GLOBALS
uint32_t *lfb; // display buffer pointer
uint32_t width, height, pitch; // display resolution
ssfn_font_t *font; // font struct
efi_gop_t *gop = NULL; // pointer to graphic output protocol funcs

#define BG_COLOR 0x1C2951 // "Space cadet" background color
#define HEADER_COLOR 0x0C1940 // slightly darker bg col
#define INPUT_BG_COLOR 0x0C1940// unselected input col
#define INPUT_ACTIVE_COLOR 0x112549 // selected input col
#define SELECT_COLOR 0x293960 // sligtly lighter select col

/*
    Loads SSFN font to struct font from file with specified path
*/
int load_font(char* path) {
    FILE *f;
    long int size;
    if((f = fopen(path, "r"))) {
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
        font = (ssfn_font_t*)malloc(size + 1);
        if(!font) {
            fprintf(stderr, "unable to allocate memory\n");
            return 0;
        }
        fread(font, size, 1, f);
        fclose(f);
        return 1;
    } else {
        fprintf(stderr, "Unable to load font\n");
        return 0;
    }
}

int locate_gop(efi_gop_t **pgop) {
    efi_guid_t gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_status_t status;
    status = BS->LocateProtocol(&gopGuid, NULL, (void**)pgop);
    if(!EFI_ERROR(status) && *pgop) {
        return 1;
    } else {
        fprintf(stderr, "unable to get graphics output protocol\n");
        return 0;
    }
}

int set_video_mode() {
    efi_status_t status;
    if (locate_gop(&gop)) {
        status = gop->SetMode(gop, 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
        if(EFI_ERROR(status)) {
            fprintf(stderr, "unable to set video mode\n");
            return 0;
        }
        // set up destination buffer
        lfb = (uint32_t*)gop->Mode->FrameBufferBase;
        width = gop->Mode->Information->HorizontalResolution;
        height = gop->Mode->Information->VerticalResolution;
        pitch = sizeof(unsigned int) * gop->Mode->Information->PixelsPerScanLine;
        return 1;
    }
    return 0;
}


void printString(int x, int y, const char *s)
{
    unsigned char *ptr, *chr, *frg;
    unsigned int c;
    uintptr_t o, p;
    int i, j, k, l, m, n;
    int bx = x;
    while (*s) {
        c = 0; 
        s += mbtowc((wchar_t*)&c, (const char*)s, 4);
        if (c == '\r') { 
            x = bx; 
            continue; 
        } else
            if(c == '\n') { 
                x = bx; 
                y += font->height; 
                continue; 
            }
        for(ptr = (unsigned char*)font + font->characters_offs, chr = 0, i = 0; i < 0x110000; i++) {
            if (ptr[0] == 0xFF) { 
                i += 65535; 
                ptr++; 
            } else if((ptr[0] & 0xC0) == 0xC0) { 
                j = (((ptr[0] & 0x3F) << 8) | ptr[1]);
                i += j;
                ptr += 2;
            } else if((ptr[0] & 0xC0) == 0x80) { 
                j = (ptr[0] & 0x3F);
                i += j;
                ptr++;
            } else { 
                if((unsigned int)i == c) { 
                    chr = ptr; 
                    break; 
                } 
                ptr += 6 + ptr[1] * (ptr[0] & 0x40 ? 6 : 5); 
            }
        }
        if(!chr) continue;
        ptr = chr + 6;
        uint32_t buf[8*16] = {0};
        gop->Blt(gop, buf, EfiBltVideoToBltBuffer, x, y, 0, 0, 8, 16, 0);
        o = (uintptr_t)buf;
        int pitch = sizeof(unsigned int)*8;
        //fprintf(stderr, "%d %d\n", font->width, font->height);
        for(i = n = 0; i < chr[1]; i++, ptr += chr[0] & 0x40 ? 6 : 5) {
            if(ptr[0] == 255 && ptr[1] == 255) continue;
            frg = (unsigned char*)font + (chr[0] & 0x40 ? ((ptr[5] << 24) | (ptr[4] << 16) | (ptr[3] << 8) | ptr[2]) :
                ((ptr[4] << 16) | (ptr[3] << 8) | ptr[2]));
            if((frg[0] & 0xE0) != 0x80) continue;
            o += (int)(ptr[1] - n) * pitch; 
            n = ptr[1];
            k = ((frg[0] & 0x1F) + 1) << 3; 
            j = frg[1] + 1; 
            frg += 2;
            for(m = 1; j; j--, n++, o += pitch)
                for(p = o, l = 0; l < k; l++, p += 4, m <<= 1) {
                    if(m > 0x80) { frg++; m = 1; }
                    if(*frg & m) *((unsigned int*)(p)) = 0xFFFFFF;
                }
        }
        gop->Blt(gop, buf, EfiBltBufferToVideo, 0, 0, x, y, 8, 16, 0);
        x += chr[4]+1; y += chr[5];
    }
}

//Returns number of symbols in multibyte string
int mbstrlen(char *str) {
    int r = 0;
    while (*str) {
        str += mbtowc(0, (const char*)str, 4);
        r++;
    }
    return r;
}

void print_centered(int y, char *str) {
    int offset = (width - mbstrlen(str)*9)/2;
    printString(offset, y, str);
}

// FONT - 14 height, 8 width, 1 spacing per symbol!!

int draw_box(uintn_t sx, uintn_t sy, uintn_t w, uintn_t h, uint32_t col) {
    //uint32_t c = col;
    gop->Blt(gop, &col, EfiBltVideoFill, 0, 0, sx, sy, w, h, 0);
    return 0;
}

void draw_base_screen() {
    uint32_t px = BG_COLOR;
    gop->Blt(gop, &px, EfiBltVideoFill, 0, 0, 0, 0, width, height, 0);
    px = HEADER_COLOR;  // space cadet bg col
    draw_box(0, 0, width, 34, HEADER_COLOR);
    gop->Blt(gop, &px, EfiBltVideoFill, 0, 0, 0, 0, width, 34, 0);
    char name[] = "Pretty Poor Privacy SDZ v.0.2";
    int len = mbstrlen(name)*9;
    int sx = (width-len)/2;
    int sy = 10;
    printString(sx, sy, name);
}

efi_input_key_t get_key() {
    efi_event_t events[1];
    efi_input_key_t key;
    key.ScanCode = 0;
    key.UnicodeChar = u'\0';
    events[0] = cin->WaitForKey;
    uintn_t index = 0;
    BS->WaitForEvent(1, events, &index);
    if (index == 0) cin->ReadKeyStroke(cin, &key);
    return key;
}

enum {
    ITEM_SELECTABLE,
    ITEM_INPUT
} ItemTypes;

typedef struct {
    int     type;
    boolean_t is_selected;
    int x;
    int y;
    void*   item;
} item_t;

typedef struct {
    item_t **items;
    int len; //length of items in menu
    int cur; //active selection
} menu_t;

//if is active use input handler?
typedef struct {
    char*       label;
    char*       buf;
    boolean_t   is_password;
    boolean_t   is_active;
    int         len; // of buffer
} input_t;

typedef struct {
    char*   label;
    void*   action;
} select_t;

void print_hidden(int x, int y, int len) {
    char* str = calloc(len+1, sizeof(char));
    for (int i = 0; i < len; i++)
        str[i] = '*';
    printString(x, y, str);
    free(str);
}

int draw_input(item_t *inp) {
    input_t *it = inp->item;
    draw_box(inp->x, inp->y, 4 + 9*it->len, 20, 
        inp->is_selected ? (it->is_active ? INPUT_ACTIVE_COLOR : SELECT_COLOR) : INPUT_BG_COLOR);
    int llen = mbstrlen(it->label);
    printString(inp->x-9*llen-4, inp->y+2, it->label);
    if (it->is_password) {
        print_hidden(inp->x+2, inp->y+2, strlen(it->buf));
    } else {
        printString(inp->x+2, inp->y+2, it->buf);
    }
    return 0;
}

void draw_selectable(item_t *sel) {
    select_t *it = sel->item;
    draw_box(sel->x, sel->y, 4 + 9*mbstrlen(it->label), 20, sel->is_selected ? SELECT_COLOR : BG_COLOR);
    printString(sel->x+2, sel->y+2, it->label);
}

void process_input(item_t *it) {
    input_t *inp = it->item;
    inp->is_active = 1;
    draw_input(it);
    
    while (1) {
        char c = getchar();
        if (c == 0x08) {
            inp->buf[strlen(inp->buf)-1] = 0;
            draw_input(it);
            continue;
        } else if (c == 0x0D) {
            break; 
        } else if (strlen(inp->buf) >= inp->len)
            continue;
        strncat(inp->buf, &c, 1);
        draw_input(it);
    }
    inp->is_active = 0;
    draw_input(it);
}

menu_t create_menu(item_t **items, int len) {
    return (menu_t){items, len, 0};
}

item_t create_input(char* label, char* buf, boolean_t is_password, int len, int x, int y) {
    input_t *input = malloc(sizeof(input_t));
    *input = (input_t){label, buf, is_password, 0, len};
    return (item_t){ITEM_INPUT, 0, x, y, input};
}

item_t create_selectable(char* label, void* action, int x, int y) {
    select_t *select = malloc(sizeof(select_t));
    *select = (select_t){label, action};
    return (item_t){ITEM_SELECTABLE, 0, x, y, select};
}

void free_menu(menu_t *m) {
    for (int i = 0; i < m->len; i++)
        free(m->items[i]->item);
}

void draw_item(item_t *it) {
    switch (it->type) {
    case ITEM_INPUT:
        draw_input(it);
        break;
    case ITEM_SELECTABLE:
        draw_selectable(it);
        break;
    }
}

void select_item(item_t *it, boolean_t sel) {
    it->is_selected = sel;
    draw_item(it);
}

void handle_menu(menu_t *m) {
    select_item(m->items[0], 1);
    while (1) {
        //char c = getchar();
        efi_input_key_t c = get_key();
        if (c.ScanCode == SCAN_DOWN) {
            select_item(m->items[m->cur], 0);
            draw_item(m->items[m->cur]);
            m->cur = (m->cur+1) % m->len;
            select_item(m->items[m->cur], 1);
            draw_item(m->items[m->cur]);
        } else if (c.ScanCode == SCAN_UP) {
            select_item(m->items[m->cur], 0);
            draw_item(m->items[m->cur]);
            m->cur = (m->cur - 1) < 0 ? m->len - 1 : m->cur - 1;
            select_item(m->items[m->cur], 1);
            draw_item(m->items[m->cur]);
        } else if (c.UnicodeChar == 0x0D) {
            switch (m->items[m->cur]->type)
            {
            case ITEM_INPUT:
                process_input(m->items[m->cur]);
                break;
            case ITEM_SELECTABLE:
                select_t *s = (select_t *)m->items[m->cur]->item;
                void (*fun_ptr)() = (void *)s->action;
                (*fun_ptr)();
                break;
            }
        }
    }
    free_menu(m);
}

void draw_menu(menu_t *m) {
    for (int i = 0; i < m->len; i++) {malloc(sizeof(item_t)*2);
        item_t *item = m->items[i];
        switch (item->type) {
        case ITEM_INPUT:
            draw_input(item);
            break;
        case ITEM_SELECTABLE:
            draw_selectable(item);
            break;
        }
    }
    handle_menu(m);
}

void create_account_test() {
    print_centered(210, "Аккаунт создан!");
}

int main_menu() {
    print_centered(70, "Первый запуск системы, введите логин и пароль для аккаунта администратора");
    char login[16] = {0};
    char password[16] = {0};
    item_t log_form = create_input("Логин:", login, 0, 16, width/2, 120);
    item_t pass_form = create_input("Пароль:", password, 1, 16, width/2, 150);
    item_t submit = create_selectable("Создать аккаунт", &create_account_test, width/2, 180);
    item_t *items[] = {&log_form, &pass_form, &submit};
    menu_t menu = create_menu(items, 3);
    draw_menu(&menu);
    return 0;
}

int main() {
    // Disable Watchdog Timer
    BS->SetWatchdogTimer(0, 0x10000, 0, NULL);
    // Clear screen
    cout->ClearScreen(cout);
    load_font("font.sfn");
    set_video_mode();
    draw_base_screen();
    main_menu();
    getchar();
    return 0;
}