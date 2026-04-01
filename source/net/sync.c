/*
 * sync.c — Orchestrates save upload, download, list, and delete.
 * Cloud path: /DEVICE/GAME/GAMEID/SAVEFILES/<save_id>/<files>
 */

#include "sync.h"
#include "save.h"
#include "network.h"
#include "config.h"

#include <unistd.h>

/* ── Error strings ── */

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

/* ── Helpers ── */

static void temp_cleanup(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        char fpath[MAX_PATH_LEN];
        snprintf(fpath, sizeof(fpath), "%s/%s", dir_path, ent->d_name);
        remove(fpath);
    }
    closedir(dir);
    rmdir(dir_path);
}

static void build_save_id(char *out, size_t max)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (tm)
        snprintf(out, max, "%04d%02d%02d_%02d%02d%02d",
                 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
    else
        snprintf(out, max, "%lld", (long long)now);
}

static bool is_meta_file(const char *name)
{
    return strcmp(name, "meta") == 0 || strstr(name, ".meta") != NULL;
}

/* Download + parse a remote meta file. Returns true on success. */
static bool fetch_remote_meta(const char *remote_path, int index,
                              SaveFileInfo *out)
{
    char local_meta[MAX_PATH_LEN];
    snprintf(local_meta, sizeof(local_meta), "%s/meta_tmp_%d.meta",
             TEMP_DIR, index);

    if (!net_download(remote_path, local_meta))
        return false;

    bool ok = save_parse_meta(local_meta, out);
    remove(local_meta);
    return ok;
}

/* Download all non-meta files from a remote dir into a local dir. */
static bool download_dir_files(const char *remote_dir, const char *local_dir)
{
    char **entries = NULL;
    int count = net_list_dir(remote_dir, &entries);
    if (count <= 0) return false;

    bool any = false;
    for (int i = 0; i < count; i++) {
        if (is_meta_file(entries[i])) continue;

        char remote_file[MAX_PATH_LEN], local_file[MAX_PATH_LEN];
        snprintf(remote_file, sizeof(remote_file), "%s/%s", remote_dir, entries[i]);
        snprintf(local_file,  sizeof(local_file),  "%s/%s", local_dir,  entries[i]);

        if (net_download(remote_file, local_file))
            any = true;
    }
    net_free_names(entries, count);
    return any;
}

/* Upload all files from local dir to remote dir. Returns total bytes or 0 on failure. */
static size_t upload_dir_files(const char *local_dir, const char *remote_dir)
{
    DIR *dir = opendir(local_dir);
    if (!dir) return 0;

    struct dirent *ent;
    size_t total = 0;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char local_file[MAX_PATH_LEN], remote_file[MAX_PATH_LEN];
        snprintf(local_file,  sizeof(local_file),  "%s/%s", local_dir,  ent->d_name);
        snprintf(remote_file, sizeof(remote_file), "%s/%s", remote_dir, ent->d_name);

        if (!net_upload(local_file, remote_file)) {
            closedir(dir);
            return 0;
        }

        struct stat st;
        if (stat(local_file, &st) == 0)
            total += (size_t)st.st_size;
    }
    closedir(dir);
    return total;
}

/* Delete all files in a remote dir, then the dir itself. */
static void delete_remote_dir(const char *remote_dir)
{
    char **entries = NULL;
    int count = net_list_dir(remote_dir, &entries);
    if (count > 0) {
        for (int i = 0; i < count; i++) {
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path), "%s/%s", remote_dir, entries[i]);
            net_delete(path);
        }
    }
    net_free_names(entries, count);
    net_delete(remote_dir);
}

/* ── Upload save ── */

SyncResult sync_upload_save(const GameTitle *game,
                            const char *device_name,
                            const char *commit_message,
                            const char *description)
{
    if (!net_is_connected()) return SYNC_ERR_NOT_CONNECTED;

    /* Extract to temp */
    char temp_dir[MAX_PATH_LEN];
    snprintf(temp_dir, sizeof(temp_dir), "%s/%016llX",
             TEMP_DIR, (unsigned long long)game->title_id);

    int extract_size = save_extract(game->title_id, temp_dir);
    if (extract_size < 0)  { temp_cleanup(temp_dir); return SYNC_ERR_NO_SAVE_DATA; }
    if (extract_size == 0) { temp_cleanup(temp_dir); return SYNC_ERR_EXTRACT_FAIL; }

    /* Create remote dir structure */
    char cloud_dir[MAX_PATH_LEN];
    save_build_cloud_path(cloud_dir, sizeof(cloud_dir),
                          device_name, game->name, game->title_id);
    if (!net_mkdir(cloud_dir)) { temp_cleanup(temp_dir); return SYNC_ERR_MKDIR_FAIL; }

    /* Build save id + upload all extracted files */
    char save_id[64];
    build_save_id(save_id, sizeof(save_id));

    char remote_save_dir[MAX_PATH_LEN];
    snprintf(remote_save_dir, sizeof(remote_save_dir), "%s/%s", cloud_dir, save_id);

    size_t total_size = upload_dir_files(temp_dir, remote_save_dir);
    if (total_size == 0) { temp_cleanup(temp_dir); return SYNC_ERR_UPLOAD_FAIL; }

    /* Write + upload metadata */
    SaveFileInfo meta = {0};
    strncpy(meta.filename, save_id, MAX_PATH_LEN - 1);
    strncpy(meta.device_name, device_name, MAX_DEVICE_NAME - 1);
    strncpy(meta.game_name, game->name, 63);
    meta.game_id     = game->title_id;
    meta.created_at  = time(NULL);
    meta.modified_at = meta.created_at;
    meta.file_size   = total_size;
    meta.region      = game->region;
    if (commit_message) strncpy(meta.commit_message, commit_message, MAX_COMMIT_MSG_LEN - 1);
    if (description)    strncpy(meta.description,    description,    MAX_DESCRIPTION_LEN - 1);

    char meta_local[MAX_PATH_LEN];
    snprintf(meta_local, sizeof(meta_local), "%s/%s.meta", temp_dir, save_id);
    save_write_meta(meta_local, &meta);

    char meta_remote[MAX_PATH_LEN];
    snprintf(meta_remote, sizeof(meta_remote), "%s/meta", remote_save_dir);
    bool meta_ok = net_upload(meta_local, meta_remote);

    temp_cleanup(temp_dir);
    return meta_ok ? SYNC_OK : SYNC_ERR_META_FAIL;
}

/* ── Download legacy flat-file layout (fallback) ── */

static bool download_legacy_files(const char *cloud_dir, const char *temp_dir,
                                  const char *save_filename)
{
    char **names = NULL;
    int count = net_list_dir(cloud_dir, &names);
    if (count <= 0) return false;

    bool any = false;
    for (int i = 0; i < count; i++) {
        if (!strstr(names[i], save_filename)) continue;
        if (is_meta_file(names[i])) continue;

        char remote_file[MAX_PATH_LEN];
        snprintf(remote_file, sizeof(remote_file), "%s/%s", cloud_dir, names[i]);

        /* Strip prefix to get original filename */
        const char *orig = names[i];
        const char *s = strstr(names[i], save_filename);
        if (s) {
            s += strlen(save_filename);
            while (*s == '_' || *s == '-') s++;
            if (*s) orig = s;
        }

        char local_file[MAX_PATH_LEN];
        snprintf(local_file, sizeof(local_file), "%s/%s", temp_dir, orig);
        if (net_download(remote_file, local_file)) any = true;
    }
    net_free_names(names, count);
    return any;
}

/* ── Download + import save ── */

SyncResult sync_download_save(const SaveFileInfo *save_info,
                              const GameTitle *game)
{
    if (!net_is_connected()) return SYNC_ERR_NOT_CONNECTED;

    char cloud_dir[MAX_PATH_LEN];
    save_build_cloud_path(cloud_dir, sizeof(cloud_dir),
                          save_info->device_name,
                          save_info->game_name,
                          save_info->game_id);

    char temp_dir[MAX_PATH_LEN];
    snprintf(temp_dir, sizeof(temp_dir), "%s/import_%016llX",
             TEMP_DIR, (unsigned long long)game->title_id);
    mkdir(temp_dir, 0777);

    /* Try new layout (subdirectory), fallback to legacy (flat) */
    char save_sub[MAX_PATH_LEN];
    snprintf(save_sub, sizeof(save_sub), "%s/%s", cloud_dir, save_info->filename);

    bool any = download_dir_files(save_sub, temp_dir);
    if (!any)
        any = download_legacy_files(cloud_dir, temp_dir, save_info->filename);

    if (!any) {
        temp_cleanup(temp_dir);
        return SYNC_ERR_DOWNLOAD_FAIL;
    }

    save_delete_secure_value(game->title_id);
    bool ok = save_import_directory(game->title_id, temp_dir);
    temp_cleanup(temp_dir);
    return ok ? SYNC_OK : SYNC_ERR_IMPORT_FAIL;
}

/* ── Sync all games ── */

SyncResult sync_all(const GameTitle *games, int game_count,
                    const char *device_name,
                    const char *commit_message,
                    SyncProgressCb progress_cb)
{
    if (!net_is_connected()) return SYNC_ERR_NOT_CONNECTED;

    SyncResult overall = SYNC_OK;
    for (int i = 0; i < game_count; i++) {
        if (progress_cb) progress_cb(i + 1, game_count, games[i].name);
        if (!games[i].has_save_data) continue;

        SyncResult res = sync_upload_save(&games[i], device_name, commit_message, NULL);
        if (res != SYNC_OK && res != SYNC_ERR_NO_SAVE_DATA)
            overall = res;
    }
    return overall;
}

/* ── List saves from cloud ── */

/* Scan entries in a single SAVEFILES directory. */
static int scan_save_dir(const char *save_dir, SaveFileInfo *out,
                         int max, int current)
{
    char **files = NULL;
    int count = net_list_dir(save_dir, &files);
    if (count <= 0) return 0;

    int found = 0;
    for (int f = 0; f < count && (current + found) < max; f++) {
        char entry[MAX_PATH_LEN];
        snprintf(entry, sizeof(entry), "%s/%s", save_dir, files[f]);

        /* Try as subdirectory (new layout) */
        char **inner = NULL;
        int inner_count = net_list_dir(entry, &inner);
        if (inner_count > 0) {
            for (int j = 0; j < inner_count; j++) {
                if (!is_meta_file(inner[j])) continue;
                char meta[MAX_PATH_LEN];
                snprintf(meta, sizeof(meta), "%s/%s", entry, inner[j]);
                if (fetch_remote_meta(meta, current + found, &out[current + found]))
                    found++;
                break;
            }
            net_free_names(inner, inner_count);
            continue;
        }

        /* Legacy: flat .meta file */
        if (is_meta_file(files[f])) {
            if (fetch_remote_meta(entry, current + found, &out[current + found]))
                found++;
        }
    }

    net_free_names(files, count);
    return found;
}

int sync_list_saves(const GameTitle *game,
                    SaveFileInfo *out_saves, int max_saves)
{
    if (!net_is_connected()) return 0;

    char **devices = NULL;
    int dev_count = net_list_dir("/", &devices);
    if (dev_count <= 0) return 0;

    int total = 0;
    for (int d = 0; d < dev_count && total < max_saves; d++) {
        char save_dir[MAX_PATH_LEN];
        snprintf(save_dir, sizeof(save_dir), "/%s/%s/%016llX/SAVEFILES",
                 devices[d], game->name, (unsigned long long)game->title_id);
        total += scan_save_dir(save_dir, out_saves, max_saves, total);
    }
    net_free_names(devices, dev_count);

    /* Sort newest first */
    for (int i = 1; i < total; i++) {
        SaveFileInfo tmp = out_saves[i];
        int j = i - 1;
        while (j >= 0 && out_saves[j].created_at < tmp.created_at) {
            out_saves[j + 1] = out_saves[j];
            j--;
        }
        out_saves[j + 1] = tmp;
    }
    return total;
}

/* ── Delete save from cloud ── */

SyncResult sync_delete_save(const SaveFileInfo *save_info)
{
    if (!net_is_connected()) return SYNC_ERR_NOT_CONNECTED;

    char cloud_dir[MAX_PATH_LEN];
    save_build_cloud_path(cloud_dir, sizeof(cloud_dir),
                          save_info->device_name,
                          save_info->game_name,
                          save_info->game_id);

    char **names = NULL;
    int count = net_list_dir(cloud_dir, &names);
    if (count <= 0) return SYNC_ERR_GENERIC;

    bool any = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(names[i], save_info->filename) == 0) {
            char sub[MAX_PATH_LEN];
            snprintf(sub, sizeof(sub), "%s/%s", cloud_dir, names[i]);
            delete_remote_dir(sub);
            any = true;
        } else if (strstr(names[i], save_info->filename)) {
            char path[MAX_PATH_LEN];
            snprintf(path, sizeof(path), "%s/%s", cloud_dir, names[i]);
            net_delete(path);
            any = true;
        }
    }
    net_free_names(names, count);
    return any ? SYNC_OK : SYNC_ERR_GENERIC;
}
