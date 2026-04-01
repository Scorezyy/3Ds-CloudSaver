/*
 * ui_settings.c — Settings screen: draw, edit, input.
 */

#include "ui.h"
#include "ui_common.h"
#include "lang.h"
#include "config.h"
#include "input.h"
#include "network.h"

/* Settings item IDs (order must match drawing order) */
#define SETTINGS_ITEM_DEVICE     0
#define SETTINGS_ITEM_LANGUAGE   1
#define SETTINGS_ITEM_TYPE       2
#define SETTINGS_ITEM_HOST       3
#define SETTINGS_ITEM_PORT       4
#define SETTINGS_ITEM_USER       5
#define SETTINGS_ITEM_PASS       6
#define SETTINGS_ITEM_PATH       7
#define SETTINGS_ITEM_AUTO_CONN  8
#define SETTINGS_ITEM_AUTO_SYNC  9
#define SETTINGS_ITEM_STATUS     10
#define SETTINGS_ITEM_CONNECT    11
#define SETTINGS_ITEM_COUNT      12

/* Scroll / selection state (local to this module) */
static int settings_scroll   = 0;
static int settings_selected = 0;

/* Edit the currently selected settings item (keyboard / toggle). */
static void settings_edit_item(int item)
{
    char buf[256] = {0};

    switch (item) {
    case SETTINGS_ITEM_DEVICE:
        if (input_keyboard(lang_str(STR_SETTINGS_DEVICE),
                g_ctx.config.device_name, buf, MAX_DEVICE_NAME)) {
            strncpy(g_ctx.config.device_name, buf, MAX_DEVICE_NAME - 1);
            config_save(&g_ctx.config);
        }
        break;

    case SETTINGS_ITEM_LANGUAGE:
        g_ctx.config.language = (Language)((g_ctx.config.language + 1) % LANG_COUNT);
        lang_set(g_ctx.config.language);
        config_save(&g_ctx.config);
        break;

    case SETTINGS_ITEM_TYPE:
        if (g_ctx.config.server.type == CONN_NONE)
            g_ctx.config.server.type = CONN_SFTP;
        else if (g_ctx.config.server.type == CONN_SFTP)
            g_ctx.config.server.type = CONN_SMB;
        else
            g_ctx.config.server.type = CONN_NONE;
        config_save(&g_ctx.config);
        break;

    case SETTINGS_ITEM_HOST:
        if (input_keyboard(lang_str(STR_SERVER_HOST),
                g_ctx.config.server.host, buf, MAX_HOST_LEN)) {
            strncpy(g_ctx.config.server.host, buf, MAX_HOST_LEN - 1);
            config_save(&g_ctx.config);
        }
        break;

    case SETTINGS_ITEM_PORT: {
        char port_buf[8];
        snprintf(port_buf, sizeof(port_buf), "%d", g_ctx.config.server.port);
        if (input_keyboard(lang_str(STR_SERVER_PORT),
                port_buf, buf, sizeof(buf))) {
            int p = atoi(buf);
            if (p > 0 && p < 65536) {
                g_ctx.config.server.port = p;
                config_save(&g_ctx.config);
            }
        }
        break;
    }

    case SETTINGS_ITEM_USER:
        if (input_keyboard(lang_str(STR_SERVER_USER),
                g_ctx.config.server.username, buf, MAX_USER_LEN)) {
            strncpy(g_ctx.config.server.username, buf, MAX_USER_LEN - 1);
            config_save(&g_ctx.config);
        }
        break;

    case SETTINGS_ITEM_PASS:
        if (input_keyboard(lang_str(STR_SERVER_PASS),
                "", buf, MAX_PASS_LEN)) {
            strncpy(g_ctx.config.server.password, buf, MAX_PASS_LEN - 1);
            config_save(&g_ctx.config);
        }
        break;

    case SETTINGS_ITEM_PATH:
        break;

    case SETTINGS_ITEM_AUTO_CONN:
        g_ctx.config.auto_connect = !g_ctx.config.auto_connect;
        config_save(&g_ctx.config);
        break;

    case SETTINGS_ITEM_AUTO_SYNC:
        g_ctx.config.auto_sync = !g_ctx.config.auto_sync;
        config_save(&g_ctx.config);
        break;

    case SETTINGS_ITEM_STATUS:
        break;

    case SETTINGS_ITEM_CONNECT:
        if (g_ctx.server_connected) {
            net_disconnect();
            g_ctx.server_connected = false;
        }
        g_ctx.server_connected = net_connect(&g_ctx.config.server);
        break;
    }
}

/* Draw the settings screen. */
void ui_draw_bottom_settings(void)
{
    C2D_TargetClear(ui_bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(ui_bottom_target);

    float content_h = BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT;
    float row_h     = 34.0f;
    float gap       = 3.0f;
    float pad       = 8.0f;
    float row_w     = BOTTOM_SCREEN_WIDTH - 2 * pad;
    float header_h  = 30.0f;

    /* Header bar */
    C2D_DrawRectSolid(0, 0, 0.5f, BOTTOM_SCREEN_WIDTH, header_h, CLR_SURFACE);
    ui_draw_text_centered(5.0f, FONT_SIZE_MED, CLR_TEXT,
                          lang_str(STR_SETTINGS));

    /* Hint bar at bottom (above navbar) */
    float hint_h = 18.0f;
    float hint_y = content_h - hint_h;
    C2D_DrawRectSolid(0, hint_y, 0.5f, BOTTOM_SCREEN_WIDTH, hint_h,
                      CLR_SURFACE);
    ui_draw_text(pad, hint_y + 2, 0.38f, CLR_SUBTEXT,
                 "\xE2\x96\xB2\xE2\x96\xBC Navigate   A: Edit   B: Back");

    /* Drawable area */
    float draw_top    = header_h;
    float draw_bottom = hint_y - 2.0f;

    int visible_rows = (int)((draw_bottom - draw_top) / (row_h + gap));
    if (visible_rows < 1) visible_rows = 1;

    /* Clamp scroll */
    if (settings_selected < settings_scroll)
        settings_scroll = settings_selected;
    if (settings_selected >= settings_scroll + visible_rows)
        settings_scroll = settings_selected - visible_rows + 1;
    if (settings_scroll < 0) settings_scroll = 0;
    if (settings_scroll > SETTINGS_ITEM_COUNT - visible_rows)
        settings_scroll = SETTINGS_ITEM_COUNT - visible_rows;
    if (settings_scroll < 0) settings_scroll = 0;

    struct {
        const char *label;
        const char *value;
        u32 value_clr;
        char buf[32];
    } items[SETTINGS_ITEM_COUNT];

    /* Prepare item data */
    items[SETTINGS_ITEM_DEVICE].label = lang_str(STR_SETTINGS_DEVICE);
    items[SETTINGS_ITEM_DEVICE].value = g_ctx.config.device_name;
    items[SETTINGS_ITEM_DEVICE].value_clr = CLR_TEXT;

    items[SETTINGS_ITEM_LANGUAGE].label = lang_str(STR_SETTINGS_LANGUAGE);
    items[SETTINGS_ITEM_LANGUAGE].value = lang_name(g_ctx.config.language);
    items[SETTINGS_ITEM_LANGUAGE].value_clr = CLR_ACCENT;

    items[SETTINGS_ITEM_TYPE].label = lang_str(STR_SERVER_TYPE);
    {
        const char *ts = lang_str(STR_SERVER_TYPE_NONE);
        if (g_ctx.config.server.type == CONN_SFTP)
            ts = lang_str(STR_SERVER_TYPE_SFTP);
        else if (g_ctx.config.server.type == CONN_SMB)
            ts = lang_str(STR_SERVER_TYPE_SMB);
        items[SETTINGS_ITEM_TYPE].value = ts;
    }
    items[SETTINGS_ITEM_TYPE].value_clr = CLR_TEXT;

    items[SETTINGS_ITEM_HOST].label = lang_str(STR_SERVER_HOST);
    items[SETTINGS_ITEM_HOST].value =
        g_ctx.config.server.host[0] ? g_ctx.config.server.host : "\xE2\x80\x94";
    items[SETTINGS_ITEM_HOST].value_clr = CLR_TEXT;

    items[SETTINGS_ITEM_PORT].label = lang_str(STR_SERVER_PORT);
    snprintf(items[SETTINGS_ITEM_PORT].buf,
             sizeof(items[SETTINGS_ITEM_PORT].buf),
             "%d", g_ctx.config.server.port);
    items[SETTINGS_ITEM_PORT].value = items[SETTINGS_ITEM_PORT].buf;
    items[SETTINGS_ITEM_PORT].value_clr = CLR_TEXT;

    items[SETTINGS_ITEM_USER].label = lang_str(STR_SERVER_USER);
    items[SETTINGS_ITEM_USER].value =
        g_ctx.config.server.username[0] ? g_ctx.config.server.username : "\xE2\x80\x94";
    items[SETTINGS_ITEM_USER].value_clr = CLR_TEXT;

    items[SETTINGS_ITEM_PASS].label = lang_str(STR_SERVER_PASS);
    items[SETTINGS_ITEM_PASS].value =
        g_ctx.config.server.password[0] ? "********" : "\xE2\x80\x94";
    items[SETTINGS_ITEM_PASS].value_clr = CLR_TEXT;

    items[SETTINGS_ITEM_PATH].label = lang_str(STR_SERVER_PATH);
    items[SETTINGS_ITEM_PATH].value = "Auto (Server)";
    items[SETTINGS_ITEM_PATH].value_clr = CLR_SUBTEXT;

    items[SETTINGS_ITEM_AUTO_CONN].label = lang_str(STR_SETTINGS_AUTO_CONNECT);
    items[SETTINGS_ITEM_AUTO_CONN].value =
        g_ctx.config.auto_connect ? "ON" : "OFF";
    items[SETTINGS_ITEM_AUTO_CONN].value_clr =
        g_ctx.config.auto_connect ? CLR_SUCCESS : CLR_OVERLAY;

    items[SETTINGS_ITEM_AUTO_SYNC].label = lang_str(STR_SETTINGS_AUTO_SYNC);
    items[SETTINGS_ITEM_AUTO_SYNC].value =
        g_ctx.config.auto_sync ? "ON" : "OFF";
    items[SETTINGS_ITEM_AUTO_SYNC].value_clr =
        g_ctx.config.auto_sync ? CLR_SUCCESS : CLR_OVERLAY;

    items[SETTINGS_ITEM_STATUS].label = "Status";
    items[SETTINGS_ITEM_STATUS].value =
        g_ctx.server_connected
        ? lang_str(STR_SERVER_STATUS_OK)
        : lang_str(STR_SERVER_STATUS_FAIL);
    items[SETTINGS_ITEM_STATUS].value_clr =
        g_ctx.server_connected ? CLR_SUCCESS : CLR_ERROR;

    items[SETTINGS_ITEM_CONNECT].label =
        g_ctx.server_connected ? "\xE2\x86\xBB Reconnect" : "\xE2\x96\xB6 Connect";
    items[SETTINGS_ITEM_CONNECT].value = "";
    items[SETTINGS_ITEM_CONNECT].value_clr = CLR_ACCENT;

    /* Draw visible rows */
    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        float ry = draw_top + (float)(i - settings_scroll) * (row_h + gap);

        if (ry + row_h < draw_top || ry > draw_bottom) continue;

        bool selected = (i == settings_selected);
        u32 bg_clr = selected ? CLR_BUTTON_HOVER : CLR_SURFACE;

        /* Connect button: special rendering */
        if (i == SETTINGS_ITEM_CONNECT) {
            u32 btn_clr = selected ? CLR_ACCENT : C2D_Color32(0x30, 0x80, 0xE0, 0xFF);
            ui_draw_rounded_rect(pad, ry, row_w, row_h, 4.0f, btn_clr);
            ui_draw_text_centered(ry + 8, FONT_SIZE_SMALL, CLR_WHITE,
                                  items[i].label);
            continue;
        }

        /* Row background */
        ui_draw_rounded_rect(pad, ry, row_w, row_h, 4.0f, bg_clr);

        /* Selection indicator */
        if (selected) {
            C2D_DrawRectSolid(pad, ry + 4, 0.6f, 3.0f, row_h - 8, CLR_ACCENT);
        }

        /* Label */
        ui_draw_text(pad + 10, ry + 8, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     items[i].label);

        /* Value */
        if (items[i].value) {
            if (i == SETTINGS_ITEM_STATUS) {
                C2D_DrawCircleSolid(row_w - 10, ry + row_h / 2.0f,
                                    0.6f, 5, items[i].value_clr);
                ui_draw_text_truncated(pad + 130, ry + 8, FONT_SIZE_SMALL,
                                       items[i].value_clr, items[i].value,
                                       row_w - 150);
            } else {
                ui_draw_text_truncated(pad + 130, ry + 8, FONT_SIZE_SMALL,
                                       items[i].value_clr, items[i].value,
                                       row_w - 140);
            }
        }
    }

    /* Scroll indicator */
    if (SETTINGS_ITEM_COUNT > visible_rows) {
        float track_h = draw_bottom - draw_top - 8;
        float thumb_h = track_h * (float)visible_rows / (float)SETTINGS_ITEM_COUNT;
        float thumb_y = draw_top + 4 +
            (track_h - thumb_h) * (float)settings_scroll /
            (float)(SETTINGS_ITEM_COUNT - visible_rows);
        C2D_DrawRectSolid(BOTTOM_SCREEN_WIDTH - 4, thumb_y, 0.6f,
                          3, thumb_h, CLR_OVERLAY);
    }
}

/* Handle input in settings screen. Returns true if user pressed B. */
bool ui_settings_handle_input(u32 kDown, touchPosition touch, bool touched)
{
    if (kDown & KEY_B) return true;

    if (kDown & KEY_DOWN) {
        settings_selected++;
        if (settings_selected >= SETTINGS_ITEM_COUNT)
            settings_selected = 0;
    }
    if (kDown & KEY_UP) {
        settings_selected--;
        if (settings_selected < 0)
            settings_selected = SETTINGS_ITEM_COUNT - 1;
    }

    if (kDown & KEY_A) {
        settings_edit_item(settings_selected);
    }

    /* Touch: figure out which row was tapped */
    if (touched) {
        float header_h  = 30.0f;
        float row_h     = 34.0f;
        float gap       = 3.0f;
        float content_h = BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT;
        float hint_h    = 18.0f;
        float draw_top  = header_h;
        float draw_bot  = content_h - hint_h - 2.0f;

        if (touch.py >= draw_top && touch.py < draw_bot) {
            float rel_y = (float)touch.py - draw_top;
            int tapped = (int)(rel_y / (row_h + gap)) + settings_scroll;
            if (tapped >= 0 && tapped < SETTINGS_ITEM_COUNT) {
                settings_selected = tapped;
                settings_edit_item(tapped);
            }
        }
    }

    return false;
}
