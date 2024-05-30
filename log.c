#include "log.h"

/* Writes current time in log_date_t struct */
void get_time(log_date_t *ts) {
    efi_time_t t;
    efi_time_capabilities_t cap;
    RT->GetTime(&t, &cap);
    ts->year = t.Year;
    ts->month = t.Month;
    ts->day = t.Day;
    ts->hour = t.Hour;
    ts->minute = t.Minute;
    ts->second = t.Second;
}

/* Writes msg to file at LOGPATH */
void write_log(char* username, role_t role, log_action_t action) {
    log_entry_t entry;
    strncpy(entry.login, username, MAX_LOGIN);
    get_time(&entry.timestamp);
    entry.action = action;
    entry.role = role;
    FILE *f = fopen(LOGPATH, "a");
    fwrite(&entry, sizeof(entry), 1, f);
    fclose(f);
}

/* Puts string representation of given action to string out */
void get_action_string(log_action_t action, char* out) {
    switch (action) {
        case ACTION_LOGIN:
            strncpy(out, "Вход в систему", 27);
            break;
        case ACTION_LOGOUT:
            strncpy(out, "Выход из системы", 31);
            break;
        case ACTION_BOOT:
            strncpy(out, "Загрузка ОС", 22);
            break;
        case ACTION_LOGIN_ATTEMPT:
            strncpy(out, "Неудачная попытка входа", 45);
            break;
        case ACTION_REGISTER:
            strncpy(out, "Регистрация аккаунта", 40);
            break;
        case ACTION_SHUTDOWN:
            strncpy(out, "Выключение компьютера", 42);
            break;
        case ACTION_LOCK:
            strncpy(out, "Блокировка авторизации", 44);
            break;
    }
}

/* Writes role string representation to out string */
void get_role_string(role_t role, char* out) {
    switch (role) {
    case ROLE_UNAUTHORIZED:
        strncpy(out, "None", 5);
        break;
    case ROLE_USER:
        strncpy(out, "User", 5);
        break;
    case ROLE_ADMIN:
        strncpy(out, "Admin", 6);
        break;
    }
}

/* Returns total number of log entries */
int get_entries_num() {
    FILE *f = fopen(LOGPATH, "r");
    int l = get_file_len(f);
    fclose(f);
    return l / sizeof(log_entry_t);
}

/* 
    Retrieves log entries from log file to out
    Returns amount of retrieved entries
*/
uint32_t get_log_entries(uint32_t n, uint32_t start, log_text_entry_t* out) {
    log_entry_t entry;
    FILE *f = fopen(LOGPATH, "r");
    uint32_t i = 0;
    fseek(f, sizeof(log_entry_t), SEEK_END);
    for (; i < n; i++) {
        if (fseek(f, -2*(int)sizeof(log_entry_t), SEEK_CUR))
            break;
        if (!fread(&entry, sizeof(log_entry_t), 1, f))
            break;
        log_date_t t = entry.timestamp;
        sprintf(out->ts, "%04u-%02u-%02u %02u:%02u:%02u",
            (uint64_t)t.year, (uint64_t)t.month, (uint64_t)t.day,
            (uint64_t)t.hour, (uint64_t)t.minute, (uint64_t)t.second);
        snprintf(out->login, 16, "%s", entry.login);
        char a[46] = {0};
        char r[6] = {0}; //need to fix cut role names smh
        get_action_string(entry.action, a);
        get_role_string(entry.role, r);
        snprintf(out->role, 12, "%s", r);
        snprintf(out->action, 50, "%s", a);
        out += sizeof(log_text_entry_t);
    }
    fclose(f);
    return i;
}