#include "user.h"

/* 
    Returns number of accounts at accounts file
*/
uint32_t get_accounts_num() {
    FILE *f = fopen(ACCPATH, "r");
    if (f == NULL)
        return 0;
    size_t size = get_file_len(f);
    fclose(f);
    return size / sizeof(user_t);
}

/* Deletes account at specified position in file (id) */
void delete_account(uint32_t id) {
    uint32_t n = get_accounts_num();
    if (!n)
        return;
    FILE *f = fopen(ACCPATH, "r");
    user_t *users = malloc((n-1)*sizeof(user_t));
    fread(users, sizeof(user_t), id, f);
    user_t to_del;
    fread(&to_del, sizeof(user_t), 1, f);
    if (to_del.role != ROLE_ADMIN) {
        fread(users, sizeof(user_t), n - id - 1, f);
        fclose(f);
        f = fopen(ACCPATH, "w");
        fwrite(users, sizeof(user_t), n - 1, f);
    }
    fclose(f);
    free(users);
}

/* 
    Puts sha256 hash of zstring s to d
    Salted with name and const string STSALT
*/
void sha256_hash(char *s, uint8_t *d) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (BYTE *)s, strlen(s));
    //sha256_update(&ctx, (BYTE *)salt, strlen(salt));
    sha256_update(&ctx, (BYTE *)STSALT, STSALT_LEN);
    sha256_final(&ctx, d);
}

/* Returns result of strncmp on password hash */
int check_password(uint8_t *hash, char *pass, char* login) {
    uint8_t d_hash[HASH_LEN] = {0};
    sha256_hash(pass, d_hash);
    return strncmp((char*)hash, (char*)d_hash, HASH_LEN-1);
}

/* Returns if this name was already used */
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

/* Returnts amount of written users */
uint32_t get_accounts(uint32_t start, uint32_t amount, user_t* users) {
    uint32_t n = get_accounts_num();
    FILE *f = fopen(ACCPATH, "r");
    fseek(f, (int)sizeof(user_t)*(start), SEEK_SET);
    uint32_t r = amount > n - start ? amount : n - start;
    fread(users, sizeof(user_t), r, f);
    fclose(f);
    return r;
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

boolean_t get_acc_by_id(uint32_t id, user_t *user) {
    FILE *f = fopen(ACCPATH, "r");
    fseek(f, id*sizeof(user_t), SEEK_SET);
    size_t s = fread(user, sizeof(user_t), 1, f);
    fclose(f);
    return s;
}

boolean_t update_acc_by_id(uint32_t id, user_t *user) {
    FILE *f = fopen(ACCPATH, "a");
    fseek(f, id*sizeof(user_t), SEEK_SET);
    size_t s = fwrite(user, sizeof(user_t), 1, f);
    fclose(f);
    return s;
}


/* Saves user with specified credentials */
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

/* Returns resulting role after auth */
int auth(char* login, char* password) {
    user_t u;
    int res = get_account(login, &u);
    if (res && !check_password(u.hash, password, login)) {
        return u.role;
    }
    return 0;
}