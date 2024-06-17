#include "log.h"

/* Writes current time in log_date_t struct */
void get_time(log_date_t *ts) {
    efi_time_t *t = malloc(sizeof(efi_time_t));
    RT->GetTime(t, NULL);
    ts->year = t->Year;
    ts->month = t->Month;
    ts->day = t->Day;
    ts->hour = t->Hour;
    ts->minute = t->Minute;
    ts->second = t->Second;
    free(t);
}

/* Returns total number of log entries */
uint32_t get_entries_num() {
    FILE *f = fopen(LOGPATH, "r");
    uint32_t l = get_file_len(f);
    fclose(f);
    return (uint32_t)(l / sizeof(log_entry_t));
}

/* trims oldest entries from log for size of MAX_LOG_ENTRIES-1 */
void trim_log() {
    FILE *f = fopen(LOGPATH, "r");
    log_entry_t *buf = malloc(sizeof(log_entry_t)*(MAX_LOG_ENTRIES));
    fseek(f, -(long)sizeof(log_entry_t)*(MAX_LOG_ENTRIES ), SEEK_END);
    fread(buf, sizeof(log_entry_t), MAX_LOG_ENTRIES, f);
    fclose(f);
    f = fopen(LOGPATH, "w");
    fwrite(buf, sizeof(log_entry_t), MAX_LOG_ENTRIES, f);
    free(buf);
    fclose(f);
}

/* 
    Creates log entry from given arguments and appends it to file at LOGPATH 
    Trims log on write if MAX_LOG_ENTRIES was reached
*/
void write_log(char* username, role_t role, log_action_t action) {
    log_entry_t *entry = malloc(sizeof(log_entry_t));
    strncpy(entry->login, username, MAX_LOGIN);
    get_time(&entry->timestamp);
    entry->action = action;
    entry->role = role;
    FILE *f = fopen(LOGPATH, "a");
    fwrite(entry, sizeof(log_entry_t), 1, f);
    fclose(f);
    if (get_entries_num() >= MAX_LOG_ENTRIES) {
        trim_log();
        fprintf(stderr, "trimmed");
    }
}


/* Puts string representation of given action to string out */
void get_action_string(log_action_t action, char* out) {
    switch (action) {
        case ACTION_LOGIN:
            strncpy(out, "Вход в систему", 50);
            break;
        case ACTION_LOGOUT:
            strncpy(out, "Выход из системы", 50);
            break;
        case ACTION_BOOT:
            strncpy(out, "Загрузка ОС", 50);
            break;
        case ACTION_LOGIN_ATTEMPT:
            strncpy(out, "Неудачная попытка входа", 50);
            break;
        case ACTION_REGISTER:
            strncpy(out, "Регистрация аккаунта", 50);
            break;
        case ACTION_SHUTDOWN:
            strncpy(out, "Выключение компьютера", 50);
            break;
        case ACTION_LOCK:
            strncpy(out, "Блокировка авторизации", 50);
            break;
        case ACTION_OLDNAME:
            strncpy(out, "Изменение имени с", 50);
            break;
        case ACTION_NEWNAME:
            strncpy(out, "Изменение имени на", 50);
            break;
        case ACTION_NEWPASS:
            strncpy(out, "Изменение пароля", 50);
            break;
        case ACTION_DELACC:
            strncpy(out, "Удаление аккаунта", 50);
            break;
    }
}

/* Writes role string representation to out string */
void get_role_string(role_t role, char* out) {
    switch (role) {
    case ROLE_UNAUTHORIZED:
        strncpy(out, "None", 6);
        break;
    case ROLE_USER:
        strncpy(out, "User", 6);
        break;
    case ROLE_ADMIN:
        strncpy(out, "Admin", 6);
        break;
    }
}

/* 
    Retrieves log entries from log file to out
    Returns amount of retrieved entries
*/
uint32_t get_log_entries(uint32_t n, uint32_t start, log_text_entry_t* out) {
    log_entry_t *entry = malloc(sizeof(log_entry_t));
    FILE *f = fopen(LOGPATH, "r");
    uint32_t i = 0;
    fseek(f, (int)sizeof(log_entry_t)*(1 - start), SEEK_END);
    for (; i < n; i++) {
        if (fseek(f, -2*(int)sizeof(log_entry_t), SEEK_CUR))
            break;
        if (!fread(entry, sizeof(log_entry_t), 1, f))
            break;
        snprintf(out[i].ts, MAX_TS_LEN, "%04u-%02u-%02u %02u:%02u:%02u",
            (uint64_t)entry->timestamp.year,
            (uint64_t)entry->timestamp.month,
            (uint64_t)entry->timestamp.day,
            (uint64_t)entry->timestamp.hour,
            (uint64_t)entry->timestamp.minute,
            (uint64_t)entry->timestamp.second);
        snprintf(out[i].login, MAX_LOGIN+1, "%s\0", entry->login);
        char *a = calloc(MAX_ACTION_LEN, sizeof(uint8_t));
        char *r = calloc(MAX_ROLE_LEN, sizeof(uint8_t));
        get_action_string(entry->action, a);
        get_role_string(entry->role, r);
        snprintf(out[i].role, 12, "%s", r);
        snprintf(out[i].action, 50, "%s", a);
        free(a);
        free(r);
    }
    fclose(f);
    free(entry);
    return i;
}

