#include "config.h"

void config_defaults(AppConfig *cfg)
{
    memset(cfg, 0, sizeof(AppConfig));
    cfg->first_run    = true;
    cfg->language     = LANG_EN;
    cfg->auto_connect = true;
    cfg->auto_sync    = false;
    cfg->server.type  = CONN_NONE;
    cfg->server.port  = 5000;
    strncpy(cfg->server.remote_path, "/CloudSaver", MAX_PATH_LEN - 1);
}

bool config_ensure_dirs(void)
{
    mkdir(CONFIG_DIR, 0777);
    mkdir(SAVES_DIR,  0777);
    mkdir(TEMP_DIR,   0777);
    return true;
}

bool config_save(const AppConfig *cfg)
{
    config_ensure_dirs();

    FILE *f = fopen(CONFIG_FILE, "wb");
    if (!f) return false;

    const u32 magic   = 0x43534156; /* "CSAV" */
    const u32 version = 1;
    fwrite(&magic,   sizeof(u32), 1, f);
    fwrite(&version, sizeof(u32), 1, f);
    fwrite(cfg,      sizeof(AppConfig), 1, f);
    fclose(f);
    return true;
}

bool config_load(AppConfig *cfg)
{
    FILE *f = fopen(CONFIG_FILE, "rb");
    if (!f) return false;

    u32 magic = 0, version = 0;
    if (fread(&magic,   sizeof(u32), 1, f) != 1 ||
        fread(&version, sizeof(u32), 1, f) != 1) {
        fclose(f);
        return false;
    }

    if (magic != 0x43534156 || version != 1) {
        fclose(f);
        return false;
    }

    if (fread(cfg, sizeof(AppConfig), 1, f) != 1) {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

bool config_init(AppConfig *cfg)
{
    config_defaults(cfg);
    config_ensure_dirs();

    if (!config_load(cfg)) {
        cfg->first_run = true;
        config_save(cfg);
    }
    return true;
}
