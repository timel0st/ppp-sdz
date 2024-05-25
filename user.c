#include "user.h"
#include "sha256.h"

struct User {
    char name[MAX_USER_LEN];
    uint8_t hash[HASH_LEN];
    char role;
} user;

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

/*
    Puts user with specifed name to u user struct
    Returns result of finding user with specified name
*/
int get_account(char *name, struct User *u) {
    FILE *f = fopen(ACCPATH, "r");
    if (!f)
        return 0;
    size_t size;
    size = get_file_len(f);
    for (int i = 0; i < size/sizeof(struct User); i++) {
        struct User su;
        fread(&su, sizeof(struct User), 1, f);
        if (strncmp(name, su.name, strlen(su.name)))
            continue;
        fseek(f, -(long)sizeof(struct User), SEEK_CUR);
        if (u != NULL)
            fread(u, sizeof(struct User), 1, f);
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

/* saves user with specified credentials */
int register_account(char role, char* name, char* pass) {
    struct User u;
    strncpy(u.name, name, MAX_USER_LEN);
    uint8_t p_hash[HASH_LEN] = {0};
    sha256_hash(pass, p_hash);
    memcpy(u.hash, p_hash, HASH_LEN);
    u.role = role;
    FILE *f = fopen(ACCPATH, "a");
    fwrite(&u, sizeof(struct User), 1, f);
    fclose(f);
    return 0;
}

/* returns resulting role after auth */
int auth(char* login, char* password) {
    struct User u;
    int res = get_account(login, &u);
    if (res && !check_password(u.hash, password)) {
        return u.role;
    }
    return 0;
}