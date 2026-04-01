/*
 * ui_misc.c — Setup, connecting, sync, toast, navbar, confirm dialog.
 */

#include "ui.h"
#include "ui_common.h"
#include "lang.h"

/* Setup Screen (Device Name) */
void ui_draw_setup(void)
{
    /* Top screen: app title + welcome */
    C2D_TargetClear(ui_top_target, CLR_BACKGROUND);
    C2D_SceneBegin(ui_top_target);

    ui_draw_text_centered_top(60.0f, FONT_SIZE_TITLE, CLR_ACCENT,
                              lang_str(STR_APP_TITLE));
    ui_draw_text_centered_top(110.0f, FONT_SIZE_MED, CLR_TEXT,
                              lang_str(STR_WELCOME));
    ui_draw_text_centered_top(150.0f, FONT_SIZE_SMALL, CLR_SUBTEXT,
                              APP_VERSION_STRING);

    /* Bottom screen: prompt */
    C2D_TargetClear(ui_bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(ui_bottom_target);

    /* Icon area (placeholder box) */
    float box_x = (BOTTOM_SCREEN_WIDTH - 80) / 2.0f;
    ui_draw_rounded_rect(box_x, 30.0f, 80.0f, 80.0f, 8.0f, CLR_SURFACE);
    ui_draw_text_centered(50.0f, FONT_SIZE_TITLE, CLR_ACCENT, "CS");

    ui_draw_text_centered(130.0f, FONT_SIZE_MED, CLR_TEXT,
                          lang_str(STR_ENTER_DEVICE_NAME));
    ui_draw_text_centered(160.0f, FONT_SIZE_SMALL, CLR_SUBTEXT,
                          lang_str(STR_DEVICE_NAME_HINT));

    /* "Press A" prompt */
    float pulse = 0.7f + 0.3f * ((float)(g_ctx.frame_count % 60) / 60.0f);
    u32 pulse_clr = C2D_Color32(137, 180, 250, (u32)(255 * pulse));
    ui_draw_text_centered(200.0f, FONT_SIZE_SMALL, pulse_clr,
                          "Press  A  to enter name");
}

/* Connecting Screen */
void ui_draw_connecting(void)
{
    C2D_TargetClear(ui_top_target, CLR_BACKGROUND);
    C2D_SceneBegin(ui_top_target);
    ui_draw_text_centered_top(100.0f, FONT_SIZE_LARGE, CLR_ACCENT,
                              lang_str(STR_APP_TITLE));

    C2D_TargetClear(ui_bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(ui_bottom_target);

    /* Spinning indicator (simple dots animation) */
    int dots = (g_ctx.frame_count / 15) % 4;
    char loading[32];
    snprintf(loading, sizeof(loading), "%s%.*s",
             lang_str(STR_CONNECTING), dots, "...");

    ui_draw_text_centered(100.0f, FONT_SIZE_MED, CLR_TEXT, loading);

    /* Progress bar (indeterminate) */
    float bar_x = 60.0f;
    float bar_w = BOTTOM_SCREEN_WIDTH - 120.0f;
    float bar_y = 140.0f;
    float bar_h = 6.0f;
    C2D_DrawRectSolid(bar_x, bar_y, 0.5f, bar_w, bar_h, CLR_SURFACE);

    /* Sliding segment */
    float seg_w = bar_w * 0.3f;
    float phase = (float)(g_ctx.frame_count % 120) / 120.0f;
    float seg_x = bar_x + (bar_w - seg_w) * phase;
    C2D_DrawRectSolid(seg_x, bar_y, 0.5f, seg_w, bar_h, CLR_ACCENT);
}

/* Sync All Screen */
void ui_draw_bottom_sync(void)
{
    C2D_TargetClear(ui_bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(ui_bottom_target);

    ui_draw_text_centered(40.0f, FONT_SIZE_LARGE, CLR_ACCENT,
                          lang_str(STR_SYNC_ALL));
    ui_draw_text_centered(80.0f, FONT_SIZE_MED, CLR_TEXT,
                          lang_str(STR_SYNC_IN_PROGRESS));

    /* Indeterminate progress */
    float bar_x = 40.0f;
    float bar_w = BOTTOM_SCREEN_WIDTH - 80.0f;
    float bar_y = 120.0f;
    float bar_h = 8.0f;
    C2D_DrawRectSolid(bar_x, bar_y, 0.5f, bar_w, bar_h, CLR_SURFACE);

    float seg_w = bar_w * 0.25f;
    float phase = (float)(g_ctx.frame_count % 90) / 90.0f;
    float seg_x = bar_x + (bar_w - seg_w) * phase;
    C2D_DrawRectSolid(seg_x, bar_y, 0.5f, seg_w, bar_h, CLR_ACCENT);

    ui_draw_text_centered(150.0f, FONT_SIZE_SMALL, CLR_SUBTEXT,
                          lang_str(STR_PLEASE_WAIT));
}

/* Status Toast */
void ui_draw_status_toast(void)
{
    if (g_ctx.status_message[0] == '\0') return;

    float alpha = CLAMP(g_ctx.status_timer / 0.5f, 0.0f, 1.0f);
    u32 bg = C2D_Color32(49, 50, 68, (u32)(220 * alpha));
    u32 fg = C2D_Color32(205, 214, 244, (u32)(255 * alpha));

    float toast_w = BOTTOM_SCREEN_WIDTH - 40.0f;
    float toast_h = 28.0f;
    float toast_x = 20.0f;
    float toast_y = BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT - toast_h - 8.0f;

    C2D_SceneBegin(ui_bottom_target);
    ui_draw_rounded_rect(toast_x, toast_y, toast_w, toast_h, 6.0f, bg);

    C2D_Text txt;
    C2D_TextFontParse(&txt, ui_font, ui_text_buf, g_ctx.status_message);
    C2D_TextOptimize(&txt);
    float tw, th;
    C2D_TextGetDimensions(&txt, FONT_SIZE_SMALL, FONT_SIZE_SMALL, &tw, &th);
    float tx = toast_x + (toast_w - tw) / 2.0f;
    float ty = toast_y + (toast_h - th) / 2.0f;
    C2D_DrawText(&txt, C2D_WithColor, tx, ty, 0.95f,
                 FONT_SIZE_SMALL, FONT_SIZE_SMALL, fg);
}

/* Navigation Bar */
void ui_draw_navbar(void)
{
    C2D_SceneBegin(ui_bottom_target);

    float y = BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT;
    C2D_DrawRectSolid(0, y, 0.8f, BOTTOM_SCREEN_WIDTH, NAVBAR_HEIGHT,
                      CLR_NAVBAR);

    /* Divider line */
    C2D_DrawRectSolid(0, y, 0.85f, BOTTOM_SCREEN_WIDTH, 1.0f, CLR_OVERLAY);

    const char *labels[NAV_COUNT] = {
        lang_str(STR_NAV_GAMES),
        lang_str(STR_NAV_SYNC),
        lang_str(STR_NAV_SETTINGS),
    };

    float tab_w = BOTTOM_SCREEN_WIDTH / (float)NAV_COUNT;
    for (int i = 0; i < NAV_COUNT; i++) {
        float tx = i * tab_w;
        bool active = (i == (int)g_ctx.current_tab);

        /* Active indicator bar */
        if (active) {
            C2D_DrawRectSolid(tx + 8, y + 2, 0.85f,
                              tab_w - 16, 3.0f, CLR_NAVBAR_ACTIVE);
        }

        /* Label */
        u32 clr = active ? CLR_NAVBAR_ACTIVE : CLR_SUBTEXT;
        C2D_Text txt;
        C2D_TextFontParse(&txt, ui_font, ui_text_buf, labels[i]);
        C2D_TextOptimize(&txt);
        float tw, th;
        C2D_TextGetDimensions(&txt, FONT_SIZE_SMALL, FONT_SIZE_SMALL,
                              &tw, &th);
        float lx = tx + (tab_w - tw) / 2.0f;
        float ly = y + (NAVBAR_HEIGHT - th) / 2.0f + 2.0f;
        C2D_DrawText(&txt, C2D_WithColor, lx, ly, 0.9f,
                     FONT_SIZE_SMALL, FONT_SIZE_SMALL, clr);
    }
}

/* Confirm Dialog (blocking) */
bool ui_confirm_dialog(const char *title, const char *message)
{
    bool result = false;
    bool done = false;

    while (aptMainLoop() && !done) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_A) { result = true;  done = true; }
        if (kDown & KEY_B) { result = false; done = true; }

        C2D_TextBufClear(ui_text_buf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(ui_bottom_target, CLR_BACKGROUND);
        C2D_SceneBegin(ui_bottom_target);

        /* Dialog box */
        float dw = 280.0f, dh = 120.0f;
        float dx = (BOTTOM_SCREEN_WIDTH - dw) / 2.0f;
        float dy = (BOTTOM_SCREEN_HEIGHT - dh) / 2.0f;
        ui_draw_rounded_rect(dx, dy, dw, dh, 8.0f, CLR_SURFACE);

        /* Title */
        ui_draw_text_centered(dy + 12, FONT_SIZE_MED, CLR_ACCENT, title);

        /* Message */
        ui_draw_text_centered(dy + 42, FONT_SIZE_SMALL, CLR_TEXT, message);

        /* Buttons */
        char a_label[32], b_label[32];
        snprintf(a_label, sizeof(a_label), "A: %s", lang_str(STR_YES));
        snprintf(b_label, sizeof(b_label), "B: %s", lang_str(STR_NO));
        ui_draw_text(dx + 30, dy + dh - 30, FONT_SIZE_SMALL,
                     CLR_SUCCESS, a_label);
        ui_draw_text(dx + dw - 100, dy + dh - 30, FONT_SIZE_SMALL,
                     CLR_ERROR, b_label);

        C3D_FrameEnd(0);
    }

    return result;
}
