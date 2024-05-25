#ifndef USER_H
#define USER_H
#include "uefi/uefi.h"
#include "utils.h"

#define MAX_LOGIN 16
#define MAX_PASS 16
#define MAX_PASS_LEN    16
#define MAX_USER_LEN    16
#define HASH_LEN        33
#define ACCPATH         "accs"
#define MAX_ATTEMPTS    3
#define COOLDOWN        1 // in minutes

typedef enum {
    ROLE_UNAUTHORIZED,
    ROLE_USER,
    ROLE_ADMIN
} role_t;

struct User;

int check_acc_exist();
void sha256_hash(char *s, uint8_t *d);
int check_password(uint8_t *hash, char *pass);
int get_account(char *, struct User *);
int register_account(char role, char* name, char* pass);
int auth(char* login, char* password);

#endif