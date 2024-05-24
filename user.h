#ifndef USER_H
#define USER_H
#include "uefi/uefi.h"
#include "utils.h"

#define MAX_PASS_LEN    16
#define MAX_USER_LEN    16
#define HASH_LEN        33
#define ACCPATH         "accs"

struct User;

int check_acc_exist();
void sha256_hash(char *s, uint8_t *d);
int check_password(char *, char *);
int get_account(char *, struct User *);

#endif