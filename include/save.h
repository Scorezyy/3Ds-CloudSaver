/**
 * 3DS CloudSaver - Save Manager
 * Handles reading installed titles and extracting/importing save data.
 */

#ifndef CLOUDSAVER_SAVE_H
#define CLOUDSAVER_SAVE_H

#include "common.h"

/** Scan all installed titles and populate the games array. */
int save_scan_titles(GameTitle *games, int max_games);

/** Check if a title has accessible save data. */
bool save_has_data(u64 title_id);

/** Extract the save data for a title into a buffer.
 *  Returns the size of the extracted data, or -1 on error. */
int save_extract(u64 title_id, const char *out_path);

/** Import (write) save data from a file into a title's save slot. */
bool save_import(u64 title_id, const char *in_path);

/** Import ALL files from a directory into a title's save archive.
 *  Opens the archive once, writes all files, commits once, closes.
 *  This is the correct way to restore multi-file saves. */
bool save_import_directory(u64 title_id, const char *dir_path);

/** Delete the Secure Value for a title (anti-savescum counter).
 *  MUST be called before restoring a save, otherwise games like
 *  Pokémon will reject the restored save as "corrupt".
 *  Returns true on success or if no secure value existed. */
bool save_delete_secure_value(u64 title_id);

/** Get the region of a title from its ID. */
GameRegion save_get_region(u64 title_id);

/** Get the SMDH icon for a title. Writes into a C2D_Image. */
bool save_get_icon(u64 title_id, C2D_Image *out_icon);

/** Get a friendly name for a title from its SMDH. */
bool save_get_name(u64 title_id, char *out_name, size_t max_len);

/** Build the cloud path for a save:
 *  /DEVICENAME/GAMENAME/GAMEID/SAVEFILES/
 */
void save_build_cloud_path(char *out, size_t max_len,
                           const char *device_name,
                           const char *game_name,
                           u64 game_id);

/** Build a save filename with timestamp. */
void save_build_filename(char *out, size_t max_len,
                         const char *device_name, time_t timestamp);

/** Parse a SaveFileInfo from a metadata JSON file. */
bool save_parse_meta(const char *json_path, SaveFileInfo *out);

/** Write a SaveFileInfo as metadata JSON. */
bool save_write_meta(const char *json_path, const SaveFileInfo *info);

/** Free GPU textures allocated for game icons. */
void save_free_icons(GameTitle *games, int count);

#endif /* CLOUDSAVER_SAVE_H */
