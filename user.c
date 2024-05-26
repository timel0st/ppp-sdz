#include "user.h"
#include "sha256.h"

/* 
    Checks if any acc exsist in ACCPATH file
    Returns size of file, 0 = no accs created yet 
*/
int check_acc_exist() {
    FILE *f = fopen(ACCPATH, "r");
    if (f == NULL)
        return 0;
    size_t size = get_file_len(f);
    fclose(f);
    return size;
}

/* Puts sha256 hash of zstring s to d*/
void sha256_hash(char *s, uint8_t *d) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (BYTE *)s, strlen(s));
    sha256_final(&ctx, d);
}

/* Returns result of strncmp on password hash */
int check_password(uint8_t *hash, char *pass) {
    uint8_t d_hash[HASH_LEN] = {0};
    sha256_hash(pass, d_hash);
    return strncmp((char*)hash, (char*)d_hash, HASH_LEN-1);
}

/* returns if this name was already used */
boolean_t does_user_exist(char *name) {
    FILE *f = fopen(ACCPATH, "r");
    if (!f)
        return 0;
    size_t size;
    size = get_file_len(f);
    for (int i = 0; i < size/sizeof(user_t); i++) {
        user_t su;
        fread(&su, sizeof(user_t), 1, f);
        if (strncmp(name, su.name, strlen(su.name)))
            continue;
        fclose(f);
        return 1;
    }
    return 0;
}

/*
    Puts user with specifed name to u user struct
    Returns result of finding user with specified name
*/
int get_account(char *name, user_t *u) {
    FILE *f = fopen(ACCPATH, "r");
    if (!f)
        return 0;
    size_t size;
    size = get_file_len(f);
    for (int i = 0; i < size/sizeof(user_t); i++) {
        user_t su;
        fread(&su, sizeof(user_t), 1, f);
        if (strncmp(name, su.name, strlen(su.name)))
            continue;
        fseek(f, -(long)sizeof(user_t), SEEK_CUR);
        if (u != NULL)
            fread(u, sizeof(user_t), 1, f);
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

/* saves user with specified credentials */
int register_account(char role, char* name, char* pass) {
    user_t u;
    strncpy(u.name, name, MAX_USER_LEN);
    uint8_t p_hash[HASH_LEN] = {0};
    sha256_hash(pass, p_hash);
    memcpy(u.hash, p_hash, HASH_LEN);
    u.role = role;
    FILE *f = fopen(ACCPATH, "a");
    fwrite(&u, sizeof(user_t), 1, f);
    fclose(f);
    return 0;
}

/* returns resulting role after auth */
int auth(char* login, char* password) {
    user_t u;
    int res = get_account(login, &u);
    if (res && !check_password(u.hash, password)) {
        return u.role;
    }
    return 0;
}