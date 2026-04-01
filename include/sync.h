/**
 * 3DS CloudSaver - Sync Engine
 * Orchestrates save synchronisation between 3DS and cloud.
 */

#ifndef CLOUDSAVER_SYNC_H
#define CLOUDSAVER_SYNC_H

#include "common.h"

/** Sync result codes. */
typedef enum {
    SYNC_OK = 0,
    SYNC_ERR_NOT_CONNECTED,
    SYNC_ERR_NO_SAVE_DATA,
    SYNC_ERR_EXTRACT_FAIL,
    SYNC_ERR_UPLOAD_FAIL,
    SYNC_ERR_DOWNLOAD_FAIL,
    SYNC_ERR_MKDIR_FAIL,
    SYNC_ERR_META_FAIL,
    SYNC_ERR_IMPORT_FAIL,
    SYNC_ERR_GENERIC
} SyncResult;

/** Progress callback: (current, total, game_name) */
typedef void (*SyncProgressCb)(int current, int total, const char *game_name);

/** Upload a single game's save to the cloud.
 *  commit_message and description are optional (may be NULL). */
SyncResult sync_upload_save(const GameTitle *game,
                            const char *device_name,
                            const char *commit_message,
                            const char *description);

/** Download a save file from the cloud and import it into the game. */
SyncResult sync_download_save(const SaveFileInfo *save_info,
                              const GameTitle *game);

/** Sync All: upload every game's current save to the cloud.
 *  progress_cb is called for each game. */
SyncResult sync_all(const GameTitle *games, int game_count,
                    const char *device_name,
                    const char *commit_message,
                    SyncProgressCb progress_cb);

/** List all saves for a game from the cloud.
 *  Returns the number of saves found, populates out_saves. */
int sync_list_saves(const GameTitle *game,
                    SaveFileInfo *out_saves, int max_saves);

/** Delete a save file from the cloud. */
SyncResult sync_delete_save(const SaveFileInfo *save_info);

/** Get a human-readable error message for a SyncResult. */
const char *sync_error_str(SyncResult result);

#endif /* CLOUDSAVER_SYNC_H */
