#include "gui.h"

efi_gop_t *gop = NULL; // pointer to graphic output protocol funcs
ssfn_font_t *font; // font struct

/* 
    Locates Graphical Output Protocol by its GUID and writes its handle to pgop
    Returns TRUE on success, FALSE on failure
*/
boolean_t locate_gop(efi_gop_t **pgop) {
    efi_guid_t gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_status_t status;
    status = BS->LocateProtocol(&gopGuid, NULL, (void**)pgop);
    if(!EFI_ERROR(status) && *pgop) {
        return TRUE;
    } else {
        fprintf(stderr, "unable to get graphics output protocol\n");
        return FALSE;
    }
}

/* 
    Initialises GOP video mode and sets it to default
    Returns result as boolean 
*/
boolean_t set_video_mode(display_t *d) {
    efi_gop_t **ppgop = &gop;
    efi_status_t status;
    if (locate_gop(ppgop)) {
        efi_gop_t *pgop = *ppgop;
        status = pgop->SetMode(pgop, 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
        if(EFI_ERROR(status)) {
            fprintf(stderr, "unable to set video mode\n");
            return FALSE;
        }
        // set up destination buffer
        d->lfb = (uint32_t*)pgop->Mode->FrameBufferBase;
        d->width = pgop->Mode->Information->HorizontalResolution;
        d->height = pgop->Mode->Information->VerticalResolution;
        d->pitch = sizeof(unsigned int) * pgop->Mode->Information->PixelsPerScanLine;
        return TRUE;
    }
    return FALSE;
}

/* 
    Loads font to global struct for GUI
    Returns result as boolean
*/
boolean_t load_font(char* path) {
    FILE *f;
    long int size;
    if((f = fopen(path, "r"))) {
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
        font = (ssfn_font_t*)malloc(size + 1);
        if(!font) {
            fprintf(stderr, "unable to allocate memory\n");
            return FALSE;
        }
        fread(font, size, 1, f);
        fclose(f);
        return TRUE;
    } else {
        fprintf(stderr, "Unable to load font\n");
        return FALSE;
    }
}

/* 
    Prints string s with pre-loaded SSFN font starting from
    specified x and y absolute screen coordinates.
    Requires set_video_mode and loaded font
*/
void print_string(int x, int y, const char *s)
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
        uint8_t pitch = sizeof(unsigned int)*8;
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
                    if(*frg & m) *((unsigned int*)(p)) = FONT_COLOR;
                }
        }
        gop->Blt(gop, buf, EfiBltBufferToVideo, 0, 0, x, y, 8, 16, 0);
        x += chr[4]+1; y += chr[5];
    }
}

/* 
    Draws box starting from sx and sy absolute screen coordinates
    With width w and height h of col RGBA formatted color
*/
void draw_box(uintn_t sx, uintn_t sy, uintn_t w, uintn_t h, uint32_t col) {
    uint32_t c = col;
    gop->Blt(gop, &c, EfiBltVideoFill, 0, 0, sx, sy, w, h, 0);
}

/* 
    Performs print_string at x and y coordinates of len length
    Filled with "*" symbols. Used at password inputs.
*/
void print_hidden(int x, int y, int len) {
    char* str = calloc(len+1, sizeof(char));
    for (int i = 0; i < len; i++)
        str[i] = '*';
    print_string(x, y, str);
    free(str);
}

/*
    Draws input field on screen for item inp.
    Should be used for items of type ITEM_INPUT 
*/
void draw_input(item_t *inp) {
    input_t *it = inp->item;
    draw_box(inp->x, inp->y, 4 + 9*it->len, 20, 
        inp->is_selected ? (it->is_active ? INPUT_ACTIVE_COLOR : SELECT_COLOR) : INPUT_BG_COLOR);
    int llen = mbstrlen(it->label);
    print_string(inp->x-9*llen-4, inp->y+2, it->label);
    if (it->flag == INPUT_PASSWORD) {
        print_hidden(inp->x+2, inp->y+2, strlen(it->buf));
    } else {
        print_string(inp->x+2, inp->y+2, it->buf);
    }
}

/*
    Draws selectable on screen for item sel.
    Should be used for items of type ITEM_SELECTABLE 
*/
void draw_selectable(item_t *sel) {
    select_t *it = sel->item;
    draw_box(sel->x, sel->y, 4 + 9*mbstrlen(it->label), 20, sel->is_selected ? SELECT_COLOR : BG_COLOR);
    print_string(sel->x+2, sel->y+2, it->label);
}

/*
    Input handler for active input fields
*/
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
        if (inp->flag == INPUT_DIGITS && (c < '0' || c > '9'))
            continue;
        if (c < '!' || c > '~')
            continue;
        strncat(inp->buf, &c, 1);
        draw_input(it);
    }
    inp->is_active = 0;
    draw_input(it);
}

/* Returns menu struct from array of items of specified length len */
menu_t create_menu(item_t **items, int len) {
    return (menu_t){items, len, 0};
}

/* Returns item struct of ITEM_INPUT type with specified label, buffer buf,
    which stores input after confirmation, if is_password = true output
    would be hidden from user with "*", len - max length of input 
    x and y defines starting coordinates of input field */
item_t* create_input(char* label, char* buf, uint8_t flag, int len, int x, int y) {
    input_t *input = malloc(sizeof(input_t));
    *input = (input_t){label, buf, flag, 0, len};
    item_t *item = malloc(sizeof(item_t));
    *item = (item_t){ITEM_INPUT, 0, x, y, input};
    return item;
}

/* Returns item struct of ITEM_SELECTABLE type with specified label. 
    action - pointer to function to call on activate selectable
    x and y defines starting coordinates of label string */
item_t* create_selectable(char* label, void* action, int arg, int x, int y) {
    select_t *select = malloc(sizeof(select_t));
    *select = (select_t){label, action, arg};
    item_t *item = malloc(sizeof(item_t));
    *item = (item_t){ITEM_SELECTABLE, 0, x, y, select};
    return item;
}

/* Unallocates every item in menu */
void free_menu(menu_t *m) {
    for (int i = 0; i < m->len; i++) {
        free(m->items[i]->item);
        free(m->items[i]);
    }
}

/* Draws item it on screen */
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

/* Marks item it as selected if sel = TRUE and vice versa */
void select_item(item_t *it, boolean_t sel) {
    it->is_selected = sel;
    draw_item(it);
}

/* 
    menu inputs handler
    returned number designed to be as next menu number or some action (e.g. boot)
*/
int handle_menu(menu_t *m) {
    select_item(m->items[0], 1);
    while (1) {
        efi_input_key_t c = get_key();
        if (c.ScanCode == SCAN_DOWN || c.ScanCode == SCAN_UP) {
            select_item(m->items[m->cur], 0);
            draw_item(m->items[m->cur]);
            m->cur = c.ScanCode == SCAN_DOWN ? (m->cur+1) % m->len : (m->cur - 1) < 0 ? m->len - 1 : m->cur - 1;
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
                int (*fun_ptr)(int) = (void *)s->action;
                int r = (*fun_ptr)(s->arg);
                if (r) return r;
                break;
            }
        }
    }
}

/* Renders menu elements */
void render_menu(menu_t *m) {
    for (int i = 0; i < m->len; i++) {
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
}

/* Process menu */
int draw_menu(menu_t *m) {
    render_menu(m);
    int r = handle_menu(m);
    free_menu(m);
    return r;
}

/* Frees allocated global GUI font */
void free_font() {
    free(font);
}