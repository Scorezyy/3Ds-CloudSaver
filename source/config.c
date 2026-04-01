/**
 * 3DS CloudSaver - Configuration Manager Implementation
 */

#include "config.h"
#include "lang.h"

/*═══════════════════════════════════════════════════════════════*
 *  Defaults
 *═══════════════════════════════════════════════════════════════*/
void config_defaults(AppConfig *cfg)
{
    memset(cfg, 0, sizeof(AppConfig));
    cfg->first_run    = true;
    cfg->language     = LANG_EN;
    cfg->auto_connect = true;
    cfg->auto_sync    = false;
    cfg->server.type  = CONN_NONE;
    cfg->server.port  = 22; /* default SFTP */
    strncpy(cfg->server.remote_path, "/CloudSaver", MAX_PATH_LEN - 1);
}

/*═══════════════════════════════════════════════════════════════*
 *  Directory creation
 *═══════════════════════════════════════════════════════════════*/
bool config_ensure_dirs(void)
{
    mkdir(CONFIG_DIR, 0777);
    mkdir(SAVES_DIR,  0777);
    mkdir(TEMP_DIR,   0777);
    return true;
}

/*═══════════════════════════════════════════════════════════════*
 *  Save / Load (binary format for simplicity on 3DS)
 *═══════════════════════════════════════════════════════════════*/
bool config_save(const AppConfig *cfg)
{
    config_ensure_dirs();

    FILE *f = fopen(CONFIG_FILE, "wb");
    if (!f) return false;

    /* Write a magic number + version so we can detect corrupt files */
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

/*═══════════════════════════════════════════════════════════════*
 *  Init
 *═══════════════════════════════════════════════════════════════*/
bool config_init(AppConfig *cfg)
{
    config_defaults(cfg);
    config_ensure_dirs();

    if (!config_load(cfg)) {
        /* First run or corrupt config – keep defaults */
        cfg->first_run = true;
        config_save(cfg);
    }
    return true;
}

/*═══════════════════════════════════════════════════════════════*
 *  Software Keyboard Wrappers
 *═══════════════════════════════════════════════════════════════*/
bool config_prompt_device_name(char *out, size_t max_len)
{
    SwkbdState swkbd;
    char buf[MAX_DEVICE_NAME] = {0};

    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, (int)max_len - 1);
    swkbdSetHintText(&swkbd, lang_str(STR_DEVICE_NAME_HINT));
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetFeatures(&swkbd, SWKBD_DEFAULT_QWERTY);

    SwkbdButton button = swkbdInputText(&swkbd, buf, sizeof(buf));
    if (button == SWKBD_BUTTON_CONFIRM && buf[0] != '\0') {
        strncpy(out, buf, max_len - 1);
        out[max_len - 1] = '\0';
        return true;
    }
    return false;
}

bool config_keyboard_input(const char *hint, const char *initial,
                           char *out, size_t max_len)
{
    SwkbdState swkbd;
    char buf[256] = {0};

    if (initial && initial[0] != '\0')
        strncpy(buf, initial, sizeof(buf) - 1);

    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, (int)max_len - 1);
    swkbdSetHintText(&swkbd, hint ? hint : "");
    swkbdSetInitialText(&swkbd, buf);
    swkbdSetFeatures(&swkbd, SWKBD_DEFAULT_QWERTY);

    SwkbdButton button = swkbdInputText(&swkbd, buf, sizeof(buf));
    if (button == SWKBD_BUTTON_CONFIRM) {
        strncpy(out, buf, max_len - 1);
        out[max_len - 1] = '\0';
        return true;
    }
    return false;
}
