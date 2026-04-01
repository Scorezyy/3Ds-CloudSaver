#include "title.h"
#include "smdh.h"
#include <malloc.h>

/* ── Shared helper: populate a GameTitle from title ID + SMDH ── */

static bool populate_game(u64 tid, GameTitle *g)
{
    static SMDH smdh;
    static const int lang_order[] = { 1, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    u32 high = (u32)(tid >> 32);
    if (high != 0x00040000 && high != 0x00040002)
        return false;

    memset(g, 0, sizeof(GameTitle));
    g->title_id      = tid;
    g->has_save_data = true;
    g->region        = REGION_UNKNOWN;
    g->media_type    = MEDIA_SD;
    g->is_cartridge  = false;

    snprintf(g->name, sizeof(g->name), "%08lX",
             (unsigned long)(tid & 0xFFFFFFFF));

    char product_code[16] = {0};
    if (R_SUCCEEDED(AM_GetTitleProductCode(MEDIATYPE_SD, tid, product_code)))
        strncpy(g->product_code, product_code, sizeof(g->product_code) - 1);

    memset(&smdh, 0, sizeof(smdh));
    if (!smdh_read(tid, &smdh))
        return false;

    bool is_official =
        (g->product_code[0] == 'C' && g->product_code[1] == 'T' &&
         g->product_code[2] == 'R' && g->product_code[3] == '-') ||
        (g->product_code[0] == 'K' && g->product_code[1] == 'T' &&
         g->product_code[2] == 'R' && g->product_code[3] == '-');
    if (!is_official)
        return false;

    char name_buf[128] = {0};
    for (int l = 0; l < 12; l++) {
        int idx = lang_order[l];
        if (idx >= 16) break;
        smdh_utf16_to_utf8(smdh.titles[idx].short_desc, name_buf, sizeof(name_buf));
        if (name_buf[0] != '\0') break;
    }
    if (name_buf[0] != '\0')
        strncpy(g->name, name_buf, sizeof(g->name) - 1);

    g->region = region_from_smdh(smdh.region_lockout);
    if (g->region == REGION_UNKNOWN)
        g->region = region_from_product_code(g->product_code);

    if (smdh_icon_to_c2d(smdh.large_icon, &g->icon))
        g->has_icon = true;

    return true;
}

/* ── Scan all installed titles ── */

int save_scan_titles(GameTitle *games, int max_games)
{
    u32 title_count = 0;
    if (R_FAILED(AM_GetTitleCount(MEDIATYPE_SD, &title_count)) || title_count == 0)
        return 0;

    u64 *title_ids = (u64 *)malloc(title_count * sizeof(u64));
    if (!title_ids) return 0;

    u32 read_count = 0;
    if (R_FAILED(AM_GetTitleList(&read_count, MEDIATYPE_SD, title_count, title_ids))) {
        free(title_ids);
        return 0;
    }

    int count = 0;
    for (u32 i = 0; i < read_count && count < max_games; i++) {
        if (populate_game(title_ids[i], &games[count]))
            count++;
    }

    free(title_ids);
    return count;
}

/* ── Get raw title ID list ── */

u32 save_get_title_ids(u64 **out_ids)
{
    u32 title_count = 0;
    if (R_FAILED(AM_GetTitleCount(MEDIATYPE_SD, &title_count)) || title_count == 0) {
        *out_ids = NULL;
        return 0;
    }

    u64 *ids = (u64 *)malloc(title_count * sizeof(u64));
    if (!ids) { *out_ids = NULL; return 0; }

    u32 read_count = 0;
    if (R_FAILED(AM_GetTitleList(&read_count, MEDIATYPE_SD, title_count, ids))) {
        free(ids);
        *out_ids = NULL;
        return 0;
    }

    *out_ids = ids;
    return read_count;
}

/* ── Scan a single title (for incremental loading) ── */

bool save_scan_one_title(u64 title_id, GameTitle *out)
{
    return populate_game(title_id, out);
}
