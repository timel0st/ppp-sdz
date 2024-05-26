#ifndef CFG_H
#define CFG_H
#include "uefi/uefi.h"
#define CFGPATH "cfg"
#define DEFAULT_TRIES 3
#define DEFAULT_TIMEOUT 1

typedef struct cfg {
    uint8_t tries;
    uint8_t timeout;
    uint32_t lock_till;
} cfg_t;

void load_settings(uint8_t* tries, uint8_t* timeout, uint32_t *lock_till);
void save_cfg(uint8_t tries, uint8_t timeout);
void update_lock(uint32_t lock_till);

#endif