/**
 * 3DS CloudSaver - Save Manager Implementation
 * ─────────────────────────────────────────────
 * Uses AM and FS services to enumerate titles and extract/import saves.
 */

#include "save.h"
#include "config.h"
#include <malloc.h>
#include <tex3ds.h>

/*═══════════════════════════════════════════════════════════════*
 *  SMDH structure (defined manually – libctru has no typedef)
 *═══════════════════════════════════════════════════════════════*/
#define SMDH_MAGIC 0x48444D53 /* "SMDH" */

typedef struct {
    u16 short_desc[0x40];   /* Short description (UTF-16, 128 bytes) */
    u16 long_desc[0x80];    /* Long description  (UTF-16, 256 bytes) */
    u16 publisher[0x40];    /* Publisher          (UTF-16, 128 bytes) */
} SMDH_Title;

typedef struct {
    u32 magic;              /* "SMDH" = 0x48444D53 */
    u16 version;
    u16 reserved;
    SMDH_Title titles[16];  /* One per language */
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
    u8  small_icon[0x480];  /* 24x24 RGB565 tiled */
    u8  large_icon[0x1200]; /* 48x48 RGB565 tiled */
} SMDH;

/*═══════════════════════════════════════════════════════════════*
 *  SMDH icon conversion (FBI-style direct tile copy)
 *═══════════════════════════════════════════════════════════════*/

/** Convert an SMDH large icon (48x48 tiled RGB565) to a C2D_Image.
 *
 *  Uses the exact same approach as FBI (Steveice10):
 *    - SMDH icon data is already in Morton (Z-order) tiled format
 *    - GPU textures use the same Morton tiling
 *    - So we copy tile data DIRECTLY, only converting RGB565 → RGBA8
 *    - NO un-tiling to linear, NO re-tiling, NO Y-flip
 *    - UV coordinates handle the mapping at draw time
 */
static bool smdh_icon_to_c2d(const u8 *icon_data, C2D_Image *out)
{
    const u32 icon_w = 48;
    const u32 icon_h = 48;
    const u32 tex_w  = 64;
    const u32 tex_h  = 64;

    /* ── Create GPU texture ─────────────────────────────────────── */
    C3D_Tex *tex = (C3D_Tex *)malloc(sizeof(C3D_Tex));
    if (!tex) return false;

    if (!C3D_TexInit(tex, tex_w, tex_h, GPU_RGBA8)) {
        free(tex);
        return false;
    }
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);

    /* Clear entire texture to transparent black */
    memset(tex->data, 0, tex->size);

    /* ── Copy SMDH tiles directly into GPU texture ──────────────── */
    /* Both SMDH and GPU use the same Morton tile ordering.
     * SMDH: 6×6 tiles of 8×8 pixels (48×48), sequential in memory.
     * GPU:  8×8 tiles (64×64), same Morton order within tiles.
     *
     * FBI copies rows of tiles with memcpy (same format).
     * We can't memcpy because we need RGB565→RGBA8 conversion,
     * but we iterate tiles in the same order. */
    const u16 *src = (const u16 *)icon_data;
    u32 *gpu = (u32 *)tex->data;

    /* Iterate over SMDH tile rows (6 tiles wide × 6 tiles tall) */
    for (u32 ty = 0; ty < icon_h / 8; ty++) {
        for (u32 tx = 0; tx < icon_w / 8; tx++) {
            /* Source tile index (in SMDH): sequential */
            /* Destination tile index (in GPU): same x, same y, but
             * the GPU texture is wider (8 tiles wide instead of 6) */
            u32 dst_tile_idx = ty * (tex_w / 8) + tx;

            for (u32 p = 0; p < 64; p++) {
                u16 rgb565 = *src++;

                u8 r = (u8)(((rgb565 >> 11) & 0x1F) << 3);
                u8 g = (u8)(((rgb565 >>  5) & 0x3F) << 2);
                u8 b = (u8)(( rgb565        & 0x1F) << 3);

                /* GPU_RGBA8: memory layout is {A,B,G,R} per pixel.
                 * As little-endian u32: (R << 24) | (G << 16) | (B << 8) | A */
                gpu[dst_tile_idx * 64 + p] = ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | 0xFF;
            }
        }
    }

    /* Flush data cache so GPU sees the writes */
    GSPGPU_FlushDataCache(tex->data, tex->size);

    /* ── Subtexture: map the 48×48 region within the 64×64 tex ─── */
    /* FBI's UV mapping for draw (screen_draw_texture):
     *   left   = 0
     *   bottom = (tex_h - icon_h) / tex_h = 16/64 = 0.25
     *   right  = icon_w / tex_w = 48/64 = 0.75
     *   top    = 1.0
     *
     * For citro2d SubTexture (non-rotated: top > bottom):
     *   top    = 1.0  →  top of drawn image samples from V=1.0
     *   bottom = 0.25 →  bottom of drawn image samples from V=0.25 */
    Tex3DS_SubTexture *subtex = (Tex3DS_SubTexture *)malloc(sizeof(Tex3DS_SubTexture));
    if (!subtex) { C3D_TexDelete(tex); free(tex); return false; }

    subtex->width  = icon_w;
    subtex->height = icon_h;
    subtex->left   = 0.0f;
    subtex->right  = (float)icon_w / (float)tex_w;            /* 0.75  */
    subtex->top    = 1.0f;
    subtex->bottom = (float)(tex_h - icon_h) / (float)tex_h;  /* 0.25 */

    out->tex    = tex;
    out->subtex = subtex;

    return true;
}

/*═══════════════════════════════════════════════════════════════*
 *  Region detection from SMDH region_lockout
 *═══════════════════════════════════════════════════════════════*/
static GameRegion region_from_smdh(u32 region_lockout)
{
    /* Bit flags: 0=JPN, 1=USA, 2=EUR, 3=AUS, 4=CHN, 5=KOR, 6=TWN */

    /* Region-free: treat as region-free (will show product code instead) */
    if (region_lockout == 0 || region_lockout == 0x7FFFFFFF ||
        region_lockout == 0xFFFFFFFF || region_lockout == 0x7F)
        return REGION_UNKNOWN;

    /* Check single-region or pick the first match */
    if (region_lockout & 0x04) return REGION_EUR; /* Europe */
    if (region_lockout & 0x02) return REGION_USA; /* USA */
    if (region_lockout & 0x01) return REGION_JPN; /* Japan */
    if (region_lockout & 0x20) return REGION_KOR; /* Korea */
    if (region_lockout & 0x40) return REGION_TWN; /* Taiwan */

    return REGION_UNKNOWN;
}

/** Try to detect region from product code last char (P=EUR, E=USA, J=JPN) */
static GameRegion region_from_product_code(const char *code)
{
    if (!code || code[0] == '\0') return REGION_UNKNOWN;
    /* Product codes like CTR-P-AXXX where last char before null indicates region */
    int len = strlen(code);
    char last = code[len - 1];
    switch (last) {
    case 'P': return REGION_EUR;
    case 'E': return REGION_USA;
    case 'J': return REGION_JPN;
    case 'K': return REGION_KOR;
    default: break;
    }
    return REGION_UNKNOWN;
}

GameRegion save_get_region(u64 title_id)
{
    (void)title_id;
    return REGION_UNKNOWN;
}

/*═══════════════════════════════════════════════════════════════*
 *  SMDH helpers
 *═══════════════════════════════════════════════════════════════*/

/** Convert UTF-16LE to UTF-8 (proper multi-byte support).
 *  Handles characters like é (Pokémon), ü, ñ, etc. */
static void smdh_utf16_to_utf8(const u16 *src, char *dst, int max_len)
{
    int di = 0;
    for (int si = 0; src[si] && di < max_len - 1; si++) {
        u32 cp = src[si];

        /* Handle surrogate pairs (characters > U+FFFF) */
        if (cp >= 0xD800 && cp <= 0xDBFF && src[si + 1] >= 0xDC00 && src[si + 1] <= 0xDFFF) {
            cp = 0x10000 + ((cp - 0xD800) << 10) + (src[si + 1] - 0xDC00);
            si++;
        }

        if (cp < 0x80) {
            if (di + 1 > max_len - 1) break;
            dst[di++] = (char)cp;
        } else if (cp < 0x800) {
            if (di + 2 > max_len - 1) break;
            dst[di++] = (char)(0xC0 | (cp >> 6));
            dst[di++] = (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            if (di + 3 > max_len - 1) break;
            dst[di++] = (char)(0xE0 | (cp >> 12));
            dst[di++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            dst[di++] = (char)(0x80 | (cp & 0x3F));
        } else {
            if (di + 4 > max_len - 1) break;
            dst[di++] = (char)(0xF0 | (cp >> 18));
            dst[di++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            dst[di++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            dst[di++] = (char)(0x80 | (cp & 0x3F));
        }
    }
    dst[di] = '\0';
}

/** Read SMDH data for a title.
 *  Uses FSUSER_OpenFileDirectly with PATH_BINARY paths (proven JKSM approach). */
static bool read_smdh(u64 title_id, SMDH *smdh_out)
{
    Handle file_handle = 0;
    Result res;

    /* Archive path: low_id, high_id, mediatype, 0 */
    u32 arch_path[4];
    arch_path[0] = (u32)(title_id & 0xFFFFFFFF);
    arch_path[1] = (u32)(title_id >> 32);
    arch_path[2] = MEDIATYPE_SD;
    arch_path[3] = 0;
    FS_Path archive_path = { PATH_BINARY, 0x10, arch_path };

    /* Binary file path for "icon" inside the archive.
     * This specific format is used by JKSM and the 3DS homebrew menu.
     * It's NOT a UTF-16 string – it's a binary blob:
     *   {0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000}
     * where 0x6E6F6369 = "icon" in little-endian ASCII. */
    static const u32 icon_file_path[] = { 0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000 };
    FS_Path file_path = { PATH_BINARY, 0x14, icon_file_path };

    /* Open file directly (combined archive+file open in one call) */
    res = FSUSER_OpenFileDirectly(&file_handle, ARCHIVE_SAVEDATA_AND_CONTENT,
                                  archive_path, file_path, FS_OPEN_READ, 0);
    if (R_FAILED(res)) return false;

    u32 bytes_read = 0;
    res = FSFILE_Read(file_handle, &bytes_read, 0, smdh_out, sizeof(SMDH));
    FSFILE_Close(file_handle);
    svcCloseHandle(file_handle);

    if (R_FAILED(res) || bytes_read < sizeof(SMDH)) return false;
    if (smdh_out->magic != SMDH_MAGIC) return false;

    return true;
}

/*═══════════════════════════════════════════════════════════════*
 *  Scan installed titles
 *═══════════════════════════════════════════════════════════════*/
int save_scan_titles(GameTitle *games, int max_games)
{
    int count = 0;
    u32 title_count = 0;

    /* Get SD titles */
    Result res = AM_GetTitleCount(MEDIATYPE_SD, &title_count);
    if (R_FAILED(res) || title_count == 0)
        return 0;

    u64 *title_ids = (u64 *)malloc(title_count * sizeof(u64));
    if (!title_ids) return 0;

    u32 read_count = 0;
    res = AM_GetTitleList(&read_count, MEDIATYPE_SD, title_count, title_ids);
    if (R_FAILED(res)) {
        free(title_ids);
        return 0;
    }

    /* One static SMDH buffer reused across iterations (14 KB) */
    static SMDH smdh;

    for (u32 i = 0; i < read_count && count < max_games; i++) {
        u64 tid = title_ids[i];

        /* Filter: only user-installed titles
         * 0x00040000 = application (games & apps)
         * 0x00040002 = demo
         * 0x00040010 = system applet (some users want these too) */
        u32 high = (u32)(tid >> 32);
        if (high != 0x00040000 && high != 0x00040002)
            continue;

        GameTitle *g = &games[count];
        memset(g, 0, sizeof(GameTitle));
        g->title_id = tid;
        g->has_icon  = false;
        g->has_save_data = true;
        g->region = REGION_UNKNOWN;

        /* Default name = lower 32 bits of title ID in hex */
        snprintf(g->name, sizeof(g->name), "%08lX",
                 (unsigned long)(tid & 0xFFFFFFFF));

        /* Try to get product code from AM */
        char product_code[16] = {0};
        if (R_SUCCEEDED(AM_GetTitleProductCode(MEDIATYPE_SD, tid, product_code))) {
            strncpy(g->product_code, product_code, sizeof(g->product_code) - 1);

            /* Detect region from product code prefix (CTR-P-XXXX) */
            /* Last char of a 4-char game code often indicates region:
             * E=USA, P=EUR, J=JPN, K=KOR, W=TWN, etc.  but it's unreliable.
             * We rely on SMDH region_lockout below instead. */
        }

        /* Try to read SMDH for name, icon, and region */
        memset(&smdh, 0, sizeof(smdh));
        if (read_smdh(tid, &smdh)) {
            /* ── Filter: skip homebrew / non-commercial titles ─────
             * Method 1: Product code must start with "CTR-" (all official
             *           3DS games have this, homebrew doesn't).
             * Method 2: If no product code, check publisher field for
             *           known commercial publishers. */
            bool is_official = false;

            /* Check product code (most reliable) */
            if (g->product_code[0] == 'C' && g->product_code[1] == 'T' &&
                g->product_code[2] == 'R' && g->product_code[3] == '-') {
                is_official = true;
            }
            /* Also accept KTR- (New 3DS exclusive titles) */
            if (g->product_code[0] == 'K' && g->product_code[1] == 'T' &&
                g->product_code[2] == 'R' && g->product_code[3] == '-') {
                is_official = true;
            }

            if (!is_official)
                continue; /* Skip homebrew / non-commercial titles */

            /* Get game name: prefer English (1), fallback to Japanese (0),
             * then try any non-empty title */
            char name_buf[128] = {0};
            int lang_order[] = { 1, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
            for (int l = 0; l < 12; l++) {
                int idx = lang_order[l];
                if (idx >= 16) break;
                smdh_utf16_to_utf8(smdh.titles[idx].short_desc, name_buf, sizeof(name_buf));
                if (name_buf[0] != '\0') break;
            }
            if (name_buf[0] != '\0') {
                strncpy(g->name, name_buf, sizeof(g->name) - 1);
            }

            /* Region from SMDH, fallback to product code */
            g->region = region_from_smdh(smdh.region_lockout);
            if (g->region == REGION_UNKNOWN)
                g->region = region_from_product_code(g->product_code);

            /* Convert 48x48 icon to C2D_Image */
            if (smdh_icon_to_c2d(smdh.large_icon, &g->icon)) {
                g->has_icon = true;
            }
        } else {
            /* No SMDH → can't verify publisher → skip (likely homebrew) */
            continue;
        }

        count++;
    }

    free(title_ids);
    return count;
}

/*═══════════════════════════════════════════════════════════════*
 *  Get title ID list (for incremental loading screen)
 *═══════════════════════════════════════════════════════════════*/
u32 save_get_title_ids(u64 **out_ids)
{
    u32 title_count = 0;
    Result res = AM_GetTitleCount(MEDIATYPE_SD, &title_count);
    if (R_FAILED(res) || title_count == 0) {
        *out_ids = NULL;
        return 0;
    }

    u64 *ids = (u64 *)malloc(title_count * sizeof(u64));
    if (!ids) { *out_ids = NULL; return 0; }

    u32 read_count = 0;
    res = AM_GetTitleList(&read_count, MEDIATYPE_SD, title_count, ids);
    if (R_FAILED(res)) {
        free(ids);
        *out_ids = NULL;
        return 0;
    }

    *out_ids = ids;
    return read_count;
}

/*═══════════════════════════════════════════════════════════════*
 *  Scan a single title (for incremental loading)
 *═══════════════════════════════════════════════════════════════*/
bool save_scan_one_title(u64 title_id, GameTitle *out)
{
    static SMDH smdh;

    u32 high = (u32)(title_id >> 32);
    if (high != 0x00040000 && high != 0x00040002)
        return false;

    memset(out, 0, sizeof(GameTitle));
    out->title_id     = title_id;
    out->has_icon     = false;
    out->has_save_data = true;
    out->region       = REGION_UNKNOWN;

    snprintf(out->name, sizeof(out->name), "%08lX",
             (unsigned long)(title_id & 0xFFFFFFFF));

    char product_code[16] = {0};
    if (R_SUCCEEDED(AM_GetTitleProductCode(MEDIATYPE_SD, title_id, product_code))) {
        strncpy(out->product_code, product_code, sizeof(out->product_code) - 1);
    }

    memset(&smdh, 0, sizeof(smdh));
    if (!read_smdh(title_id, &smdh))
        return false;

    /* Filter: only official titles (CTR- or KTR-) */
    bool is_official = false;
    if (out->product_code[0] == 'C' && out->product_code[1] == 'T' &&
        out->product_code[2] == 'R' && out->product_code[3] == '-')
        is_official = true;
    if (out->product_code[0] == 'K' && out->product_code[1] == 'T' &&
        out->product_code[2] == 'R' && out->product_code[3] == '-')
        is_official = true;
    if (!is_official)
        return false;

    /* Game name from SMDH */
    char name_buf[128] = {0};
    int lang_order[] = { 1, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    for (int l = 0; l < 12; l++) {
        int idx = lang_order[l];
        if (idx >= 16) break;
        smdh_utf16_to_utf8(smdh.titles[idx].short_desc, name_buf, sizeof(name_buf));
        if (name_buf[0] != '\0') break;
    }
    if (name_buf[0] != '\0')
        strncpy(out->name, name_buf, sizeof(out->name) - 1);

    /* Region */
    out->region = region_from_smdh(smdh.region_lockout);
    if (out->region == REGION_UNKNOWN)
        out->region = region_from_product_code(out->product_code);

    /* Icon */
    if (smdh_icon_to_c2d(smdh.large_icon, &out->icon))
        out->has_icon = true;

    return true;
}

/*═══════════════════════════════════════════════════════════════*
 *  Helper: try to open a title's save archive
 *  Tries multiple archive types that may work on CFW.
 *═══════════════════════════════════════════════════════════════*/
static Result open_save_archive(FS_Archive *archive, u64 title_id)
{
    u32 path_low  = (u32)(title_id & 0xFFFFFFFF);
    u32 path_high = (u32)(title_id >> 32);
    Result res;

    /* Method 1: ARCHIVE_USER_SAVEDATA with binary path
     * {mediatype, path_low, path_high} – standard approach */
    {
        u32 path_data[3] = { MEDIATYPE_SD, path_low, path_high };
        FS_Path path = { PATH_BINARY, sizeof(path_data), path_data };
        res = FSUSER_OpenArchive(archive, ARCHIVE_USER_SAVEDATA, path);
        if (R_SUCCEEDED(res)) return res;
    }

    /* Method 2: ARCHIVE_USER_SAVEDATA2 (0x0000006B)
     * Some CFW patches expose saves through this archive ID. */
    {
        u32 path_data[3] = { MEDIATYPE_SD, path_low, path_high };
        FS_Path path = { PATH_BINARY, sizeof(path_data), path_data };
        res = FSUSER_OpenArchive(archive, 0x0000006B, path);
        if (R_SUCCEEDED(res)) return res;
    }

    /* Method 3: ARCHIVE_SAVEDATA_AND_CONTENT (0x2345678A)
     * Another alternative used by some tools */
    {
        u32 path_data[4] = { MEDIATYPE_SD, path_low, path_high, 0 };
        FS_Path path = { PATH_BINARY, sizeof(path_data), path_data };
        res = FSUSER_OpenArchive(archive, 0x2345678A, path);
        if (R_SUCCEEDED(res)) return res;
    }

    return res;  /* return last failure code */
}

/*═══════════════════════════════════════════════════════════════*
 *  Check if title has save data
 *═══════════════════════════════════════════════════════════════*/
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

/*═══════════════════════════════════════════════════════════════*
 *  Helper: recursively extract all files from a directory
 *═══════════════════════════════════════════════════════════════*/
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
            /* Convert UTF-16 name to UTF-8 for local filesystem */
            char utf8_name[256] = {0};
            int j;
            for (j = 0; j < 255 && entries[i].name[j]; j++) {
                u16 c = entries[i].name[j];
                if (c < 0x80) {
                    utf8_name[j] = (char)c;
                } else {
                    utf8_name[j] = '_'; /* safe fallback */
                }
            }
            utf8_name[j] = '\0';

            if (entries[i].attributes & FS_ATTRIBUTE_DIRECTORY) {
                /* Recurse into subdirectory */
                char sub_out[MAX_PATH_LEN];
                snprintf(sub_out, sizeof(sub_out), "%s/%s", out_path, utf8_name);

                /* Build sub-directory UTF-16 path */
                int name_len = 0;
                while (entries[i].name[name_len]) name_len++;

                int sub_len = dir_utf16_len + name_len + 1; /* +1 for '/' */
                u16 *sub_utf16 = (u16 *)calloc(sub_len + 1, sizeof(u16));
                if (!sub_utf16) continue;

                memcpy(sub_utf16, dir_utf16, (dir_utf16_len - 1) * 2); /* copy without null */
                int pos = dir_utf16_len - 1;
                /* Append directory name */
                memcpy(&sub_utf16[pos], entries[i].name, name_len * 2);
                pos += name_len;
                sub_utf16[pos++] = '/';
                sub_utf16[pos] = 0;

                total_size += extract_dir(archive, sub_utf16, pos + 1, sub_out);
                free(sub_utf16);
                continue;
            }

            /* Regular file: build output path */
            char file_out[MAX_PATH_LEN];
            snprintf(file_out, sizeof(file_out), "%s/%s", out_path, utf8_name);

            /* Open file in the save archive using the original UTF-16 name */
            Handle file_handle;
            int name_len = 0;
            while (entries[i].name[name_len]) name_len++;

            /* Build full UTF-16 path: dir + filename */
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

/*═══════════════════════════════════════════════════════════════*
 *  Extract save data to a directory
 *═══════════════════════════════════════════════════════════════*/
int save_extract(u64 title_id, const char *out_path)
{
    FS_Archive archive;

    Result res = open_save_archive(&archive, title_id);
    if (R_FAILED(res)) return -1;

    /* Create output directory */
    mkdir(out_path, 0777);

    /* Start extraction from root "/" */
    static const u16 root_path[] = { '/', 0 };
    int total_size = extract_dir(archive, root_path, 2, out_path);

    FSUSER_CloseArchive(archive);
    return total_size;
}

/*═══════════════════════════════════════════════════════════════*
 *  Import save data from a file
 *  in_path: local file on SD card (e.g. /3ds/CloudSaver/tmp/import_.../main)
 *  The file will be written into the game's save archive using the
 *  same filename as the local file.
 *═══════════════════════════════════════════════════════════════*/
bool save_import(u64 title_id, const char *in_path)
{
    FS_Archive archive;

    Result res = open_save_archive(&archive, title_id);
    if (R_FAILED(res)) return false;

    /* Read the local file */
    FILE *fin = fopen(in_path, "rb");
    if (!fin) {
        FSUSER_CloseArchive(archive);
        return false;
    }

    fseek(fin, 0, SEEK_END);
    long file_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(fin);
        FSUSER_CloseArchive(archive);
        return false;
    }

    u8 *buf = (u8 *)malloc(file_size);
    if (!buf) {
        fclose(fin);
        FSUSER_CloseArchive(archive);
        return false;
    }

    fread(buf, 1, file_size, fin);
    fclose(fin);

    /* Get the filename from the path */
    const char *fname = strrchr(in_path, '/');
    fname = fname ? fname + 1 : in_path;

    /* Build UTF-16 path: "/" + filename */
    int name_len = strlen(fname);
    int path_len = 1 + name_len; /* '/' + name */
    u16 *u16path = (u16 *)calloc(path_len + 1, sizeof(u16));
    if (!u16path) {
        free(buf);
        FSUSER_CloseArchive(archive);
        return false;
    }
    u16path[0] = '/';
    for (int i = 0; i < name_len; i++)
        u16path[1 + i] = (u16)(unsigned char)fname[i];
    u16path[path_len] = 0;

    FS_Path fpath = { PATH_UTF16, (path_len + 1) * 2, u16path };

    /* Try to delete the existing file (ignore errors) */
    FSUSER_DeleteFile(archive, fpath);

    /* Create the file with the right size */
    res = FSUSER_CreateFile(archive, fpath, 0, (u64)file_size);
    /* If create fails (file might already exist), try to proceed anyway */

    /* Open for writing */
    Handle file_handle;
    res = FSUSER_OpenFile(&file_handle, archive, fpath,
                          FS_OPEN_WRITE, 0);
    if (R_FAILED(res)) {
        /* Try with CREATE flag */
        res = FSUSER_OpenFile(&file_handle, archive, fpath,
                              FS_OPEN_WRITE | FS_OPEN_CREATE, 0);
    }
    if (R_FAILED(res)) {
        free(u16path);
        free(buf);
        FSUSER_CloseArchive(archive);
        return false;
    }

    u32 bytes_written = 0;
    FSFILE_Write(file_handle, &bytes_written, 0, buf, (u32)file_size,
                 FS_WRITE_FLUSH);

    FSFILE_Close(file_handle);
    svcCloseHandle(file_handle);
    free(u16path);
    free(buf);

    /* Commit save data changes — CRITICAL for the save to persist */
    FSUSER_ControlArchive(archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA,
                          NULL, 0, NULL, 0);
    FSUSER_CloseArchive(archive);

    return bytes_written == (u32)file_size;
}

/*═══════════════════════════════════════════════════════════════*
 *  Import ALL files from a directory into a title's save archive
 *  ─────────────────────────────────────────────────────────────
 *  Opens the archive ONCE, deletes existing files, writes all
 *  new files, commits ONCE at the end, and closes.
 *  This prevents inconsistent states from multiple open/close
 *  cycles and is the correct approach for multi-file saves.
 *═══════════════════════════════════════════════════════════════*/
bool save_import_directory(u64 title_id, const char *dir_path)
{
    FS_Archive archive;

    Result res = open_save_archive(&archive, title_id);
    if (R_FAILED(res)) return false;

    /* First pass: delete ALL existing files in the save root
     * (like JKSM's delete_directory_recursively before restore) */
    {
        Handle dir_handle;
        static const u16 root_utf16[] = { '/', 0 };
        FS_Path root_path = { PATH_UTF16, 4, root_utf16 };

        if (R_SUCCEEDED(FSUSER_OpenDirectory(&dir_handle, archive, root_path))) {
            FS_DirectoryEntry del_entries[16];
            u32 del_read = 0;
            do {
                res = FSDIR_Read(dir_handle, &del_read, 16, del_entries);
                if (R_FAILED(res)) break;
                for (u32 i = 0; i < del_read; i++) {
                    if (del_entries[i].attributes & FS_ATTRIBUTE_DIRECTORY)
                        continue; /* skip dirs for now */

                    int nlen = 0;
                    while (del_entries[i].name[nlen]) nlen++;

                    int plen = 1 + nlen; /* '/' + name */
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

    /* Second pass: write all files from dir_path into the archive */
    DIR *dir = opendir(dir_path);
    if (!dir) {
        FSUSER_CloseArchive(archive);
        return false;
    }

    struct dirent *ent;
    bool all_ok = true;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        /* Read local file */
        char local_path[MAX_PATH_LEN];
        snprintf(local_path, sizeof(local_path), "%s/%s", dir_path, ent->d_name);

        FILE *fin = fopen(local_path, "rb");
        if (!fin) { all_ok = false; continue; }

        fseek(fin, 0, SEEK_END);
        long file_size = ftell(fin);
        fseek(fin, 0, SEEK_SET);

        if (file_size <= 0) { fclose(fin); continue; }

        u8 *buf = (u8 *)malloc(file_size);
        if (!buf) { fclose(fin); all_ok = false; continue; }

        fread(buf, 1, file_size, fin);
        fclose(fin);

        /* Build UTF-16 path: "/" + filename */
        const char *fname = ent->d_name;
        int name_len = strlen(fname);
        int path_len = 1 + name_len;
        u16 *u16path = (u16 *)calloc(path_len + 1, sizeof(u16));
        if (!u16path) { free(buf); all_ok = false; continue; }

        u16path[0] = '/';
        for (int i = 0; i < name_len; i++)
            u16path[1 + i] = (u16)(unsigned char)fname[i];
        u16path[path_len] = 0;

        FS_Path fpath = { PATH_UTF16, (path_len + 1) * 2, u16path };

        /* Delete existing (ignore errors) */
        FSUSER_DeleteFile(archive, fpath);

        /* Create file with correct size */
        FSUSER_CreateFile(archive, fpath, 0, (u64)file_size);

        /* Open and write */
        Handle file_handle;
        res = FSUSER_OpenFile(&file_handle, archive, fpath,
                              FS_OPEN_WRITE, 0);
        if (R_FAILED(res)) {
            res = FSUSER_OpenFile(&file_handle, archive, fpath,
                                  FS_OPEN_WRITE | FS_OPEN_CREATE, 0);
        }

        if (R_SUCCEEDED(res)) {
            u32 bytes_written = 0;
            FSFILE_Write(file_handle, &bytes_written, 0, buf,
                         (u32)file_size, FS_WRITE_FLUSH);
            FSFILE_Close(file_handle);
            svcCloseHandle(file_handle);

            if (bytes_written != (u32)file_size)
                all_ok = false;
        } else {
            all_ok = false;
        }

        free(u16path);
        free(buf);
    }
    closedir(dir);

    /* Single commit at the end — CRITICAL */
    FSUSER_ControlArchive(archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA,
                          NULL, 0, NULL, 0);
    FSUSER_CloseArchive(archive);

    return all_ok;
}

/*═══════════════════════════════════════════════════════════════*
 *  Delete Secure Value (Anti-Savescum Protection)
 *  ─────────────────────────────────────────────
 *  Many 3DS games (Pokémon, Animal Crossing, etc.) store a
 *  "secure value" in NAND that increments on each save.
 *  When restoring an older save, the counter won't match and
 *  the game will reject the save as corrupt.
 *
 *  Solution: Delete the secure value before restoring.
 *  This is exactly what JKSM and Checkpoint do.
 *═══════════════════════════════════════════════════════════════*/
bool save_delete_secure_value(u64 title_id)
{
    /* Replicate exactly what JKSM does:
     *   UniqueID = (lower 32 bits of title_id) >> 8
     *   Input    = (SECUREVALUE_SLOT_SD << 32) | (UniqueID << 8)
     *
     * See: JKSM/source/FS/FS.cpp  FS::DeleteSecureValue()
     *      JKSM/source/Data/TitleData.cpp  GetUniqueID() = GetLowerID() >> 8
     */
    u32 lower_id  = (u32)(title_id & 0xFFFFFFFF);
    u32 unique_id = lower_id >> 8;

    u64 input = ((u64)SECUREVALUE_SLOT_SD << 32) | ((u64)unique_id << 8);
    u8 output = 0;

    Result res = FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE,
                                          &input, sizeof(u64),
                                          &output, sizeof(u8));
    /* It's OK if it fails — the title might not have a secure value */
    return R_SUCCEEDED(res) || true;
}

/*═══════════════════════════════════════════════════════════════*
 *  Path builders
 *═══════════════════════════════════════════════════════════════*/
void save_build_cloud_path(char *out, size_t max_len,
                           const char *device_name,
                           const char *game_name,
                           u64 game_id)
{
    /* Structure: /DEVICENAME/GAME/GAMEID/SAVEFILES/ */
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

/*═══════════════════════════════════════════════════════════════*
 *  Metadata (simple key=value format)
 *═══════════════════════════════════════════════════════════════*/
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
        /* Remove trailing newline */
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

/*═══════════════════════════════════════════════════════════════*
 *  Icon / Name helpers
 *═══════════════════════════════════════════════════════════════*/
bool save_get_icon(u64 title_id, C2D_Image *out_icon)
{
    static SMDH smdh;
    memset(&smdh, 0, sizeof(smdh));
    if (!read_smdh(title_id, &smdh)) return false;
    return smdh_icon_to_c2d(smdh.large_icon, out_icon);
}

bool save_get_name(u64 title_id, char *out_name, size_t max_len)
{
    static SMDH smdh;
    memset(&smdh, 0, sizeof(smdh));
    if (!read_smdh(title_id, &smdh)) return false;

    char name_buf[128] = {0};
    smdh_utf16_to_utf8(smdh.titles[1].short_desc, name_buf, sizeof(name_buf));
    if (name_buf[0] == '\0')
        smdh_utf16_to_utf8(smdh.titles[0].short_desc, name_buf, sizeof(name_buf));
    if (name_buf[0] == '\0') return false;

    strncpy(out_name, name_buf, max_len - 1);
    out_name[max_len - 1] = '\0';
    return true;
}

/*═══════════════════════════════════════════════════════════════*
 *  Free icon textures when cleaning up
 *═══════════════════════════════════════════════════════════════*/
void save_free_icons(GameTitle *games, int count)
{
    for (int i = 0; i < count; i++) {
        if (games[i].has_icon) {
            if (games[i].icon.tex) {
                C3D_TexDelete(games[i].icon.tex);
                free((void *)games[i].icon.tex);
            }
            if (games[i].icon.subtex) {
                free((void *)games[i].icon.subtex);
            }
            games[i].has_icon = false;
        }
    }
}
