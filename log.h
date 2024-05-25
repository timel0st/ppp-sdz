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
    uint32_t timestamp;
    char login[MAX_LOGIN];
    role_t role;
    log_action_t action;
} log_entry_t;

typedef struct {
    char ts[16];
    char login[16];
    char role[5];
    char action[50];
} log_text_entry_t;

void write_log(char* username, role_t role, log_action_t action);
void get_log_entries(int n, int start, log_text_entry_t* out, int* amount);

#endif