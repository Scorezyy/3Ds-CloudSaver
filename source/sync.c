/**
 * 3DS CloudSaver - Sync Engine Implementation
 * ────────────────────────────────────────────
 * Orchestrates save extraction, upload, download, and import.
 * Follows the cloud path structure:
 *   /DEVICENAME/GAMENAME/GAMEID/SAVEFILES/*.sav
 */

#include "sync.h"
#include "save.h"
#include "network.h"
#include "config.h"

#include <unistd.h>

/*═══════════════════════════════════════════════════════════════*
 *  Error strings
 *═══════════════════════════════════════════════════════════════*/
const char *sync_error_str(SyncResult result)
{
    switch (result) {
    case SYNC_OK:                return "OK";
    case SYNC_ERR_NOT_CONNECTED: return "Not connected to server";
    case SYNC_ERR_NO_SAVE_DATA:  return "No save data found";
    case SYNC_ERR_EXTRACT_FAIL:  return "Failed to extract save";
    case SYNC_ERR_UPLOAD_FAIL:   return "Upload failed";
    case SYNC_ERR_DOWNLOAD_FAIL: return "Download failed";
    case SYNC_ERR_MKDIR_FAIL:    return "Failed to create directory";
    case SYNC_ERR_META_FAIL:     return "Failed to write metadata";
    case SYNC_ERR_IMPORT_FAIL:   return "Failed to import save";
    case SYNC_ERR_GENERIC:       return "Unknown error";
    default:                     return "???";
    }
}

/*═══════════════════════════════════════════════════════════════*
 *  Upload a single game's save
 *═══════════════════════════════════════════════════════════════*/
SyncResult sync_upload_save(const GameTitle *game,
                            const char *device_name,
                            const char *commit_message,
                            const char *description)
{
    if (!net_is_connected())
        return SYNC_ERR_NOT_CONNECTED;

    /* 1. Extract save data to temp directory */
    char temp_dir[MAX_PATH_LEN];
    snprintf(temp_dir, sizeof(temp_dir), "%s/%016llX",
             TEMP_DIR, (unsigned long long)game->title_id);

    int extract_size = save_extract(game->title_id, temp_dir);
    if (extract_size < 0)
        return SYNC_ERR_NO_SAVE_DATA;
    if (extract_size == 0)
        return SYNC_ERR_EXTRACT_FAIL;

    /* 2. Build the cloud path */
    char cloud_dir[MAX_PATH_LEN];
    save_build_cloud_path(cloud_dir, sizeof(cloud_dir),
                          device_name, game->name, game->title_id);

    /* 3. Create remote directories */
    if (!net_mkdir(cloud_dir))
        return SYNC_ERR_MKDIR_FAIL;

    /* 4. Build save id (timestamp) */
    time_t now = time(NULL);
    char save_id[64];
    struct tm *tm = localtime(&now);
    if (tm) {
        snprintf(save_id, sizeof(save_id), "%04d%02d%02d_%02d%02d%02d",
                 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
    } else {
        snprintf(save_id, sizeof(save_id), "%lld", (long long)now);
    }

    /* 5. Upload all extracted files */
    DIR *dir = opendir(temp_dir);
    if (!dir) return SYNC_ERR_EXTRACT_FAIL;

    struct dirent *ent;
    bool any_uploaded = false;
    size_t total_size = 0;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char local_file[MAX_PATH_LEN];
        snprintf(local_file, sizeof(local_file), "%s/%s",
                 temp_dir, ent->d_name);

    /* Remote path: cloud_dir/<save_id>/<original_filename> */
    char remote_file[MAX_PATH_LEN];
    snprintf(remote_file, sizeof(remote_file), "%s/%s/%s",
         cloud_dir, save_id, ent->d_name);

        if (!net_upload(local_file, remote_file)) {
            closedir(dir);
            return SYNC_ERR_UPLOAD_FAIL;
        }

        /* Get file size for metadata */
        struct stat st;
        if (stat(local_file, &st) == 0) {
            total_size += (size_t)st.st_size;
        }

        any_uploaded = true;
    }
    closedir(dir);

    if (!any_uploaded)
        return SYNC_ERR_NO_SAVE_DATA;

    /* 6. Write metadata file (stored as '<save_dir>/meta') */
    SaveFileInfo meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.filename, save_id, MAX_PATH_LEN - 1);
    strncpy(meta.device_name, device_name, MAX_DEVICE_NAME - 1);
    strncpy(meta.game_name, game->name, 63);
    meta.game_id = game->title_id;
    meta.created_at = now;
    meta.modified_at = now;
    meta.file_size = total_size;
    meta.region = game->region;

    if (commit_message)
        strncpy(meta.commit_message, commit_message, MAX_COMMIT_MSG_LEN - 1);
    if (description)
        strncpy(meta.description, description, MAX_DESCRIPTION_LEN - 1);

    /* Write meta locally then upload */
    char meta_local[MAX_PATH_LEN];
    snprintf(meta_local, sizeof(meta_local), "%s/%s.meta",
             temp_dir, save_id);
    save_write_meta(meta_local, &meta);

    char meta_remote[MAX_PATH_LEN];
    snprintf(meta_remote, sizeof(meta_remote), "%s/%s/meta",
             cloud_dir, save_id);
    if (!net_upload(meta_local, meta_remote))
        return SYNC_ERR_META_FAIL;

    /* 7. Clean up temp files */
    dir = opendir(temp_dir);
    if (dir) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            char fpath[MAX_PATH_LEN];
            snprintf(fpath, sizeof(fpath), "%s/%s", temp_dir, ent->d_name);
            remove(fpath);
        }
        closedir(dir);
        rmdir(temp_dir);
    }

    return SYNC_OK;
}

/*═══════════════════════════════════════════════════════════════*
 *  Download and import a save
 *═══════════════════════════════════════════════════════════════*/
SyncResult sync_download_save(const SaveFileInfo *save_info,
                              const GameTitle *game)
{
    if (!net_is_connected())
        return SYNC_ERR_NOT_CONNECTED;

    /* 1. Build cloud path for this save */
    char cloud_dir[MAX_PATH_LEN];
    save_build_cloud_path(cloud_dir, sizeof(cloud_dir),
                          save_info->device_name,
                          save_info->game_name,
                          save_info->game_id);

    /* 2. Download save files to temp dir */
    char temp_dir[MAX_PATH_LEN];
    snprintf(temp_dir, sizeof(temp_dir), "%s/import_%016llX",
             TEMP_DIR, (unsigned long long)game->title_id);
    mkdir(temp_dir, 0777);

    /* List entries in the cloud dir and try the new per-save subdirectory
     * layout first. If no matching subdirectory is found, fall back to the
     * legacy flat-file layout where files in cloud_dir are prefixed with
     * the save filename. */
    char **names = NULL;
    int count = net_list_dir(cloud_dir, &names);
    if (count <= 0)
        return SYNC_ERR_DOWNLOAD_FAIL;

    bool any_downloaded = false;

    /* 1) Look for a subdirectory named exactly after save_info->filename */
    bool found_subdir = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(names[i], save_info->filename) != 0) continue;

        found_subdir = true;
        /* List contents of the save subdirectory */
        char save_sub[MAX_PATH_LEN];
        snprintf(save_sub, sizeof(save_sub), "%s/%s", cloud_dir, names[i]);
        char **inner = NULL;
        int inner_count = net_list_dir(save_sub, &inner);
        if (inner_count > 0) {
            for (int j = 0; j < inner_count; j++) {
                /* Skip metadata files */
                if (strcmp(inner[j], "meta") == 0 || strstr(inner[j], ".meta") != NULL)
                    continue;

                char remote_file[MAX_PATH_LEN];
                snprintf(remote_file, sizeof(remote_file), "%s/%s/%s",
                         cloud_dir, names[i], inner[j]);

                char local_file[MAX_PATH_LEN];
                snprintf(local_file, sizeof(local_file), "%s/%s",
                         temp_dir, inner[j]);

                if (net_download(remote_file, local_file))
                    any_downloaded = true;
            }
        }
        net_free_names(inner, inner_count);
        break; /* only one save dir should match */
    }

    /* 2) Fallback: legacy flat-file layout where files are stored directly
     * under cloud_dir and named with a prefix including the save id. */
    if (!found_subdir) {
        for (int i = 0; i < count; i++) {
            /* Check if this file belongs to this save (contains save filename) */
            if (strstr(names[i], save_info->filename) == NULL)
                continue;
            /* Skip .meta files */
            if (strstr(names[i], ".meta") != NULL)
                continue;

            char remote_file[MAX_PATH_LEN];
            snprintf(remote_file, sizeof(remote_file), "%s/%s",
                     cloud_dir, names[i]);

            /* Extract the original save-internal filename by trimming the
             * prefix up to save_info->filename and any separators. */
            const char *orig_name = names[i];
            const char *suffix = strstr(names[i], save_info->filename);
            if (suffix) {
                suffix += strlen(save_info->filename);
                while (*suffix == '_' || *suffix == '-') suffix++;
                if (*suffix) orig_name = suffix;
            }

            char local_file[MAX_PATH_LEN];
            snprintf(local_file, sizeof(local_file), "%s/%s",
                     temp_dir, orig_name);

            if (net_download(remote_file, local_file))
                any_downloaded = true;
        }
    }

    net_free_names(names, count);

    if (!any_downloaded)
        return SYNC_ERR_DOWNLOAD_FAIL;

    /* 3. Delete the Secure Value BEFORE importing.
     *    This is critical for games like Pokémon that use anti-savescum
     *    protection. Without this, the game will reject the restored save
     *    with "save data does not match" / "corrupt" errors. */
    save_delete_secure_value(game->title_id);

    /* 4. Import ALL downloaded files into the game's save archive in one go.
     *    This opens the archive once, deletes old files, writes all new files,
     *    commits once, and closes — avoiding inconsistent states. */
    bool import_ok = save_import_directory(game->title_id, temp_dir);

    /* Clean up temp */
    {
        DIR *cdir = opendir(temp_dir);
        if (cdir) {
            struct dirent *cent;
            while ((cent = readdir(cdir)) != NULL) {
                if (cent->d_name[0] == '.') continue;
                char fpath[MAX_PATH_LEN];
                snprintf(fpath, sizeof(fpath), "%s/%s", temp_dir, cent->d_name);
                remove(fpath);
            }
            closedir(cdir);
            rmdir(temp_dir);
        }
    }

    return import_ok ? SYNC_OK : SYNC_ERR_IMPORT_FAIL;
}

/*═══════════════════════════════════════════════════════════════*
 *  Sync All
 *═══════════════════════════════════════════════════════════════*/
SyncResult sync_all(const GameTitle *games, int game_count,
                    const char *device_name,
                    const char *commit_message,
                    SyncProgressCb progress_cb)
{
    if (!net_is_connected())
        return SYNC_ERR_NOT_CONNECTED;

    SyncResult overall = SYNC_OK;

    for (int i = 0; i < game_count; i++) {
        if (progress_cb) {
            progress_cb(i + 1, game_count, games[i].name);
        }

        /* Skip games without save data */
        if (!games[i].has_save_data) continue;

        SyncResult res = sync_upload_save(
            &games[i], device_name, commit_message, NULL);

        if (res != SYNC_OK && res != SYNC_ERR_NO_SAVE_DATA) {
            overall = res; /* Record error but continue */
        }
    }

    return overall;
}

/*═══════════════════════════════════════════════════════════════*
 *  List saves from cloud for a game
 *═══════════════════════════════════════════════════════════════*/
int sync_list_saves(const GameTitle *game,
                    SaveFileInfo *out_saves, int max_saves)
{
    if (!net_is_connected()) return 0;

    /*
     * Saves are stored under multiple device names, so we need to
     * scan all device directories. For simplicity, we'll list the
     * base path first to get device names, then look in each.
     */
    char **devices = NULL;
    int dev_count = net_list_dir("/", &devices);
    if (dev_count <= 0) return 0;

    int total = 0;

    for (int d = 0; d < dev_count && total < max_saves; d++) {
        /* Build path: /DEVICE/GAMENAME/GAMEID/SAVEFILES/ */
        char save_dir[MAX_PATH_LEN];
        snprintf(save_dir, sizeof(save_dir), "/%s/%s/%016llX/SAVEFILES",
                 devices[d], game->name,
                 (unsigned long long)game->title_id);

        /* List entries in save_dir. New format stores each save in its own
         * subdirectory named by save id. We support both new (dir/meta)
         * and legacy (flat .meta files) layouts. */
        char **files = NULL;
        int file_count = net_list_dir(save_dir, &files);
        if (file_count <= 0) continue;

        for (int f = 0; f < file_count && total < max_saves; f++) {
            char entry_path[MAX_PATH_LEN];
            snprintf(entry_path, sizeof(entry_path), "%s/%s", save_dir, files[f]);

            /* Try listing inside the entry -> indicates a subdirectory */
            char **inner = NULL;
            int inner_count = net_list_dir(entry_path, &inner);
            if (inner_count > 0) {
                /* Search for a metadata file inside the subdirectory */
                for (int j = 0; j < inner_count && total < max_saves; j++) {
                    if (strcmp(inner[j], "meta") == 0 || strstr(inner[j], ".meta") != NULL) {
                        char remote_meta[MAX_PATH_LEN];
                        snprintf(remote_meta, sizeof(remote_meta), "%s/%s/%s",
                                 save_dir, files[f], inner[j]);

                        char local_meta[MAX_PATH_LEN];
                        snprintf(local_meta, sizeof(local_meta), "%s/meta_tmp_%d.meta",
                                 TEMP_DIR, total);

                        if (net_download(remote_meta, local_meta)) {
                            if (save_parse_meta(local_meta, &out_saves[total])) {
                                total++;
                            }
                            remove(local_meta);
                        }
                        break; /* one meta per save dir */
                    }
                }
                net_free_names(inner, inner_count);
                continue;
            }

            /* Legacy: entry is a file in save_dir -> check for .meta */
            if (strstr(files[f], ".meta") != NULL) {
                char remote_meta[MAX_PATH_LEN];
                snprintf(remote_meta, sizeof(remote_meta), "%s/%s",
                         save_dir, files[f]);

                char local_meta[MAX_PATH_LEN];
                snprintf(local_meta, sizeof(local_meta), "%s/meta_tmp_%d.meta",
                         TEMP_DIR, total);

                if (net_download(remote_meta, local_meta)) {
                    if (save_parse_meta(local_meta, &out_saves[total])) {
                        total++;
                    }
                    remove(local_meta);
                }
            }
        }

        net_free_names(files, file_count);
    }

    net_free_names(devices, dev_count);

    /* Sort by creation time (newest first) */
    for (int i = 0; i < total - 1; i++) {
        for (int j = i + 1; j < total; j++) {
            if (out_saves[j].created_at > out_saves[i].created_at) {
                SaveFileInfo tmp = out_saves[i];
                out_saves[i] = out_saves[j];
                out_saves[j] = tmp;
            }
        }
    }

    return total;
}

/*═══════════════════════════════════════════════════════════════*
 *  Delete a save from the cloud
 *═══════════════════════════════════════════════════════════════*/
SyncResult sync_delete_save(const SaveFileInfo *save_info)
{
    if (!net_is_connected())
        return SYNC_ERR_NOT_CONNECTED;

    /* Build cloud directory path */
    char cloud_dir[MAX_PATH_LEN];
    save_build_cloud_path(cloud_dir, sizeof(cloud_dir),
                          save_info->device_name,
                          save_info->game_name,
                          save_info->game_id);

    /* List all files in the save directory */
    char **names = NULL;
    int count = net_list_dir(cloud_dir, &names);
    if (count <= 0)
        return SYNC_ERR_GENERIC;

    bool any_deleted = false;

    for (int i = 0; i < count; i++) {
        /* If the entry exactly equals the save id, treat it as a
         * subdirectory: delete its contents then remove the dir. */
        if (strcmp(names[i], save_info->filename) == 0) {
            char save_sub[MAX_PATH_LEN];
            snprintf(save_sub, sizeof(save_sub), "%s/%s", cloud_dir, names[i]);

            /* List and delete inner files */
            char **inner = NULL;
            int inner_count = net_list_dir(save_sub, &inner);
            if (inner_count > 0) {
                for (int j = 0; j < inner_count; j++) {
                    char remote_inner[MAX_PATH_LEN];
                    snprintf(remote_inner, sizeof(remote_inner), "%s/%s/%s",
                             cloud_dir, names[i], inner[j]);
                    net_delete(remote_inner);
                }
            }
            net_free_names(inner, inner_count);

            /* Now delete the directory itself (server will rmdir if empty) */
            char remote_dir[MAX_PATH_LEN];
            snprintf(remote_dir, sizeof(remote_dir), "%s/%s",
                     cloud_dir, names[i]);
            net_delete(remote_dir);
            any_deleted = true;
            continue;
        }

        /* Legacy: delete files that contain the save filename prefix */
        if (strstr(names[i], save_info->filename) != NULL) {
            char remote_file[MAX_PATH_LEN];
            snprintf(remote_file, sizeof(remote_file), "%s/%s",
                     cloud_dir, names[i]);
            net_delete(remote_file);
            any_deleted = true;
        }
    }
    net_free_names(names, count);

    return any_deleted ? SYNC_OK : SYNC_ERR_GENERIC;
}
