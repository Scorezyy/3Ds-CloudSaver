#include "save.h"
#include "config.h"
#include <malloc.h>

/* ── Shared helpers ── */

/* Build a UTF-16 archive path "/<filename>" from an ASCII filename.
 * Caller must free the returned pointer. Returns NULL on failure. */
static u16 *build_u16_path(const char *fname, int *out_len)
{
    int name_len = strlen(fname);
    int path_len = 1 + name_len;          /* '/' + name */
    u16 *p = (u16 *)calloc(path_len + 1, sizeof(u16));
    if (!p) return NULL;

    p[0] = '/';
    for (int i = 0; i < name_len; i++)
        p[1 + i] = (u16)(unsigned char)fname[i];
    p[path_len] = 0;

    if (out_len) *out_len = path_len;
    return p;
}

/* Read a local file into a malloc'd buffer. Returns NULL on failure. */
static u8 *read_local_file(const char *path, long *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) { fclose(f); return NULL; }

    u8 *buf = (u8 *)malloc(sz);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, sz, f);
    fclose(f);

    *out_size = sz;
    return buf;
}

/* Write data into an archive file (delete → create → open → write → close). */
static bool write_to_archive(FS_Archive archive, const char *fname,
                             const u8 *data, u32 size)
{
    int path_len;
    u16 *u16p = build_u16_path(fname, &path_len);
    if (!u16p) return false;

    FS_Path fpath = { PATH_UTF16, (path_len + 1) * 2, u16p };

    FSUSER_DeleteFile(archive, fpath);
    FSUSER_CreateFile(archive, fpath, 0, (u64)size);

    Handle fh;
    Result res = FSUSER_OpenFile(&fh, archive, fpath, FS_OPEN_WRITE, 0);
    if (R_FAILED(res))
        res = FSUSER_OpenFile(&fh, archive, fpath,
                              FS_OPEN_WRITE | FS_OPEN_CREATE, 0);
    if (R_FAILED(res)) { free(u16p); return false; }

    u32 written = 0;
    FSFILE_Write(fh, &written, 0, data, size, FS_WRITE_FLUSH);
    FSFILE_Close(fh);
    svcCloseHandle(fh);
    free(u16p);

    return written == size;
}

/* ── Open save archive (tries multiple methods for CFW compatibility) ── */

static Result open_save_archive(FS_Archive *archive, u64 title_id)
{
    u32 path_low  = (u32)(title_id & 0xFFFFFFFF);
    u32 path_high = (u32)(title_id >> 32);

    static const u32 archive_ids[] = {
        ARCHIVE_USER_SAVEDATA, 0x0000006B, 0x2345678A
    };
    static const int path_sizes[] = { 3, 3, 4 };

    for (int m = 0; m < 3; m++) {
        u32 path_data[4] = { MEDIATYPE_SD, path_low, path_high, 0 };
        FS_Path path = { PATH_BINARY, path_sizes[m] * sizeof(u32), path_data };
        Result res = FSUSER_OpenArchive(archive, archive_ids[m], path);
        if (R_SUCCEEDED(res)) return res;
    }

    return -1; /* all methods failed */
}

/* ── Check if title has save data ── */

bool save_has_data(u64 title_id)
{
    FS_Archive archive;
    Result res = open_save_archive(&archive, title_id);
    if (R_SUCCEEDED(res)) {
        FSUSER_CloseArchive(archive);
        return true;
    }
    return false;
}

/* ── Recursive directory extraction ── */

static int extract_dir(FS_Archive archive, const u16 *dir_utf16,
                       int dir_utf16_len, const char *out_path)
{
    Handle dir_handle;
    FS_Path dir_path = { PATH_UTF16, dir_utf16_len * 2, dir_utf16 };

    Result res = FSUSER_OpenDirectory(&dir_handle, archive, dir_path);
    if (R_FAILED(res)) return 0;

    mkdir(out_path, 0777);

    int total_size = 0;
    FS_DirectoryEntry entries[16];
    u32 entries_read = 0;

    do {
        res = FSDIR_Read(dir_handle, &entries_read, 16, entries);
        if (R_FAILED(res)) break;

        for (u32 i = 0; i < entries_read; i++) {
            char utf8_name[256] = {0};
            int j;
            for (j = 0; j < 255 && entries[i].name[j]; j++) {
                u16 c = entries[i].name[j];
                utf8_name[j] = (c < 0x80) ? (char)c : '_';
            }
            utf8_name[j] = '\0';

            if (entries[i].attributes & FS_ATTRIBUTE_DIRECTORY) {
                char sub_out[MAX_PATH_LEN];
                snprintf(sub_out, sizeof(sub_out), "%s/%s", out_path, utf8_name);

                int name_len = 0;
                while (entries[i].name[name_len]) name_len++;

                int sub_len = dir_utf16_len + name_len + 1;
                u16 *sub_utf16 = (u16 *)calloc(sub_len + 1, sizeof(u16));
                if (!sub_utf16) continue;

                memcpy(sub_utf16, dir_utf16, (dir_utf16_len - 1) * 2);
                int pos = dir_utf16_len - 1;
                memcpy(&sub_utf16[pos], entries[i].name, name_len * 2);
                pos += name_len;
                sub_utf16[pos++] = '/';
                sub_utf16[pos] = 0;

                total_size += extract_dir(archive, sub_utf16, pos + 1, sub_out);
                free(sub_utf16);
                continue;
            }

            char file_out[MAX_PATH_LEN];
            snprintf(file_out, sizeof(file_out), "%s/%s", out_path, utf8_name);

            Handle file_handle;
            int name_len = 0;
            while (entries[i].name[name_len]) name_len++;

            int full_len = dir_utf16_len - 1 + name_len + 1;
            u16 *full_path = (u16 *)calloc(full_len, sizeof(u16));
            if (!full_path) continue;

            memcpy(full_path, dir_utf16, (dir_utf16_len - 1) * 2);
            int fpos = dir_utf16_len - 1;
            memcpy(&full_path[fpos], entries[i].name, name_len * 2);
            fpos += name_len;
            full_path[fpos] = 0;

            FS_Path fpath = { PATH_UTF16, (fpos + 1) * 2, full_path };

            if (R_SUCCEEDED(FSUSER_OpenFile(&file_handle, archive, fpath,
                                            FS_OPEN_READ, 0)))
            {
                u64 file_size;
                FSFILE_GetSize(file_handle, &file_size);

                if (file_size > 0) {
                    u8 *buf = (u8 *)malloc((size_t)file_size);
                    if (buf) {
                        u32 bytes_read = 0;
                        FSFILE_Read(file_handle, &bytes_read, 0, buf, (u32)file_size);

                        FILE *fout = fopen(file_out, "wb");
                        if (fout) {
                            fwrite(buf, 1, bytes_read, fout);
                            fclose(fout);
                            total_size += bytes_read;
                        }
                        free(buf);
                    }
                }
                FSFILE_Close(file_handle);
                svcCloseHandle(file_handle);
            }
            free(full_path);
        }
    } while (entries_read > 0);

    FSDIR_Close(dir_handle);
    svcCloseHandle(dir_handle);
    return total_size;
}

/* ── Extract save data to directory ── */

int save_extract(u64 title_id, const char *out_path)
{
    FS_Archive archive;
    Result res = open_save_archive(&archive, title_id);
    if (R_FAILED(res)) return -1;

    mkdir(out_path, 0777);

    static const u16 root_path[] = { '/', 0 };
    int total_size = extract_dir(archive, root_path, 2, out_path);

    FSUSER_CloseArchive(archive);
    return total_size;
}

/* ── Import single file into save archive ── */

bool save_import(u64 title_id, const char *in_path)
{
    FS_Archive archive;
    if (R_FAILED(open_save_archive(&archive, title_id))) return false;

    long file_size;
    u8 *buf = read_local_file(in_path, &file_size);
    if (!buf) { FSUSER_CloseArchive(archive); return false; }

    const char *fname = strrchr(in_path, '/');
    fname = fname ? fname + 1 : in_path;

    bool ok = write_to_archive(archive, fname, buf, (u32)file_size);
    free(buf);

    FSUSER_ControlArchive(archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
    FSUSER_CloseArchive(archive);
    return ok;
}

/* ── Import all files from directory (single commit) ── */

bool save_import_directory(u64 title_id, const char *dir_path)
{
    FS_Archive archive;
    if (R_FAILED(open_save_archive(&archive, title_id))) return false;

    /* Delete existing files in save root */
    {
        Handle dir_handle;
        static const u16 root_utf16[] = { '/', 0 };
        FS_Path root_path = { PATH_UTF16, 4, root_utf16 };

        if (R_SUCCEEDED(FSUSER_OpenDirectory(&dir_handle, archive, root_path))) {
            FS_DirectoryEntry del_entries[16];
            u32 del_read = 0;
            do {
                Result res = FSDIR_Read(dir_handle, &del_read, 16, del_entries);
                if (R_FAILED(res)) break;
                for (u32 i = 0; i < del_read; i++) {
                    if (del_entries[i].attributes & FS_ATTRIBUTE_DIRECTORY)
                        continue;

                    int nlen = 0;
                    while (del_entries[i].name[nlen]) nlen++;

                    int plen = 1 + nlen;
                    u16 *dpath = (u16 *)calloc(plen + 1, sizeof(u16));
                    if (!dpath) continue;

                    dpath[0] = '/';
                    memcpy(&dpath[1], del_entries[i].name, nlen * 2);
                    dpath[plen] = 0;

                    FS_Path fp = { PATH_UTF16, (plen + 1) * 2, dpath };
                    FSUSER_DeleteFile(archive, fp);
                    free(dpath);
                }
            } while (del_read > 0);
            FSDIR_Close(dir_handle);
            svcCloseHandle(dir_handle);
        }
    }

    /* Write all files from directory using shared helpers */
    DIR *dir = opendir(dir_path);
    if (!dir) { FSUSER_CloseArchive(archive); return false; }

    struct dirent *ent;
    bool all_ok = true;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char local_path[MAX_PATH_LEN];
        snprintf(local_path, sizeof(local_path), "%s/%s", dir_path, ent->d_name);

        long file_size;
        u8 *buf = read_local_file(local_path, &file_size);
        if (!buf) { all_ok = false; continue; }

        if (!write_to_archive(archive, ent->d_name, buf, (u32)file_size))
            all_ok = false;
        free(buf);
    }
    closedir(dir);

    FSUSER_ControlArchive(archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
    FSUSER_CloseArchive(archive);
    return all_ok;
}

/* ── Delete Secure Value (JKSM approach) ── */

bool save_delete_secure_value(u64 title_id)
{
    u32 lower_id  = (u32)(title_id & 0xFFFFFFFF);
    u32 unique_id = lower_id >> 8;

    u64 input = ((u64)SECUREVALUE_SLOT_SD << 32) | ((u64)unique_id << 8);
    u8 output = 0;

    Result res = FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE,
                                          &input, sizeof(u64),
                                          &output, sizeof(u8));
    return R_SUCCEEDED(res) || true;
}

/* ── Path builders ── */

void save_build_cloud_path(char *out, size_t max_len,
                           const char *device_name,
                           const char *game_name,
                           u64 game_id)
{
    snprintf(out, max_len, "/%s/%s/%016llX/SAVEFILES",
             device_name, game_name, (unsigned long long)game_id);
}

void save_build_filename(char *out, size_t max_len,
                         const char *device_name, time_t timestamp)
{
    struct tm *tm = localtime(&timestamp);
    if (tm) {
        snprintf(out, max_len, "%s_%04d%02d%02d_%02d%02d%02d.sav",
                 device_name,
                 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
    } else {
        snprintf(out, max_len, "%s_%lld.sav",
                 device_name, (long long)timestamp);
    }
}

/* ── Metadata (key=value format) ── */

bool save_write_meta(const char *meta_path, const SaveFileInfo *info)
{
    FILE *f = fopen(meta_path, "w");
    if (!f) return false;

    fprintf(f, "filename=%s\n", info->filename);
    fprintf(f, "device=%s\n", info->device_name);
    fprintf(f, "game=%s\n", info->game_name);
    fprintf(f, "game_id=%016llX\n", (unsigned long long)info->game_id);
    fprintf(f, "description=%s\n", info->description);
    fprintf(f, "commit=%s\n", info->commit_message);
    fprintf(f, "created=%lld\n", (long long)info->created_at);
    fprintf(f, "modified=%lld\n", (long long)info->modified_at);
    fprintf(f, "size=%zu\n", info->file_size);
    fprintf(f, "region=%d\n", (int)info->region);

    fclose(f);
    return true;
}

bool save_parse_meta(const char *meta_path, SaveFileInfo *out)
{
    FILE *f = fopen(meta_path, "r");
    if (!f) return false;

    memset(out, 0, sizeof(SaveFileInfo));

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *key = line;
        const char *val = eq + 1;

        if (strcmp(key, "filename") == 0)
            strncpy(out->filename, val, MAX_PATH_LEN - 1);
        else if (strcmp(key, "device") == 0)
            strncpy(out->device_name, val, MAX_DEVICE_NAME - 1);
        else if (strcmp(key, "game") == 0)
            strncpy(out->game_name, val, 63);
        else if (strcmp(key, "game_id") == 0)
            out->game_id = strtoull(val, NULL, 16);
        else if (strcmp(key, "description") == 0)
            strncpy(out->description, val, MAX_DESCRIPTION_LEN - 1);
        else if (strcmp(key, "commit") == 0)
            strncpy(out->commit_message, val, MAX_COMMIT_MSG_LEN - 1);
        else if (strcmp(key, "created") == 0)
            out->created_at = (time_t)atoll(val);
        else if (strcmp(key, "modified") == 0)
            out->modified_at = (time_t)atoll(val);
        else if (strcmp(key, "size") == 0)
            out->file_size = (size_t)atoll(val);
        else if (strcmp(key, "region") == 0)
            out->region = (GameRegion)atoi(val);
    }

    fclose(f);
    return true;
}
