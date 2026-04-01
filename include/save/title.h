#ifndef CLOUDSAVER_TITLE_H
#define CLOUDSAVER_TITLE_H

#include "common.h"

/* Scan all installed titles into games array. Returns count. */
int save_scan_titles(GameTitle *games, int max_games);

/* Get raw title ID list (caller frees *out_ids). Returns count. */
u32 save_get_title_ids(u64 **out_ids);

/* Scan a single title. Returns true if valid game, false if skip. */
bool save_scan_one_title(u64 title_id, GameTitle *out);

#endif /* CLOUDSAVER_TITLE_H */
