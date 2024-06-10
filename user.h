#ifndef USER_H
#define USER_H
#include "uefi/uefi.h"
#include "utils.h"
#include "crypto-algorithms/sha256.h"

#define MAX_LOGIN 16
#define MAX_PASS 16
#define MAX_PASS_LEN    16
#define MAX_USER_LEN    16
#define HASH_LEN        33
#define ACCPATH         "accs"
#define MAX_ATTEMPTS    3
#define COOLDOWN        1 // in minutes
#define STSALT          "PPPSDZ"
#define STSALT_LEN      6

typedef enum {
    ROLE_UNAUTHORIZED,
    ROLE_USER,
    ROLE_ADMIN
} role_t;

typedef struct {
    char name[MAX_USER_LEN];
    uint8_t hash[HASH_LEN];
    char role;
} user_t;

uint32_t get_accounts_num();
void sha256_hash(char *s, uint8_t *d);
void delete_account(uint32_t id);
int check_password(uint8_t *hash, char *pass, char* login);
uint32_t get_accounts(uint32_t start, uint32_t amount, user_t* users);
int get_account(char *, user_t *);
int register_account(char role, char* name, char* pass);
int auth(char* login, char* password);
boolean_t does_user_exist(char *name);
boolean_t get_acc_by_id(uint32_t id, user_t *user);
boolean_t update_acc_by_id(uint32_t id, user_t *user);

#endif