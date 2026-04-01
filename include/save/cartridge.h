/*
 * cartridge.h — Physical game cartridge detection (3DS + NDS).
 *
 * Provides polling for inserted cartridges, reading title info
 * (name, icon, region, product code), and hot-swap detection.
 */

#ifndef CLOUDSAVER_CARTRIDGE_H
#define CLOUDSAVER_CARTRIDGE_H

#include "common.h"

/* ── NDS ROM header (first 0x200 bytes) ── */

typedef struct {
    char title[12];           /* 0x000: Game title (ASCII) */
    char gamecode[4];         /* 0x00C: Game code (e.g. "IPKE") */
    char makercode[2];        /* 0x010: Maker code */
    u8   unitcode;            /* 0x012: 0=NDS, 2=NDS+DSi, 3=DSi */
    u8   seed_select;         /* 0x013 */
    u8   capacity;            /* 0x014: ROM capacity */
    u8   reserved1[7];        /* 0x015-0x01B */
    u8   reserved2;           /* 0x01C */
    u8   region;              /* 0x01D: Region (0=normal) */
    u8   rom_version;         /* 0x01E */
    u8   autostart;           /* 0x01F */
    u32  arm9_offset;         /* 0x020 */
    u32  arm9_entry;          /* 0x024 */
    u32  arm9_load;           /* 0x028 */
    u32  arm9_size;           /* 0x02C */
    u32  arm7_offset;         /* 0x030 */
    u32  arm7_entry;          /* 0x034 */
    u32  arm7_load;           /* 0x038 */
    u32  arm7_size;           /* 0x03C */
    u32  fnt_offset;          /* 0x040 */
    u32  fnt_size;            /* 0x044 */
    u32  fat_offset;          /* 0x048 */
    u32  fat_size;            /* 0x04C */
    u32  arm9_overlay_off;    /* 0x050 */
    u32  arm9_overlay_sz;     /* 0x054 */
    u32  arm7_overlay_off;    /* 0x058 */
    u32  arm7_overlay_sz;     /* 0x05C */
    u32  rom_control1;        /* 0x060 */
    u32  rom_control2;        /* 0x064 */
    u32  icon_offset;         /* 0x068: Offset to icon/title data */
    /* FSUSER_GetLegacyRomHeader writes 0x3B4 bytes (IPC buffer size).
     * Pad the struct so we never overflow when used as the output buffer. */
    u8   _pad[0x3B4 - 0x06C];
} NDS_Header;

/* NDS icon/title structure (at icon_offset in ROM).
 * v1 data ends at 0x840, but FSUSER_GetLegacyBannerData uses a
 * 0x23C0 byte IPC buffer, so we pad to that size to prevent overflow. */
typedef struct {
    u16 version;              /* Always 1 for NDS */
    u16 crc16_v1;
    u16 crc16_v2;
    u16 crc16_v3;
    u16 crc16_v4;
    u8  reserved[0x16];
    u8  icon_bitmap[0x200];   /* 32x32 4bpp tile data */
    u16 icon_palette[16];     /* 16 RGB555 colours */
    u16 title_jpn[128];       /* Japanese title (UTF-16) */
    u16 title_eng[128];       /* English title (UTF-16) */
    u16 title_fre[128];       /* French title */
    u16 title_ger[128];       /* German title */
    u16 title_ita[128];       /* Italian title */
    u16 title_spa[128];       /* Spanish title */
    u8  _pad[0x23C0 - 0x840]; /* Pad to IPC buffer size */
} NDS_IconTitle;

/* ── API ── */

/**
 * Check whether a physical cartridge is currently inserted.
 * Returns true if a cart is detected (3DS or NDS).
 */
bool cart_is_inserted(void);

/**
 * Read the inserted cartridge info into a GameTitle struct.
 * Sets is_cartridge=true, media_type, name, icon, region, product_code.
 * Returns true on success, false if no cart or read failure.
 */
bool cart_read_info(GameTitle *out);

/**
 * Poll for cartridge changes (call periodically from main loop).
 * Compares with the currently tracked cart title ID.
 * Returns true if the cartridge changed (inserted, removed, or swapped).
 * On change, *out is filled with the new cart info (or zeroed if removed).
 */
bool cart_poll(u64 prev_cart_id, GameTitle *out);

#endif /* CLOUDSAVER_CARTRIDGE_H */
