#include "cfg.h"

/* Creates config file */
void create_cfg() {
    FILE *f = fopen(CFGPATH, "w");
    cfg_t d = {DEFAULT_TRIES, DEFAULT_TIMEOUT, 0};
    fwrite(&d, sizeof(cfg_t), 1, f);
    fclose(f);
}

/* load settings to cfg strutct */
void load_settings(cfg_t *cfg) {
    FILE *f = fopen(CFGPATH, "r");
    if (!f) {
        create_cfg();
        f = fopen(CFGPATH, "r");
    }
    fread(cfg, sizeof(cfg_t), 1, f);
    fclose(f);
}

/* saves current config */
void save_cfg(cfg_t *cfg) {
    FILE *f = fopen(CFGPATH, "w");
    //cfg_t d = {tries, timeout};
    fwrite(cfg, sizeof(cfg_t), 1, f);
    fclose(f);
}