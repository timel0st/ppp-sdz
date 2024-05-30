#ifndef LOG_H
#define LOG_H
#include "uefi/uefi.h"
#include "utils.h"
#include "user.h"

#define LOGPATH         "logfile"

typedef enum {
    ACTION_LOGIN,
    ACTION_LOGOUT,
    ACTION_BOOT,
    ACTION_LOGIN_ATTEMPT,
    ACTION_SHUTDOWN,
    ACTION_REGISTER,
    ACTION_LOCK
} log_action_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} log_date_t;

typedef struct {
    log_date_t timestamp;
    char login[MAX_LOGIN];
    role_t role;
    log_action_t action;
} log_entry_t;

typedef struct {
    char ts[20];
    char login[16];
    char role[6];
    char action[50];
} log_text_entry_t;

void write_log(char* username, role_t role, log_action_t action);
uint32_t get_log_entries(uint32_t n, uint32_t start, log_text_entry_t* out);

#endif