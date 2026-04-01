#ifndef CLOUDSAVER_SAVE_H
#define CLOUDSAVER_SAVE_H

#include "common.h"
#include "smdh.h"
#include "title.h"

/* Save archive operations */
bool save_has_data(u64 title_id);
int  save_extract(u64 title_id, const char *out_path);
bool save_import(u64 title_id, const char *in_path);
bool save_import_directory(u64 title_id, const char *dir_path);
bool save_delete_secure_value(u64 title_id);

/* Cloud path helpers */
void save_build_cloud_path(char *out, size_t max_len,
                           const char *device_name,
                           const char *game_name,
                           u64 game_id);

void save_build_filename(char *out, size_t max_len,
                         const char *device_name, time_t timestamp);

/* Metadata (key=value format) */
bool save_parse_meta(const char *meta_path, SaveFileInfo *out);
bool save_write_meta(const char *meta_path, const SaveFileInfo *info);

#endif /* CLOUDSAVER_SAVE_H */
