/**
 * 3DS CloudSaver - Configuration Manager
 * Handles loading/saving app config from SD card.
 */

#ifndef CLOUDSAVER_CONFIG_H
#define CLOUDSAVER_CONFIG_H

#include "common.h"

#define CONFIG_DIR      "/3ds/CloudSaver"
#define CONFIG_FILE     "/3ds/CloudSaver/config.dat"
#define SAVES_DIR       "/3ds/CloudSaver/saves"
#define TEMP_DIR        "/3ds/CloudSaver/tmp"

/**
 * Initialise the configuration subsystem.
 * Creates directories if needed, loads existing config or writes defaults.
 */
bool config_init(AppConfig *cfg);

/** Save current configuration to SD card. */
bool config_save(const AppConfig *cfg);

/** Load configuration from SD card. */
bool config_load(AppConfig *cfg);

/** Reset configuration to defaults. */
void config_defaults(AppConfig *cfg);

/** Ensure all required directories exist on the SD card. */
bool config_ensure_dirs(void);

/** Present the software keyboard and ask for the device name. */
bool config_prompt_device_name(char *out, size_t max_len);

/** Present the software keyboard for generic text input. */
bool config_keyboard_input(const char *hint, const char *initial,
                           char *out, size_t max_len);

#endif /* CLOUDSAVER_CONFIG_H */
