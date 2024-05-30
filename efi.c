#include "efi.h"

// GLOBALS

uint8_t max_tries = DEFAULT_TRIES;
uint8_t timeout = DEFAULT_TIMEOUT;
display_t display = {0};

char login[MAX_LOGIN+1] = {0}; //login buf
char password[MAX_PASS+1] = {0}; //password
char current_login[MAX_LOGIN+1] = {0}; // 0 on unauth
char role = ROLE_UNAUTHORIZED;
static uint8_t attempts = 0; // current amount of login attempts
static uint32_t lock_till = 0; // current time till unlock


/*
    Loads SSFN font to struct font from file with specified path
*/


/* 
    Perform print_string for str at specified y absolute screen coordinate,
    centered by x coordinate. Works with multibyte strings
*/
void print_centered(int y, char *str) {
    int offset = (display.width - mbstrlen(str)*9)/2;
    print_string(offset, y, str);
}

void draw_background() {
    uint32_t px = BG_COLOR;
    draw_box(0, 34, display.width, display.height-68, BG_COLOR);
}

void draw_base_screen() {
    draw_background();
    draw_box(0, 0, display.width, 34, HEADER_COLOR);
    draw_box(0, display.height-34, display.width, 34, HEADER_COLOR);
    print_centered(10, HEADER_STRING);
    print_string(10, display.height - 25, "Подсказка по управлению: Навигация - стрелки вверх/вниз, Enter - выбор пункта меню");
}

enum {
    MENU_BOOT = 1,
    MENU_AUTH,
    MENU_FIRST,
    MENU_LOGGED,
    MENU_REG,
    MENU_LOGS,
    MENU_SETTINGS,
} MenuTypes;


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

int create_account(int role, int retval) {
    draw_box(0, 240, display.width, 20, BG_COLOR);
    if (!strlen(login)) {
        print_centered(240, "Введите логин!");
        return 0;
    } if (!strlen(password)) {
        print_centered(240, "Введите пароль!");
        return 0;
    } if (does_user_exist(login)) {
        print_centered(240, "Данное имя пользователя уже занято!");
        return 0;
    }
    register_account(role, login, password);
    //make acc (check if exist?)
    print_centered(240, "Аккаунт создан!");
    write_log(login, role, ACTION_REGISTER);
    //switch menu
    return retval; //should switch menu to auth one with return
}

int create_admin_account() {
    return create_account(ROLE_ADMIN, MENU_AUTH);
}

int auth_process() {
    draw_box(0, 210, display.width, 20, BG_COLOR);
    if (!strlen(login)) {
        print_centered(210, "Введите логин!");
        return 0;
    } if (!strlen(password)) {
        print_centered(210, "Введите пароль!");
        return 0;
    }
    if (time(0) < lock_till) {
        char s[100] = {0};
        sprintf(s, "Попытки авторизации заблокированы на %d секунд(ы)", lock_till - time(0));
        print_centered(210, s);
        return 0;
    }
    attempts++;
    int r = auth(login, password);
    if (r) {
        role = r;
        strncpy(current_login, login, MAX_LOGIN);
        write_log(current_login, role, ACTION_LOGIN);
        attempts = 0;
        return MENU_LOGGED;
    }
    if (attempts >= max_tries) {
        char s[170] = {0};
        sprintf(s, "Превышено максимальное количество попыток авторизации! Вход заблокирован на %d секунд", timeout*60);
        print_centered(210, s);
        write_log(current_login, role, ACTION_LOCK);
        attempts = 0;
        lock_till = time(0) + timeout*60;
        update_lock(lock_till);
        return 0;
    }
    char s[100] = {0};
    sprintf(s, "Неверные данные для входа, осталось попыток: %d", max_tries - attempts);
    write_log(login, role, ACTION_LOGIN_ATTEMPT);
    print_centered(210, s);
    return 0;
}

int reg_process() {
    return create_account(ROLE_USER, 0);
}

int boot_os() {
    return MENU_BOOT;
}

int logout() {
    write_log(current_login, role, ACTION_LOGOUT);
    memset(current_login, 0, MAX_LOGIN);
    role = ROLE_UNAUTHORIZED;
    return MENU_AUTH;
}

int shutdown() {
    write_log(current_login, role, ACTION_SHUTDOWN);
    RT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    return 0;
}

int back_action() {
    return MENU_LOGGED;
}

int auth_menu_templ(item_t* submit, char* main_text, boolean_t backable) {
    /* template for various auth menus */
    item_t log_form = create_input("Логин:", login, INPUT_DEFAULT, MAX_LOGIN, display.width/2, 120);
    item_t pass_form = create_input("Пароль:", password, INPUT_PASSWORD, MAX_PASS, display.width/2, 150);
    item_t back_sel = create_selectable("Назад", &back_action, display.width/2, 210);
    menu_t menu;
    if (backable) {
        item_t *items[] = {&log_form, &pass_form, submit, &back_sel};
        menu = create_menu(items, 4);
    } else {
        item_t *items[] = {&log_form, &pass_form, submit};
        menu = create_menu(items, 3);
    }
    draw_background();
    print_centered(70, main_text);
    int r = draw_menu(&menu);
    reset_forms();
    return r;
}

int first_menu() {
    item_t submit = create_selectable("Создать аккаунт", &create_admin_account, display.width/2, 180);
    return auth_menu_templ(&submit, "Первый запуск системы, введите логин и пароль для аккаунта администратора", FALSE);
}

int auth_menu() {
    item_t submit = create_selectable("Войти", &auth_process, display.width/2, 180);
    return auth_menu_templ(&submit, "Введите ваш логин и пароль для продолжения", FALSE);
}

int reg_menu() {
    item_t submit = create_selectable("Зарегистрировать", &reg_process, display.width/2, 180);
    return auth_menu_templ(&submit, "Введите логин и пароль для регистрации аккаунта пользователя", TRUE);
}

int logs_menu() {
    item_t b = create_selectable("Вернуться в меню", &back_action, 20, 120);
    item_t *items[] = {&b};
    menu_t menu = create_menu(items, 1);
    draw_background();
    log_text_entry_t *o = calloc(20, sizeof(log_text_entry_t));
    log_text_entry_t *s = o;
    //s += snprintf(s, 100, "%015s%16s%5s%50s\n", "Время", "Логин", "Роль", "Событие");
    print_string(20, 150, "Время");
    print_string(220, 150, "Логин");
    print_string(370, 150, "Роль");
    print_string(450, 150, "Событие");
    int amount = get_log_entries(20, 0, s);
    int y = 180;
    for (int i = 0; i < amount; i++, y+=25) {
        print_string(20, y, s->ts);
        print_string(220, y, s->login);
        print_string(370, y, s->role);
        print_string(450, y, s->action);
        s += sizeof(log_text_entry_t);
    }
    free(o);
    print_centered(70, "Последние действия в журнале:");
    return draw_menu(&menu);
}

char tries_inp[3] = {0};
char timeout_inp[3] = {0};

int save_settings() {
    draw_box(0, 240, display.width, 20, BG_COLOR);
    uint8_t tr = atoi(tries_inp);
    if (tr < 1 || tr > 10) {
        print_centered(240, "Число попыток должно быть от 1 до 10!");
        return 0;
    }
    uint8_t to = atoi(timeout_inp);
    if (to < 1) {
        print_centered(240, "Время блокировки должно быть от 1 до 99!");
        return 0;
    }
    max_tries = tr;
    timeout = to;
    save_cfg(max_tries, timeout);
    print_centered(240, "Настройки сохранены!");
    return 0;
}

int settings_menu() {
    item_t tr = create_input("Количество попыток для ввода пароля", tries_inp, INPUT_DEFAULT, 2, display.width/2, 120);
    item_t to = create_input("Время блокировки при превышении кол-ва попыток (минут)", timeout_inp, INPUT_DEFAULT, 2, display.width/2, 150);
    item_t subm = create_selectable("Сохранить", &save_settings, display.width/2, 180);
    item_t bck = create_selectable("Назад", &back_action, display.width/2, 210);
    item_t *items[] = {&tr, &to, &subm, &bck};
    menu_t menu = create_menu(items, 4);
    draw_background();
    print_centered(70, "Настройки");
    return draw_menu(&menu);
}

int open_log() {
    return MENU_LOGS;
}

int open_reg() {
    return MENU_REG;
}

int open_settings() {
    return MENU_SETTINGS;
}

int user_menu() {
    uintn_t w = display.width/3;
    menu_t menu;
    item_t b = create_selectable("Загрузить систему", &boot_os, w, 120);
    item_t l = create_selectable("Выйти из системы", &logout, w, 150);
    item_t s = create_selectable("Выключить компьютер", &shutdown, w, 180);
    item_t r = create_selectable("Зарегистрировать пользователя", &open_reg, w, 210);
    item_t lg = create_selectable("Просмотр журнала событий", &open_log, w, 240);
    item_t st = create_selectable("Настройки", &open_settings, w, 270);
    if (role == ROLE_ADMIN) {
        item_t *items[] = {&b, &l, &s, &r, &lg, &st};
        menu = create_menu(items, 6);
        // add confgiure
        // add reg user/delete user, change password?
    } else {
        item_t *items[] = {&b, &l, &s};
        menu = create_menu(items, 3);
    }
    draw_background();
    char authed[120] = {0};
    sprintf(authed, "Вы авторизованы как %s, выберите следующее действие", current_login);
    print_centered(70, authed);
    return draw_menu(&menu);
}

//initialisation sequence
void init() {
    // Disable Watchdog Timer
    BS->SetWatchdogTimer(0, 0x10000, 0, NULL);
    // Clear screen
    cout->ClearScreen(cout);
    load_font("font.sfn");
    set_video_mode(&display);
    draw_base_screen();
    load_settings(&max_tries, &timeout, &lock_till);
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
    case MENU_LOGS:
        return logs_menu();
    case MENU_SETTINGS:
        return settings_menu();
    default:
        return MENU_BOOT;
    }
}

int main(int argc, char **argv) {
    init();
    
    int m = MENU_FIRST;
    if (check_acc_exist()) m = MENU_AUTH;
    while (m != MENU_BOOT) {
        
        m = call_menu(m);
    }
    write_log(current_login, role, ACTION_BOOT);
    free_font();
    return 0;
}