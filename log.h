#ifndef LOG_H
#define LOG_H
#include "uefi/uefi.h"
#include "utils.h"
#include "user.h"

// path to logfile
#define LOGPATH         "logfile"
// max entries in log file
#define MAX_LOG_ENTRIES 100
// max length of entities string representation
#define MAX_TS_LEN      24
#define MAX_ROLE_LEN    6
#define MAX_ACTION_LEN  50
#define ENTRIES_PER_PAGE 20

// types of actions that may store in log
typedef enum {
    ACTION_LOGIN,
    ACTION_LOGOUT,
    ACTION_BOOT,
    ACTION_LOGIN_ATTEMPT,
    ACTION_SHUTDOWN,
    ACTION_REGISTER,
    ACTION_LOCK,
    ACTION_OLDNAME,
    ACTION_NEWNAME,
    ACTION_NEWPASS,
    ACTION_DELACC
} log_action_t;

// struct for storing date
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} log_date_t;

// log entry structure 
typedef struct {
    log_date_t timestamp;
    char login[MAX_LOGIN];
    role_t role;
    log_action_t action;
} log_entry_t;

// text representation of log entry
typedef struct {
    char ts[MAX_TS_LEN];
    char login[MAX_LOGIN+1];
    char role[MAX_ROLE_LEN];
    char action[MAX_ACTION_LEN];
} log_text_entry_t;

void write_log(char* username, role_t role, log_action_t action);
uint32_t get_log_entries(uint32_t n, uint32_t start, log_text_entry_t* out);
uint32_t get_entries_num();

#endif