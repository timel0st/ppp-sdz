#include "cfg.h"

//switch to cfg struct in main later?

void create_cfg() {
    FILE *f = fopen(CFGPATH, "w");
    cfg_t d = {DEFAULT_TRIES, DEFAULT_TIMEOUT, 0};
    fwrite(&d, sizeof(cfg_t), 1, f);
    fclose(f);
}

void load_settings(uint8_t* tries, uint8_t* timeout, uint32_t* lock_till) {
    FILE *f = fopen(CFGPATH, "r");
    if (!f) {
        create_cfg();
        f = fopen(CFGPATH, "r");
    }
    cfg_t d;
    fread(&d, sizeof(cfg_t), 1, f);
    *tries = d.tries;
    *timeout = d.timeout;
    *lock_till = d.lock_till;
    fclose(f);
}

void save_cfg(uint8_t tries, uint8_t timeout) {
    FILE *f = fopen(CFGPATH, "r");
    cfg_t d = {tries, timeout};
    fwrite(&d, sizeof(cfg_t), 1, f);
    fclose(f);
}

void update_lock(uint32_t lock_till) {
    FILE *f = fopen(CFGPATH, "r");
    cfg_t d;
    fread(&d, sizeof(cfg_t), 1, f);
    fclose(f);
    f = fopen(CFGPATH, "w");
    d.lock_till = lock_till;
    fwrite(&d, sizeof(cfg_t), 1, f);
    fclose(f);
}