#include "smdh.h"
#include <malloc.h>
#include <tex3ds.h>

/* ── Icon conversion (FBI-style direct tile copy) ── */

bool smdh_icon_to_c2d(const u8 *icon_data, C2D_Image *out)
{
    const u32 icon_w = 48;
    const u32 icon_h = 48;
    const u32 tex_w  = 64;
    const u32 tex_h  = 64;

    C3D_Tex *tex = (C3D_Tex *)malloc(sizeof(C3D_Tex));
    if (!tex) return false;

    if (!C3D_TexInit(tex, tex_w, tex_h, GPU_RGBA8)) {
        free(tex);
        return false;
    }
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
    memset(tex->data, 0, tex->size);

    /* Both SMDH and GPU use Morton tile ordering.
     * SMDH: 6×6 tiles of 8×8 (48×48), sequential.
     * GPU:  8×8 tiles (64×64), same Morton order within tiles.
     * We iterate tiles in order, converting RGB565 → RGBA8. */
    const u16 *src = (const u16 *)icon_data;
    u32 *gpu = (u32 *)tex->data;

    for (u32 ty = 0; ty < icon_h / 8; ty++) {
        for (u32 tx = 0; tx < icon_w / 8; tx++) {
            u32 dst_tile_idx = ty * (tex_w / 8) + tx;

            for (u32 p = 0; p < 64; p++) {
                u16 rgb565 = *src++;
                u8 r = (u8)(((rgb565 >> 11) & 0x1F) << 3);
                u8 g = (u8)(((rgb565 >>  5) & 0x3F) << 2);
                u8 b = (u8)(( rgb565        & 0x1F) << 3);

                /* GPU_RGBA8 little-endian: (R << 24) | (G << 16) | (B << 8) | A */
                gpu[dst_tile_idx * 64 + p] = ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | 0xFF;
            }
        }
    }

    GSPGPU_FlushDataCache(tex->data, tex->size);

    Tex3DS_SubTexture *subtex = (Tex3DS_SubTexture *)malloc(sizeof(Tex3DS_SubTexture));
    if (!subtex) { C3D_TexDelete(tex); free(tex); return false; }

    subtex->width  = icon_w;
    subtex->height = icon_h;
    subtex->left   = 0.0f;
    subtex->right  = (float)icon_w / (float)tex_w;
    subtex->top    = 1.0f;
    subtex->bottom = (float)(tex_h - icon_h) / (float)tex_h;

    out->tex    = tex;
    out->subtex = subtex;
    return true;
}

/* ── Region detection ── */

GameRegion region_from_smdh(u32 region_lockout)
{
    if (region_lockout == 0 || region_lockout == 0x7FFFFFFF ||
        region_lockout == 0xFFFFFFFF || region_lockout == 0x7F)
        return REGION_UNKNOWN;

    if (region_lockout & 0x04) return REGION_EUR;
    if (region_lockout & 0x02) return REGION_USA;
    if (region_lockout & 0x01) return REGION_JPN;
    if (region_lockout & 0x20) return REGION_KOR;
    if (region_lockout & 0x40) return REGION_TWN;

    return REGION_UNKNOWN;
}

GameRegion region_from_product_code(const char *code)
{
    if (!code || code[0] == '\0') return REGION_UNKNOWN;
    int len = strlen(code);
    char last = code[len - 1];
    switch (last) {
    case 'P': return REGION_EUR;
    case 'E': return REGION_USA;
    case 'J': return REGION_JPN;
    case 'K': return REGION_KOR;
    default:  break;
    }
    return REGION_UNKNOWN;
}

GameRegion save_get_region(u64 title_id)
{
    (void)title_id;
    return REGION_UNKNOWN;
}

/* ── UTF-16LE → UTF-8 ── */

void smdh_utf16_to_utf8(const u16 *src, char *dst, int max_len)
{
    int di = 0;
    for (int si = 0; src[si] && di < max_len - 1; si++) {
        u32 cp = src[si];

        /* Surrogate pairs (> U+FFFF) */
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

/* ── Read SMDH from title ── */

bool smdh_read(u64 title_id, SMDH *smdh_out)
{
    Handle file_handle = 0;
    Result res;

    u32 arch_path[4];
    arch_path[0] = (u32)(title_id & 0xFFFFFFFF);
    arch_path[1] = (u32)(title_id >> 32);
    arch_path[2] = MEDIATYPE_SD;
    arch_path[3] = 0;
    FS_Path archive_path = { PATH_BINARY, 0x10, arch_path };

    /* Binary file path for "icon" inside the archive (JKSM approach) */
    static const u32 icon_file_path[] = { 0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000 };
    FS_Path file_path = { PATH_BINARY, 0x14, icon_file_path };

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

/* ── High-level helpers ── */

bool save_get_icon(u64 title_id, C2D_Image *out_icon)
{
    static SMDH smdh;
    memset(&smdh, 0, sizeof(smdh));
    if (!smdh_read(title_id, &smdh)) return false;
    return smdh_icon_to_c2d(smdh.large_icon, out_icon);
}

bool save_get_name(u64 title_id, char *out_name, size_t max_len)
{
    static SMDH smdh;
    memset(&smdh, 0, sizeof(smdh));
    if (!smdh_read(title_id, &smdh)) return false;

    char name_buf[128] = {0};
    smdh_utf16_to_utf8(smdh.titles[1].short_desc, name_buf, sizeof(name_buf));
    if (name_buf[0] == '\0')
        smdh_utf16_to_utf8(smdh.titles[0].short_desc, name_buf, sizeof(name_buf));
    if (name_buf[0] == '\0') return false;

    strncpy(out_name, name_buf, max_len - 1);
    out_name[max_len - 1] = '\0';
    return true;
}

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
