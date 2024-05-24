#include "efi.h"

// GLOBALS
uint32_t *lfb; // display buffer pointer
uint32_t width, height, pitch; // display resolution
ssfn_font_t *font; // font struct
efi_gop_t *gop = NULL; // pointer to graphic output protocol funcs
uint8_t tries = 0;

enum {
    ROLE_UNAUTHORIZED,
    ROLE_USER,
    ROLE_ADMIN
} Roles;

#define MAX_LOGIN 16
#define MAX_PASS 16
char login[MAX_LOGIN+1] = {0}; //login buf
char password[MAX_PASS+1] = {0}; //password
char current_login[MAX_LOGIN+1] = {0}; // 0 on unauth
char role = ROLE_UNAUTHORIZED;


#define HEADER_COLOR 0x081030 // "Space cadet" background color
#define BG_COLOR 0x0C1940 // slightly darker bg col
#define INPUT_BG_COLOR 0x091133// unselected input field col
#define INPUT_ACTIVE_COLOR 0x354369 // active input col
#define SELECT_COLOR 0x293960 // sligtly lighter select col
#define FONT_COLOR 0xFFFFFF // color of font
#define HEADER_HEIGHT 34 // header and footer height in px
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
                    if(*frg & m) *((unsigned int*)(p)) = FONT_COLOR;
                }
        }
        gop->Blt(gop, buf, EfiBltBufferToVideo, 0, 0, x, y, 8, 16, 0);
        x += chr[4]+1; y += chr[5];
    }
}

void print_centered(int y, char *str) {
    int offset = (width - mbstrlen(str)*9)/2;
    printString(offset, y, str);
}

// FONT - 14 height, 8 width, 1 spacing per symbol!!

int draw_box(uintn_t sx, uintn_t sy, uintn_t w, uintn_t h, uint32_t col) {
    uint32_t c = col;
    gop->Blt(gop, &c, EfiBltVideoFill, 0, 0, sx, sy, w, h, 0);
    return 0;
}

void draw_background() {
    uint32_t px = BG_COLOR;
    draw_box(0, 34, width, height-68, BG_COLOR);
}

void draw_base_screen() {
    draw_background();
    draw_box(0, 0, width, 34, HEADER_COLOR);
    draw_box(0, height-34, width, 34, HEADER_COLOR);
    print_centered(10, "Pretty Poor Privacy SDZ v.0.3");
    printString(10, height - 25, "Подсказка по управлению:");
}

enum {
    ITEM_SELECTABLE,
    ITEM_INPUT
} ItemTypes;

enum {
    INPUT_DEFAULT,
    INPUT_PASSWORD
} InputTypes;

enum {
    MENU_BOOT = 1,
    MENU_AUTH,
    MENU_FIRST,
    MENU_LOGGED,
    MENU_LOCK,
    MENU_REG,
    MENU_LOGS,
    MENU_SETTINGS,
} MenuTypes;

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
                int (*fun_ptr)() = (void *)s->action;
                int r = (*fun_ptr)();
                if (r) return r;
                break;
            }
        }
    }
}

void reset_forms() {
    memset(login, 0, MAX_LOGIN);
    memset(password, 0, MAX_PASS);
}

int draw_menu(menu_t *m) {
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
    int r = handle_menu(m);
    free_menu(m);
    return r;
}

int create_admin_account() {
    draw_box(0, 210, width, 20, BG_COLOR);
    if (!strlen(login)) {
        print_centered(210, "Введите логин!");
        return 0;
    } if (!strlen(password)) {
        print_centered(210, "Введите пароль!");
        return 0;
    }
    //make acc (check if exist?)
    print_centered(210, "Аккаунт создан!");
    //switch menu
    return MENU_AUTH; //should switch menu to auth one with return
}

int auth_process() {
    draw_box(0, 210, width, 20, BG_COLOR);
    if (!strlen(login)) {
        print_centered(210, "Введите логин!");
        return 0;
    } if (!strlen(password)) {
        print_centered(210, "Введите пароль!");
        return 0;
    }
    //make acc (check if exist?)
    print_centered(210, "Успех!");
    //switch menu
    return MENU_LOGGED; //should switch menu to auth one with return
}

int boot_os() {
    return MENU_BOOT;
}

int logout() {
    //reset cuurent acc smh
    return MENU_AUTH;
}

int shutdown() {
    RT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    return 0;
}

int auth_menu_templ(item_t* submit, char* main_text) {
    /* template for various auth menus */
    item_t log_form = create_input("Логин:", login, INPUT_DEFAULT, MAX_LOGIN, width/2, 120);
    item_t pass_form = create_input("Пароль:", password, INPUT_PASSWORD, MAX_PASS, width/2, 150);
    item_t *items[] = {&log_form, &pass_form, submit};
    menu_t menu = create_menu(items, 3);
    draw_base_screen();
    print_centered(70, main_text);
    int r = draw_menu(&menu);
    reset_forms();
    return r;
}

int first_menu() {
    item_t submit = create_selectable("Создать аккаунт", &create_admin_account, width/2, 180);
    return auth_menu_templ(&submit, "Первый запуск системы, введите логин и пароль для аккаунта администратора");
}

int auth_menu() {
    item_t submit = create_selectable("Войти", &auth_process, width/2, 180);
    return auth_menu_templ(&submit, "Введите ваш логин и пароль для продолжения");
}

int reg_menu() {
    item_t submit = create_selectable("Зарегистрировать", &auth_process, width/2, 180);
    return auth_menu_templ(&submit, "Введите логин и пароль для регистрации аккаунта пользователя");
}

int test_hash() {
    uint8_t h[33] = {0};
    char s[65] = {0};
    fprintf(stderr, "1");
    sha256_hash("TEST", h);
    fprintf(stderr, "2");
    write_hash(h, s);
    fprintf(stderr, "3");
    print_centered(240, s);
    return 0;
}

int user_menu() {
    uintn_t w = width/3;
    item_t b = create_selectable("Загрузить систему", &boot_os, w, 120);
    item_t l = create_selectable("Выйти из системы", &logout, w, 150);
    item_t s = create_selectable("Выключить компьютер", &shutdown, w, 180);
    item_t t = create_selectable("Test hash", &test_hash, w, 210);
    item_t *items[] = {&b, &l, &s, &t};
    menu_t menu = create_menu(items, 4);
    draw_base_screen();
    print_centered(70, "Вы авторизованы как xxx выберите следующее действие");
    return draw_menu(&menu);
}

int admin_menu() {
    return 0;
}

//initialisation sequence
void init() {
    // Disable Watchdog Timer
    BS->SetWatchdogTimer(0, 0x10000, 0, NULL);
    // Clear screen
    cout->ClearScreen(cout);
    load_font("font.sfn");
    set_video_mode();
}

int call_menu(int m) {
    switch (m) {
    case MENU_FIRST:
        return first_menu();
    case MENU_AUTH:
        return auth_menu();
    case MENU_REG:
        return reg_menu();
    case MENU_LOGGED:
        return user_menu();    
    default:
        return MENU_BOOT;
    }
}

int main() {
    init();
    int m = MENU_FIRST;
    while (m != MENU_BOOT) {
        m = call_menu(m);
    }
    free(font);
    return 0;
}