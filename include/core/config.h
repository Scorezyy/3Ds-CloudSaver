#ifndef CLOUDSAVER_CONFIG_H
#define CLOUDSAVER_CONFIG_H

#include "common.h"

#define CONFIG_DIR  "/3ds/CloudSaver"
#define CONFIG_FILE "/3ds/CloudSaver/config.dat"
#define SAVES_DIR   "/3ds/CloudSaver/saves"
#define TEMP_DIR    "/3ds/CloudSaver/tmp"

bool config_init(AppConfig *cfg);
bool config_save(const AppConfig *cfg);
bool config_load(AppConfig *cfg);
void config_defaults(AppConfig *cfg);
bool config_ensure_dirs(void);

#endif
