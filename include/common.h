/**
 * 3DS CloudSaver - Common Header
 * Shared types, constants, and includes used across all modules.
 */

#ifndef CLOUDSAVER_COMMON_H
#define CLOUDSAVER_COMMON_H

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

/*─────────────────────────── Version ───────────────────────────*/
#define APP_VERSION_MAJOR   1
#define APP_VERSION_MINOR   0
#define APP_VERSION_PATCH   0
#define APP_VERSION_STRING  "1.0.0"

/*─────────────────────────── Limits ────────────────────────────*/
#define MAX_DEVICE_NAME     32
#define MAX_GAMES           128
#define MAX_SAVEFILES       32
#define MAX_PATH_LEN        256
#define MAX_DESCRIPTION_LEN 128
#define MAX_COMMIT_MSG_LEN  64
#define MAX_HOST_LEN        128
#define MAX_USER_LEN        64
#define MAX_PASS_LEN        64

/*─────────────────────────── Screen ────────────────────────────*/
#define TOP_SCREEN_WIDTH    400
#define TOP_SCREEN_HEIGHT   240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240

#define COVER_WIDTH         48
#define COVER_HEIGHT        48
#define COVER_PADDING       8
#define COVERS_PER_ROW      6
#define COVERS_VISIBLE_ROWS 3

/*─────────────────────────── Colors ────────────────────────────*/
#define CLR_BACKGROUND      C2D_Color32(30, 30, 46, 255)
#define CLR_SURFACE         C2D_Color32(49, 50, 68, 255)
#define CLR_OVERLAY         C2D_Color32(108, 112, 134, 255)
#define CLR_TEXT            C2D_Color32(205, 214, 244, 255)
#define CLR_SUBTEXT         C2D_Color32(166, 173, 200, 255)
#define CLR_ACCENT          C2D_Color32(137, 180, 250, 255)
#define CLR_ACCENT2         C2D_Color32(166, 227, 161, 255)
#define CLR_WARNING         C2D_Color32(249, 226, 175, 255)
#define CLR_ERROR           C2D_Color32(243, 139, 168, 255)
#define CLR_SUCCESS         C2D_Color32(166, 227, 161, 255)
#define CLR_NAVBAR          C2D_Color32(24, 24, 37, 255)
#define CLR_NAVBAR_ACTIVE   C2D_Color32(137, 180, 250, 255)
#define CLR_BUTTON          C2D_Color32(69, 71, 90, 255)
#define CLR_BUTTON_HOVER    C2D_Color32(88, 91, 112, 255)
#define CLR_WHITE           C2D_Color32(255, 255, 255, 255)
#define CLR_BLACK           C2D_Color32(0, 0, 0, 255)
#define CLR_TRANSPARENT     C2D_Color32(0, 0, 0, 0)

/*─────────────────────────── Regions ───────────────────────────*/
typedef enum {
    REGION_EUR = 0,
    REGION_USA,
    REGION_JPN,
    REGION_KOR,
    REGION_TWN,
    REGION_UNKNOWN,
    REGION_COUNT
} GameRegion;

/*─────────────────────────── Connection Type ───────────────────*/
typedef enum {
    CONN_NONE = 0,
    CONN_SFTP,
    CONN_SMB
} ConnectionType;

/*─────────────────────────── App State ─────────────────────────*/
typedef enum {
    STATE_SETUP_DEVICE_NAME = 0,    /* First launch: enter device name */
    STATE_CONNECTING,               /* Connecting to server */
    STATE_MAIN_BROWSE,              /* Main: browsing games on top screen */
    STATE_GAME_SELECTED,            /* A game is selected, showing saves */
    STATE_SAVE_DETAIL,              /* Viewing save file details */
    STATE_SETTINGS,                 /* Settings screen */
    STATE_SYNC_ALL,                 /* Sync all in progress */
    STATE_SYNC_ALL_COMMIT,          /* Entering commit message */
    STATE_KEYBOARD_INPUT,           /* Software keyboard active */
    STATE_ERROR,                    /* Error display */
    STATE_EXIT                      /* App is shutting down */
} AppState;

/*─────────────────────────── Nav Tab ───────────────────────────*/
typedef enum {
    NAV_GAMES = 0,
    NAV_SYNC_ALL,
    NAV_SETTINGS,
    NAV_COUNT
} NavTab;

/*─────────────────────────── Language ──────────────────────────*/
typedef enum {
    LANG_EN = 0,
    LANG_DE,
    LANG_FR,
    LANG_ES,
    LANG_IT,
    LANG_JA,
    LANG_COUNT
} Language;

/*─────────────────────── Structures ────────────────────────────*/

/** Server connection configuration */
typedef struct {
    ConnectionType type;
    char host[MAX_HOST_LEN];
    int port;
    char username[MAX_USER_LEN];
    char password[MAX_PASS_LEN];
    char remote_path[MAX_PATH_LEN]; /* Base path on server */
    bool connected;
} ServerConfig;

/** Persistent application configuration */
typedef struct {
    char device_name[MAX_DEVICE_NAME];
    ServerConfig server;
    Language language;
    bool first_run;
    bool auto_connect;
    bool auto_sync;
} AppConfig;

/** Represents an installed 3DS title */
typedef struct {
    u64 title_id;
    char name[128];
    char product_code[16];
    GameRegion region;
    C2D_Image icon;
    bool has_icon;
    bool has_save_data;
} GameTitle;

/** Represents a save file stored in the cloud */
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

/** Global application context */
typedef struct {
    AppState state;
    AppState prev_state;
    AppConfig config;

    /* Games */
    GameTitle games[MAX_GAMES];
    int game_count;
    int selected_game;
    int scroll_offset;

    /* Save files for selected game */
    SaveFileInfo saves[MAX_SAVEFILES];
    int save_count;
    int selected_save;

    /* Navigation */
    NavTab current_tab;

    /* Animation state */
    float transition_alpha;
    float transition_target;
    bool transitioning;

    /* Connection status */
    bool server_connected;
    char status_message[256];
    float status_timer;

    /* Frame counter for animations */
    u32 frame_count;
} AppContext;

/*─────────────────── Global Context (extern) ───────────────────*/
extern AppContext g_ctx;

/*─────────────────── Utility Macros ────────────────────────────*/
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi) MIN(MAX(v, lo), hi)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif /* CLOUDSAVER_COMMON_H */
