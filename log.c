#include "log.h"

//need to transform this for output later on
void file_write_datetime(FILE* f) {
    efi_time_t t;
    efi_time_capabilities_t cap;
    RT->GetTime(&t, &cap);
    fprintf(f, "[%04u-%02u-%02u %02u:%02u:%02u] ",
            (uint64_t)t.Year, (uint64_t)t.Month, (uint64_t)t.Day,
            (uint64_t)t.Hour, (uint64_t)t.Minute, (uint64_t)t.Second);
}

/* Writes msg to file at LOGPATH */
void write_log(char* username, role_t role, log_action_t action) {
    log_entry_t entry;
    strncpy(entry.login, username, MAX_LOGIN);
    entry.timestamp = time(0);
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

void get_log_entries(int n, int start, log_text_entry_t* out, int* amount) {
    log_entry_t entry;
    FILE *f = fopen(LOGPATH, "r");
    *amount = 0;
    for (int i = 0; i < n; i++, *amount+=1) {
        if (!fread(&entry, sizeof(entry), 1, f))
            break;
        snprintf(out->ts, 16, "%d", entry.timestamp);
        snprintf(out->login, 16, "%s", entry.login);
        snprintf(out->role, 5, "%d", entry.role);
        char a[46] = {0};
        get_action_string(entry.action, a);
        snprintf(out->action, 50, "%s", a);
        out += sizeof(log_text_entry_t);
    }
    fclose(f);
}