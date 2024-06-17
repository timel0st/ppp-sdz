#include "efi.h"

// GLOBALS
cfg_t cfg;
display_t display = {0};

char login[MAX_LOGIN+1] = {0}; //login buf
char password[MAX_PASS+1] = {0}; //password
char current_login[MAX_LOGIN+1] = {0}; // 0 on unauth
char role = ROLE_UNAUTHORIZED;
static uint32_t log_cursor = 0; // current pos for log pages

/* 
    Perform print_string for str at specified y absolute screen coordinate,
    centered by x coordinate. Works with multibyte strings
*/
void print_centered(int y, char *str) {
    int offset = (display.width - mbstrlen(str)*9)/2;
    print_string(offset, y, str);
}

/* Fills background workspace with BG_COLOR */
void draw_background() {
    uint32_t px = BG_COLOR;
    draw_box(0, 34, display.width, display.height-68, BG_COLOR);
}

/* Renders header rect with header string */
void draw_header(char* header) {
    draw_box(0, 0, display.width, 34, HEADER_COLOR);
    print_centered(10, header);
}

/* Renders footer rect with footer string */
void draw_footer(char* footer) {
    draw_box(0, display.height-34, display.width, 34, HEADER_COLOR);
    print_string(10, display.height - 25, footer);
}

/* Renders base screen for every menu */
void draw_base_screen() {
    draw_background();
    draw_header(HEADER_STRING);
    draw_footer("Подсказка: Навигация - стрелки вверх/вниз, Enter - выбор пункта меню");
}

enum {
    MENU_SAME,
    MENU_BOOT,
    MENU_AUTH,
    MENU_FIRST,
    MENU_LOGGED,
    MENU_REG,
    MENU_LOGS,
    MENU_SETTINGS,
    MENU_MANAGE,
} MenuTypes;

/* Sets login and password forms to zero */
void reset_forms() {
    memset(login, 0, MAX_LOGIN);
    memset(password, 0, MAX_PASS);
}

/* template menu for account creation */
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

/* calls for templatw */
int create_admin_account(int menu) {
    return create_account(ROLE_ADMIN, menu);
}

int auth_process() {
    static uint8_t attempts = 0; // current amount of login attempts
    draw_box(0, 210, display.width, 20, BG_COLOR);
    if (!strlen(login)) {
        print_centered(210, "Введите логин!");
        return MENU_SAME;
    } 
    if (!strlen(password)) {
        print_centered(210, "Введите пароль!");
        return MENU_SAME;
    }
    if (time(0) < cfg.lock_till) {
        char s[100] = {0};
        sprintf(s, "Попытки авторизации заблокированы на %d секунд(ы)", cfg.lock_till - time(0));
        print_centered(210, s);
        return MENU_SAME;
    }
    attempts++;
    int r = auth(login, password);
    if (r) {
        role = r;
        strncpy(current_login, login, MAX_LOGIN+1);
        write_log(current_login, role, ACTION_LOGIN);
        attempts = 0;
        return MENU_LOGGED;
    }
    if (attempts >= cfg.tries) {
        char s[170] = {0};
        sprintf(s, "Превышено максимальное количество попыток авторизации! Вход заблокирован на %d секунд", cfg.timeout*60);
        print_centered(210, s);
        write_log(current_login, role, ACTION_LOCK);
        attempts = 0;
        cfg.lock_till = time(0) + cfg.timeout*60;
        save_cfg(&cfg);
        return MENU_SAME;
    }
    char s[100] = {0};
    snprintf(s, 100, "Неверные данные для входа, осталось попыток: %d", cfg.tries - attempts);
    print_centered(210, s);
    write_log(login, role, ACTION_LOGIN_ATTEMPT);
    return MENU_SAME;
}

int reg_process() {
    return create_account(ROLE_USER, 0);
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

int open_menu(int menu_id) {
    return menu_id;
}

/* template menu for authorizaton */
int auth_menu_templ(item_t* submit, char* main_text, boolean_t backable) {
    /* template for various auth menus */
    item_t *items[] = {
        create_input("Логин:", login, INPUT_DEFAULT, MAX_LOGIN, display.width/2, 120),
        create_input("Пароль:", password, INPUT_PASSWORD, MAX_PASS, display.width/2, 150),
        submit,
        create_selectable("Назад", &open_menu, MENU_LOGGED, display.width/2, 210)
    };
    menu_t menu = create_menu(items, 3 + backable);
    draw_background();
    print_centered(70, main_text);
    int r = draw_menu(&menu);
    reset_forms();
    return r;
}

/* Menu for installation launch for admin */
int first_menu() {
    item_t *submit = create_selectable("Создать аккаунт", &create_admin_account, MENU_AUTH, display.width/2, 180);
    return auth_menu_templ(submit, "Первый запуск системы, введите логин и пароль для аккаунта администратора", FALSE);
}

/* Authorization menu */
int auth_menu() {
    item_t *submit = create_selectable("Войти", &auth_process, 0, display.width/2, 180);
    return auth_menu_templ(submit, "Введите ваш логин и пароль для продолжения", FALSE);
}

/* Registration menu*/
int reg_menu() {
    item_t *submit = create_selectable("Зарегистрировать", &reg_process, ROLE_USER, display.width/2, 180);
    return auth_menu_templ(submit, "Введите логин и пароль для регистрации аккаунта пользователя", TRUE);
}

/* Renders logs inside logs menu */
void render_logs() {
    draw_box(0, 150, display.width, display.height-34-150, BG_COLOR);
    log_text_entry_t *o = malloc(ENTRIES_PER_PAGE*sizeof(log_text_entry_t));
    int amount = get_log_entries(ENTRIES_PER_PAGE, log_cursor, o);
    int y = 210;
    for (int i = 0; i < amount; i++, y+=25) {
        print_string(20, y, o[i].ts);
        print_string(220, y, o[i].login);
        print_string(370, y, o[i].role);
        print_string(450, y, o[i].action);
    }
    char *info = calloc(100, sizeof(uint8_t));
    snprintf(info, 100, "Отображаются записи %u-%u. Всего записей: %u.",
            (uint64_t)(log_cursor+1),
            (uint64_t)(log_cursor+amount),
            (uint64_t)get_entries_num());
    print_string(20, 150, info);
    free(info);
    free(o);
}

typedef enum {
    NAV_FWD,
    NAV_REW
} nav_btn_t;

int nav_btn(int a) {
    if (a == 1 && (log_cursor + ENTRIES_PER_PAGE < get_entries_num()))
        log_cursor += ENTRIES_PER_PAGE;
    if (a == 0 && log_cursor > 0)
        log_cursor -= ENTRIES_PER_PAGE;
    render_logs();
    return MENU_SAME;
}

int logs_menu() {
    item_t *items[] = {
        create_selectable("Вернуться в меню", &open_menu, MENU_LOGGED, 430, 120), 
        create_selectable("К более старым записям", &nav_btn, NAV_REW, 20, 120), 
        create_selectable("К более новым записям", &nav_btn, NAV_FWD, 230, 120)
    };
    menu_t menu = create_menu(items, 3);
    draw_background();
    print_centered(70, "Журнал действий (сначала новые, 20 записей на странице):");
    print_string(20, 180, "Время");
    print_string(220, 180, "Логин");
    print_string(370, 180, "Роль");
    print_string(450, 180, "Событие");
    render_logs();
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
    if (to < 0) {
        print_centered(240, "Время блокировки должно быть от 0 до 99!");
        return 0;
    }
    cfg.tries = tr;
    cfg.timeout = to;
    save_cfg(&cfg);
    print_centered(240, "Настройки сохранены!");
    return 0;
}

int settings_menu() {
    itoa(cfg.tries, tries_inp);
    itoa(cfg.timeout, timeout_inp);
    item_t *items[] = {
        create_input("Количество попыток для ввода пароля", tries_inp, INPUT_DIGITS, 2, display.width/2, 120),
        create_input("Время блокировки при превышении кол-ва попыток (минут)", timeout_inp, INPUT_DIGITS, 2, display.width/2, 150),
        create_selectable("Сохранить", &save_settings, 0, display.width/2, 180),
        create_selectable("Назад", &open_menu, MENU_LOGGED, display.width/2, 210)
    };
    menu_t menu = create_menu(items, 4);
    draw_background();
    print_centered(70, "Настройки");
    return draw_menu(&menu);
}

int del_account(int id) {
    user_t user;
    get_acc_by_id(id, &user);
    write_log(user.name, user.role, ACTION_DELACC);
    delete_account(id);
    return MENU_MANAGE;
}

int ensure_delete(int id) {
    user_t user;
    get_acc_by_id(id, &user);
    item_t *items[] = { 
        create_selectable("Нет", &open_menu, MENU_MANAGE, display.width/2, 100),
        create_selectable("Да", &del_account, id, display.width/2, 130)
    };
    menu_t menu = create_menu(items, 2);
    draw_background();
    char *s = calloc(100, sizeof(char));
    snprintf(s, 100, "Вы уверены что хотите удалить аккаунт %s?", user.name);
    print_centered(70, s);
    free(s);
    int r = draw_menu(&menu);
    memset(login, 0, MAX_LOGIN);
    return r;
}

int save_new_name(int id) {
    draw_box(0, 220, display.width, 20, BG_COLOR);
    if (!strlen(login)) {
        print_centered(220, "Введите новый логин!");
        return MENU_SAME;
    }
    if (does_user_exist(login)) {
        print_centered(220, "Этот логин уже занят!");
        return MENU_SAME;
    }
    user_t user;
    get_acc_by_id(id, &user);
    write_log(user.name, user.role, ACTION_OLDNAME);
    strncpy(user.name, login, MAX_LOGIN+1);
    update_acc_by_id(id, &user);
    write_log(user.name, user.role, ACTION_NEWNAME);
    print_centered(220, "Логин успешно обновлён!");
    return MENU_SAME;
}

int change_acc_name(int id) {
    user_t user;
    get_acc_by_id(id, &user);
    item_t *items[] = { 
        create_input("Новый логин", login, FALSE, MAX_LOGIN, display.width/2, 100),
        create_selectable("Сохранить", &save_new_name, id, display.width/2, 130),
        create_selectable("Назад", &open_menu, MENU_MANAGE, display.width/2, 160)
    };
    menu_t menu = create_menu(items, 3);
    draw_background();
    char *s = calloc(100, sizeof(char));
    snprintf(s, 100, "Изменить логин для %s", user.name);
    print_centered(70, s);
    free(s);
    int r = draw_menu(&menu);
    memset(login, 0, MAX_LOGIN);
    return r;
}

int save_new_pass(int id) {
    draw_box(0, 220, display.width, 20, BG_COLOR);
    if (!strlen(password)) {
        print_centered(220, "Введите новый пароль!");
        return MENU_SAME;
    }
    user_t user;
    get_acc_by_id(id, &user);
    uint8_t p_hash[HASH_LEN] = {0};
    sha256_hash(password, p_hash);
    memcpy(user.hash, p_hash, HASH_LEN);
    update_acc_by_id(id, &user);
    write_log(user.name, user.role, ACTION_NEWPASS);
    print_centered(220, "Пароль успешно обновлён!");
    return MENU_SAME;
}

int change_acc_password(int id) {
    user_t user;
    get_acc_by_id(id, &user);
    item_t *items[] = { 
        create_input("Новый пароль", password, TRUE, MAX_PASS, display.width/2, 100),
        create_selectable("Сохранить", &save_new_pass, id, display.width/2, 130),
        create_selectable("Назад", &open_menu, MENU_MANAGE, display.width/2, 160)
    };
    menu_t menu = create_menu(items, 3);
    draw_background();
    char *s = calloc(100, sizeof(char));
    snprintf(s, 100, "Изменить пароль для %s", user.name);
    print_centered(70, s);
    free(s);
    int r = draw_menu(&menu);
    memset(password, 0, MAX_PASS);
    return r;
}

int edit_user_menu(int id) {
    user_t user;
    get_acc_by_id(id, &user);
    item_t *items[] = { 
        create_selectable("Назад", &open_menu, MENU_MANAGE, 30, 100),
        create_selectable("Изменить логин", &change_acc_name, id, 30, 160),
        create_selectable("Изменить пароль", &change_acc_password, id, 30, 190),
        create_selectable("Удалить аккаунт", &ensure_delete, id, 30, 220)
    };
    menu_t menu = create_menu(items, user.role == ROLE_ADMIN ? 3 : 4);
    int r = 0;
    while (!r) {
        draw_background();
        char *s = calloc(100, sizeof(char));
        snprintf(s, 100, "Управление аккаунтом %s", user.name);
        print_centered(70, s);
        free(s);
        r = draw_menu(&menu);
    }
    return r;
}

int select_account(int id) {
    edit_user_menu(id);
    return MENU_MANAGE;
}

int manage_menu() {
    uint32_t n = get_accounts_num();
    item_t **items = malloc((n+1)*sizeof(item_t*));
    user_t *users = malloc(n*sizeof(user_t));
    get_accounts(0, n, users);
    uint32_t y = 130;
    for (int i = 0; i < n; i++, y += 30) {
        items[i] = create_selectable(users[i].name, &select_account, i, 50, y);
    }
    items[n] = create_selectable("Назад", &open_menu, MENU_LOGGED, 30, 100);
    menu_t menu = create_menu(items, n+1);
    draw_background();
    print_centered(70, "Выберите пользователя для изменения:");
    int r = draw_menu(&menu);
    free(users);
    return r;
}

int user_menu() {
    uintn_t w = display.width/3;
    item_t *items[] = {
        create_selectable("Загрузить систему", &open_menu, MENU_BOOT, w, 120),
        create_selectable("Выйти из системы", &logout, 0, w, 150),
        create_selectable("Выключить компьютер", &shutdown, 0, w, 180),
        create_selectable("Зарегистрировать пользователя", &open_menu, MENU_REG, w, 210),
        create_selectable("Просмотр журнала событий", &open_menu, MENU_LOGS, w, 240),
        create_selectable("Настройки", &open_menu, MENU_SETTINGS, w, 270),
        create_selectable("Управление пользователями", &open_menu, MENU_MANAGE, w, 300)
    };
    menu_t menu = create_menu(items, role == ROLE_ADMIN ? 7 : 3);
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
    load_settings(&cfg);
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
    case MENU_MANAGE:
        return manage_menu();
    default:
        return MENU_BOOT;
    }
}

/* Entry point with main menu loop */
int main(int argc, char **argv) {
    init();
    int m = MENU_FIRST;
    if (get_accounts_num()) m = MENU_AUTH;
    while (m != MENU_BOOT) {
        m = call_menu(m);
    }
    write_log(current_login, role, ACTION_BOOT);
    free_font();
    return 0;
}