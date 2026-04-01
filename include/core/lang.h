/**
 * 3DS CloudSaver - Localization / Multi-Language Support
 * String table with keys for all UI text.
 */

#ifndef CLOUDSAVER_LANG_H
#define CLOUDSAVER_LANG_H

#include "common.h"

/*───────────────── String Keys ─────────────────*/
typedef enum {
    /* General */
    STR_APP_TITLE = 0,
    STR_OK,
    STR_CANCEL,
    STR_BACK,
    STR_YES,
    STR_NO,
    STR_ERROR,
    STR_SUCCESS,
    STR_LOADING,
    STR_PLEASE_WAIT,

    /* Setup */
    STR_WELCOME,
    STR_ENTER_DEVICE_NAME,
    STR_DEVICE_NAME_HINT,

    /* Main */
    STR_NO_GAMES_FOUND,
    STR_INSTALLED_GAMES,
    STR_SELECT_GAME,
    STR_NO_SAVES,
    STR_SAVES_FOR,

    /* Save details */
    STR_SAVE_CREATED,
    STR_SAVE_DEVICE,
    STR_SAVE_GAME,
    STR_SAVE_DESCRIPTION,
    STR_SAVE_SIZE,
    STR_SAVE_REGION,
    STR_ADD_DESCRIPTION,
    STR_EDIT_DESCRIPTION,

    /* Sync */
    STR_SYNC_ALL,
    STR_SYNC_ALL_DESC,
    STR_SYNC_IN_PROGRESS,
    STR_SYNC_COMPLETE,
    STR_SYNC_FAILED,
    STR_ENTER_COMMIT_MSG,
    STR_COMMIT_MSG_HINT,
    STR_UPLOAD_SAVE,
    STR_DOWNLOAD_SAVE,

    /* Navigation */
    STR_NAV_GAMES,
    STR_NAV_SYNC,
    STR_NAV_SETTINGS,

    /* Settings */
    STR_SETTINGS,
    STR_SETTINGS_SERVER,
    STR_SETTINGS_LANGUAGE,
    STR_SETTINGS_DEVICE,
    STR_SETTINGS_AUTO_CONNECT,
    STR_SETTINGS_AUTO_SYNC,
    STR_SERVER_HOST,
    STR_SERVER_PORT,
    STR_SERVER_USER,
    STR_SERVER_PASS,
    STR_SERVER_PATH,
    STR_SERVER_TYPE,
    STR_SERVER_TYPE_SFTP,
    STR_SERVER_TYPE_SMB,
    STR_SERVER_TYPE_NONE,
    STR_SERVER_CONNECT,
    STR_SERVER_DISCONNECT,
    STR_SERVER_TEST,
    STR_SERVER_STATUS_OK,
    STR_SERVER_STATUS_FAIL,
    STR_SERVER_NOT_CONFIGURED,

    /* Connection */
    STR_CONNECTING,
    STR_CONNECTED,
    STR_DISCONNECTED,
    STR_CONNECTION_FAILED,
    STR_CONNECTION_SUCCESS,

    /* Regions */
    STR_REGION_EUR,
    STR_REGION_USA,
    STR_REGION_JPN,
    STR_REGION_KOR,
    STR_REGION_UNKNOWN,

    /* Language names (displayed in their own language) */
    STR_LANG_EN,
    STR_LANG_DE,
    STR_LANG_FR,
    STR_LANG_ES,
    STR_LANG_IT,
    STR_LANG_JA,

    STR_COUNT
} StringKey;

/** Initialise the language system with the given language. */
void lang_init(Language lang);

/** Set the current language. */
void lang_set(Language lang);

/** Get the current language. */
Language lang_get(void);

/** Get a localised string by key. */
const char *lang_str(StringKey key);

/** Get the display name for a language. */
const char *lang_name(Language lang);

/** Cycle to the next language and return it. */
Language lang_next(Language current);

#endif /* CLOUDSAVER_LANG_H */
