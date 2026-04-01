#ifndef CLOUDSAVER_TYPES_H
#define CLOUDSAVER_TYPES_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "constants.h"

/* ── Regions ── */

typedef enum {
    REGION_EUR = 0,
    REGION_USA,
    REGION_JPN,
    REGION_KOR,
    REGION_TWN,
    REGION_UNKNOWN,
    REGION_COUNT
} GameRegion;

/* ── Connection ── */

typedef enum {
    CONN_NONE = 0,
    CONN_SFTP,
    CONN_SMB
} ConnectionType;

/* ── App State ── */

typedef enum {
    STATE_SETUP_DEVICE_NAME = 0,
    STATE_CONNECTING,
    STATE_LOADING,
    STATE_MAIN_BROWSE,
    STATE_GAME_SELECTED,
    STATE_SAVE_DETAIL,
    STATE_SETTINGS,
    STATE_SYNC_ALL,
    STATE_SYNC_ALL_COMMIT,
    STATE_KEYBOARD_INPUT,
    STATE_ERROR,
    STATE_EXIT
} AppState;

/* ── Navigation ── */

typedef enum {
    NAV_GAMES = 0,
    NAV_SYNC_ALL,
    NAV_SETTINGS,
    NAV_COUNT
} NavTab;

/* ── Language ── */

typedef enum {
    LANG_EN = 0,
    LANG_DE,
    LANG_FR,
    LANG_ES,
    LANG_IT,
    LANG_JA,
    LANG_COUNT
} Language;

/* ── Structs ── */

typedef struct {
    ConnectionType type;
    char host[MAX_HOST_LEN];
    int port;
    char username[MAX_USER_LEN];
    char password[MAX_PASS_LEN];
    char remote_path[MAX_PATH_LEN];
    bool connected;
} ServerConfig;

typedef struct {
    char device_name[MAX_DEVICE_NAME];
    ServerConfig server;
    Language language;
    bool first_run;
    bool auto_connect;
    bool auto_sync;
} AppConfig;

typedef struct {
    u64 title_id;
    char name[128];
    char product_code[16];
    GameRegion region;
    C2D_Image icon;
    bool has_icon;
    bool has_save_data;
} GameTitle;

typedef struct {
    char filename[MAX_PATH_LEN];
    char device_name[MAX_DEVICE_NAME];
    char game_name[64];
    u64 game_id;
    char description[MAX_DESCRIPTION_LEN];
    char commit_message[MAX_COMMIT_MSG_LEN];
    time_t created_at;
    time_t modified_at;
    size_t file_size;
    GameRegion region;
} SaveFileInfo;

#endif /* CLOUDSAVER_TYPES_H */
