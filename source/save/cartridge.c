/*
 * cartridge.c — Physical game cartridge detection (3DS + NDS).
 *
 * Uses FSUSER_CardSlotIsInserted / FSUSER_GetCardType to detect
 * the card type, then reads metadata accordingly:
 *  - 3DS (CTR): AM title list + SMDH for name/icon/region
 *  - NDS (TWL): FSUSER_GetLegacyRomHeader + GetLegacyBannerData
 */

#include "cartridge.h"
#include "smdh.h"
#include <malloc.h>

/* ── Common helpers ── */

static GameRegion nds_region_from_gamecode(const char *gamecode)
{
    if (!gamecode || gamecode[3] == '\0') return REGION_UNKNOWN;
    switch (gamecode[3]) {
    case 'P': case 'X': case 'Y': case 'D':
    case 'F': case 'S': case 'H': case 'I':
    case 'R': case 'U': case 'V':
        return REGION_EUR;
    case 'E':
        return REGION_USA;
    case 'J':
        return REGION_JPN;
    case 'K':
        return REGION_KOR;
    case 'T': case 'W':
        return REGION_TWN;
    default:
        return REGION_UNKNOWN;
    }
}

/* ── 3DS (CTR) cartridge ── */

static bool cart_3ds_get_title_id(u64 *out_tid)
{
    u32 count = 0;
    if (R_FAILED(AM_GetTitleCount(MEDIATYPE_GAME_CARD, &count)) || count == 0)
        return false;

    u64 tid = 0;
    u32 read = 0;
    if (R_FAILED(AM_GetTitleList(&read, MEDIATYPE_GAME_CARD, 1, &tid))
        || read == 0)
        return false;

    u32 high = (u32)(tid >> 32);
    if (high != 0x00040000 && high != 0x00040002)
        return false;

    *out_tid = tid;
    return true;
}

static bool cart_3ds_read_smdh(u64 title_id, SMDH *smdh_out)
{
    Handle file_handle = 0;

    u32 arch_path[4];
    arch_path[0] = (u32)(title_id & 0xFFFFFFFF);
    arch_path[1] = (u32)(title_id >> 32);
    arch_path[2] = MEDIATYPE_GAME_CARD;
    arch_path[3] = 0;
    FS_Path archive_path = { PATH_BINARY, 0x10, arch_path };

    static const u32 icon_file_path[] = {
        0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000
    };
    FS_Path file_path = { PATH_BINARY, 0x14, icon_file_path };

    Result res = FSUSER_OpenFileDirectly(&file_handle,
                                         ARCHIVE_SAVEDATA_AND_CONTENT,
                                         archive_path, file_path,
                                         FS_OPEN_READ, 0);
    if (R_FAILED(res)) return false;

    u32 bytes_read = 0;
    res = FSFILE_Read(file_handle, &bytes_read, 0, smdh_out, sizeof(SMDH));
    FSFILE_Close(file_handle);
    svcCloseHandle(file_handle);

    if (R_FAILED(res) || bytes_read < sizeof(SMDH)) return false;
    return smdh_out->magic == SMDH_MAGIC;
}

static bool cart_3ds_populate(u64 tid, GameTitle *g)
{
    static SMDH smdh;
    static const int lang_order[] = { 1, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    memset(g, 0, sizeof(GameTitle));
    g->title_id      = tid;
    g->is_cartridge  = true;
    g->media_type    = MEDIA_GAMECARD_3DS;
    g->has_save_data = true;
    g->region        = REGION_UNKNOWN;

    snprintf(g->name, sizeof(g->name), "%016llX",
             (unsigned long long)tid);

    char product_code[16] = {0};
    if (R_SUCCEEDED(AM_GetTitleProductCode(MEDIATYPE_GAME_CARD, tid, product_code)))
        strncpy(g->product_code, product_code, sizeof(g->product_code) - 1);

    memset(&smdh, 0, sizeof(smdh));
    if (!cart_3ds_read_smdh(tid, &smdh))
        return true;

    char name_buf[128] = {0};
    for (int l = 0; l < 12; l++) {
        int idx = lang_order[l];
        if (idx >= 16) break;
        smdh_utf16_to_utf8(smdh.titles[idx].short_desc, name_buf,
                           sizeof(name_buf));
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

/* ── NDS (TWL) cartridge ── */

/* Convert NDS 32×32 4bpp tiled icon → C2D_Image.
 * NDS icon: 4×4 macro-tiles of 8×8 pixels, 4bpp packed.
 * GPU expects Morton/Z-ordered pixels within each 8×8 tile. */
static bool nds_icon_to_c2d(const u8 *bitmap, const u16 *palette,
                            C2D_Image *out)
{
    const u32 icon_w = 32, icon_h = 32;
    const u32 tex_w  = 64, tex_h  = 64;

    C3D_Tex *tex = (C3D_Tex *)malloc(sizeof(C3D_Tex));
    if (!tex) return false;

    if (!C3D_TexInit(tex, tex_w, tex_h, GPU_RGBA8)) {
        free(tex);
        return false;
    }
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
    memset(tex->data, 0, tex->size);

    u32 *gpu = (u32 *)tex->data;
    int src_idx = 0;

    for (u32 ty = 0; ty < icon_h / 8; ty++) {
        for (u32 tx = 0; tx < icon_w / 8; tx++) {
            u32 dst_tile = ty * (tex_w / 8) + tx;

            for (u32 row = 0; row < 8; row++) {
                for (u32 col = 0; col < 8; col += 2) {
                    u8 byte = bitmap[src_idx++];
                    u8 lo = byte & 0x0F;
                    u8 hi = (byte >> 4) & 0x0F;

                    for (int p = 0; p < 2; p++) {
                        u8 pal_idx = (p == 0) ? lo : hi;
                        u32 px = col + p;

                        /* Morton/Z-order: interleave x and y bits */
                        u32 morton = 0;
                        for (int b = 0; b < 3; b++) {
                            morton |= ((px  >> b) & 1) << (b * 2);
                            morton |= ((row >> b) & 1) << (b * 2 + 1);
                        }

                        u32 pixel;
                        if (pal_idx == 0) {
                            pixel = 0x00000000;
                        } else {
                            u16 c = palette[pal_idx];
                            u8 r = (u8)(((c >>  0) & 0x1F) << 3);
                            u8 g = (u8)(((c >>  5) & 0x1F) << 3);
                            u8 b = (u8)(((c >> 10) & 0x1F) << 3);
                            pixel = ((u32)r << 24) | ((u32)g << 16) |
                                    ((u32)b << 8)  | 0xFF;
                        }
                        gpu[dst_tile * 64 + morton] = pixel;
                    }
                }
            }
        }
    }

    GSPGPU_FlushDataCache(tex->data, tex->size);

    Tex3DS_SubTexture *subtex =
        (Tex3DS_SubTexture *)malloc(sizeof(Tex3DS_SubTexture));
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

/* Read NDS cart header + banner using proper libctru APIs. */
static bool cart_nds_populate(GameTitle *g)
{
    memset(g, 0, sizeof(GameTitle));
    g->is_cartridge  = true;
    g->media_type    = MEDIA_GAMECARD_NDS;
    g->has_save_data = true;
    g->region        = REGION_UNKNOWN;

    /* ── Read ROM header ── */
    NDS_Header hdr;
    memset(&hdr, 0, sizeof(hdr));

    /* programId = 0 → currently inserted card */
    bool have_header = false;
    Result res = FSUSER_GetLegacyRomHeader2(
        sizeof(NDS_Header), MEDIATYPE_GAME_CARD, 0, &hdr);
    if (R_SUCCEEDED(res) && hdr.gamecode[0] != '\0')
        have_header = true;

    if (!have_header) {
        res = FSUSER_GetLegacyRomHeader(MEDIATYPE_GAME_CARD, 0, &hdr);
        if (R_SUCCEEDED(res) && hdr.gamecode[0] != '\0')
            have_header = true;
    }

    if (have_header) {
        /* Product code: "NTR-XXXX" */
        snprintf(g->product_code, sizeof(g->product_code),
                 "NTR-%.4s", hdr.gamecode);

        /* Title from header (12 ASCII chars, space-padded) */
        char name_clean[13] = {0};
        memcpy(name_clean, hdr.title, 12);
        name_clean[12] = '\0';
        for (int i = 11; i >= 0; i--) {
            if (name_clean[i] == ' ' || name_clean[i] == '\0')
                name_clean[i] = '\0';
            else
                break;
        }

        if (name_clean[0] != '\0')
            strncpy(g->name, name_clean, sizeof(g->name) - 1);
        else
            snprintf(g->name, sizeof(g->name), "NDS %.4s", hdr.gamecode);

        g->region = nds_region_from_gamecode(hdr.gamecode);

        /* Stable pseudo title ID from gamecode.
         * 0x00048000_XXXXXXXX where X = gamecode bytes. */
        u32 code_val = 0;
        for (int i = 0; i < 4; i++)
            code_val = (code_val << 8) | (u8)hdr.gamecode[i];
        g->title_id = ((u64)0x00048000 << 32) | (u64)code_val;
    } else {
        /* Fallback: AM might still know about the TWL card */
        u32 count = 0;
        if (R_SUCCEEDED(AM_GetTitleCount(MEDIATYPE_GAME_CARD, &count))
            && count > 0)
        {
            u64 tid = 0;
            u32 read = 0;
            if (R_SUCCEEDED(AM_GetTitleList(&read, MEDIATYPE_GAME_CARD,
                                            1, &tid)) && read > 0)
            {
                g->title_id = tid;
                snprintf(g->name, sizeof(g->name), "NDS %08lX",
                         (unsigned long)(tid & 0xFFFFFFFF));
                char pc[16] = {0};
                if (R_SUCCEEDED(AM_GetTitleProductCode(
                        MEDIATYPE_GAME_CARD, tid, pc)))
                {
                    strncpy(g->product_code, pc,
                            sizeof(g->product_code) - 1);
                    g->region = nds_region_from_gamecode(pc);
                }
            }
        }
        if (g->title_id == 0) {
            snprintf(g->name, sizeof(g->name), "NDS Cartridge");
            g->title_id = 0xDEAD;
        }
    }

    /* ── Read banner (icon + localized titles) ── */
    NDS_IconTitle *banner =
        (NDS_IconTitle *)malloc(sizeof(NDS_IconTitle));
    if (banner) {
        memset(banner, 0, sizeof(NDS_IconTitle));
        res = FSUSER_GetLegacyBannerData(MEDIATYPE_GAME_CARD, 0, banner);

        if (R_SUCCEEDED(res) && banner->version >= 1) {
            /* Try localized title.
             * Order: English → German → Japanese → French → Spanish → Italian */
            static const int title_order[] = { 1, 3, 0, 2, 5, 4 };
            char title_buf[128] = {0};

            for (int t = 0; t < 6; t++) {
                const u16 *src = NULL;
                switch (title_order[t]) {
                case 0: src = banner->title_jpn; break;
                case 1: src = banner->title_eng; break;
                case 2: src = banner->title_fre; break;
                case 3: src = banner->title_ger; break;
                case 4: src = banner->title_ita; break;
                case 5: src = banner->title_spa; break;
                }
                if (!src) continue;

                smdh_utf16_to_utf8(src, title_buf, sizeof(title_buf));
                if (title_buf[0] != '\0') {
                    /* NDS titles contain "\n" between title/subtitle */
                    for (int i = 0; title_buf[i]; i++) {
                        if (title_buf[i] == '\n' || title_buf[i] == '\r')
                            title_buf[i] = ' ';
                    }
                    strncpy(g->name, title_buf, sizeof(g->name) - 1);
                    break;
                }
            }

            /* Convert 32×32 4bpp icon → C2D_Image */
            if (nds_icon_to_c2d(banner->icon_bitmap,
                                banner->icon_palette, &g->icon))
                g->has_icon = true;
        }

        free(banner);
    }

    return true;
}

/* ── Public API ── */

bool cart_is_inserted(void)
{
    bool inserted = false;
    if (R_SUCCEEDED(FSUSER_CardSlotIsInserted(&inserted)) && inserted)
        return true;
    return false;
}

bool cart_read_info(GameTitle *out)
{
    if (!out) return false;
    if (!cart_is_inserted()) return false;

    /* Determine card type */
    FS_CardType type = CARD_CTR;
    Result res = FSUSER_GetCardType(&type);

    if (R_FAILED(res)) {
        /* GetCardType failed — try 3DS first, then NDS */
        u64 tid = 0;
        if (cart_3ds_get_title_id(&tid))
            return cart_3ds_populate(tid, out);
        return cart_nds_populate(out);
    }

    if (type == CARD_CTR) {
        u64 tid = 0;
        if (cart_3ds_get_title_id(&tid))
            return cart_3ds_populate(tid, out);
        return false;
    }

    /* CARD_TWL — NDS cartridge */
    return cart_nds_populate(out);
}

bool cart_poll(u64 prev_cart_id, GameTitle *out)
{
    if (!out) return false;
    memset(out, 0, sizeof(GameTitle));

    GameTitle current;
    memset(&current, 0, sizeof(current));
    bool have_cart = cart_read_info(&current);

    if (!have_cart && prev_cart_id == 0)
        return false;   /* no change: still empty */

    if (!have_cart && prev_cart_id != 0)
        return true;    /* cart removed (out is zeroed) */

    if (have_cart && current.title_id != prev_cart_id) {
        *out = current; /* different cart (or first insert) */
        return true;
    }

    return false;       /* same cart, no change */
}
