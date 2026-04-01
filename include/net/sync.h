#ifndef CLOUDSAVER_SYNC_H
#define CLOUDSAVER_SYNC_H

#include "common.h"

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

typedef void (*SyncProgressCb)(int current, int total, const char *game_name);

SyncResult sync_upload_save(const GameTitle *game,
                            const char *device_name,
                            const char *commit_message,
                            const char *description);

SyncResult sync_download_save(const SaveFileInfo *save_info,
                              const GameTitle *game);

SyncResult sync_all(const GameTitle *games, int game_count,
                    const char *device_name,
                    const char *commit_message,
                    SyncProgressCb progress_cb);

int sync_list_saves(const GameTitle *game,
                    SaveFileInfo *out_saves, int max_saves);

SyncResult sync_delete_save(const SaveFileInfo *save_info);

const char *sync_error_str(SyncResult result);

#endif /* CLOUDSAVER_SYNC_H */
