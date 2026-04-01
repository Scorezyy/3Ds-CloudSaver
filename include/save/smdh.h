#ifndef CLOUDSAVER_SMDH_H
#define CLOUDSAVER_SMDH_H

#include "common.h"

/* SMDH structures */

#define SMDH_MAGIC 0x48444D53

typedef struct {
    u16 short_desc[0x40];
    u16 long_desc[0x80];
    u16 publisher[0x40];
} SMDH_Title;

typedef struct {
    u32 magic;
    u16 version;
    u16 reserved;
    SMDH_Title titles[16];
    u8  ratings[16];
    u32 region_lockout;
    u32 match_maker_id;
    u64 match_maker_bit_id;
    u32 flags;
    u16 eula_version;
    u16 reserved2;
    u32 optimal_bn_anim_frame;
    u32 cec_id;
    u64 reserved3;
    u8  small_icon[0x480];
    u8  large_icon[0x1200];
} SMDH;

/* SMDH reading */
bool smdh_read(u64 title_id, SMDH *smdh_out);

/* Icon conversion (48x48 tiled RGB565 → C2D_Image) */
bool smdh_icon_to_c2d(const u8 *icon_data, C2D_Image *out);

/* UTF-16LE → UTF-8 with full multi-byte support */
void smdh_utf16_to_utf8(const u16 *src, char *dst, int max_len);

/* Region detection */
GameRegion region_from_smdh(u32 region_lockout);
GameRegion region_from_product_code(const char *code);

/* High-level helpers */
bool save_get_icon(u64 title_id, C2D_Image *out_icon);
bool save_get_name(u64 title_id, char *out_name, size_t max_len);
GameRegion save_get_region(u64 title_id);
void save_free_icons(GameTitle *games, int count);

#endif /* CLOUDSAVER_SMDH_H */
