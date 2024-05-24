#include "user.h"
#include "sha256.h"

struct User {
    char name[MAX_USER_LEN];
    uint8_t hash[HASH_LEN];
    char role;
} user;

int check_acc_exist() {
    /* 
        Checks if any acc exsist in ACCPATH file
        Returns size of file, 0 = no accs created yet 
    */
    FILE *f = fopen(ACCPATH, "r");
    if (f == NULL)
        return 0;
    size_t size = get_file_len(f);
    fclose(f);
    return size;
}

void sha256_hash(char *s, uint8_t *d) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (BYTE *)s, strlen(s));
    sha256_final(&ctx, d);
}

int check_password(char *hash, char *pass) {
    char d_hash[HASH_LEN] = {0};
    sha256_hash(pass, d_hash);
    return strncmp(hash, d_hash, HASH_LEN-1);
}

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
    // transfer into main maybe
    /*
    static int attempts = 0;
    static int lock_till = 0;
    if (time(0) < lock_till) {
        printf("Login attempts locked for %d secs.\n", lock_till - time(0));
        getchar();
        return 0;
    }
    attempts++;
    if (attempts > MAX_ATTEMPTS) {
        printf("Max login attepmpts num was exceeded! Login attempts locked for %d secs.\n", COOLDOWN);
        attempts = 0;
        lock_till = time(0) + COOLDOWN;
        getchar();
        return 0;
    }
    printf("Login attempt %d/%d\n", attempts, MAX_ATTEMPTS);
    */
    struct User u;
    int res = get_account(login, &u);
    if (res && !check_password(u.hash, password)) {
        return u.role;
    }
    return 0;
}