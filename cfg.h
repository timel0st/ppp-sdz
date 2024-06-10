#ifndef CFG_H
#define CFG_H
#include "uefi.h"

#define CFGPATH "cfg"
#define DEFAULT_TRIES 3
#define DEFAULT_TIMEOUT 1

typedef struct cfg {
    uint8_t tries;
    uint8_t timeout;
    uint32_t lock_till;
} cfg_t;

void load_settings(cfg_t *cfg);
void save_cfg(cfg_t *cfg);

#endif