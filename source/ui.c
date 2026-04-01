/**
 * 3DS CloudSaver - UI / Graphics Implementation
 * ──────────────────────────────────────────────
 * Draws both screens using citro2d.
 * Top screen:  game cover grid
 * Bottom screen: save list, details, settings, nav bar
 */

#include "ui.h"
#include "lang.h"
#include "config.h"
#include "network.h"

/*═══════════════════════════════════════════════════════════════*
 *  Render targets & state
 *═══════════════════════════════════════════════════════════════*/
static C3D_RenderTarget *top_target    = NULL;
static C3D_RenderTarget *bottom_target = NULL;
static C2D_TextBuf       text_buf      = NULL;
static C2D_Font          font          = NULL;

/* Animation */
static float anim_transition = 0.0f;
static bool  anim_active     = false;

/* Settings scroll */
static int settings_scroll   = 0;
static int settings_selected = 0;

#define NAVBAR_HEIGHT   40
#define FONT_SIZE_SMALL 0.45f
#define FONT_SIZE_MED   0.55f
#define FONT_SIZE_LARGE 0.70f
#define FONT_SIZE_TITLE 0.85f

/*═══════════════════════════════════════════════════════════════*
 *  Init / Exit
 *═══════════════════════════════════════════════════════════════*/
bool ui_init(void)
{
    top_target    = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    bottom_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    text_buf      = C2D_TextBufNew(8192);

    /* Try loading a custom font from romfs, fall back to system */
    font = C2D_FontLoadSystem(CFG_REGION_USA);

    return true;
}

void ui_exit(void)
{
    if (font)     { C2D_FontFree(font); font = NULL; }
    if (text_buf) { C2D_TextBufDelete(text_buf); text_buf = NULL; }
}

/*═══════════════════════════════════════════════════════════════*
 *  Frame management
 *═══════════════════════════════════════════════════════════════*/
void ui_frame_begin(void)
{
    C2D_TextBufClear(text_buf);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
}

void ui_frame_end(void)
{
    C3D_FrameEnd(0);
}

/*═══════════════════════════════════════════════════════════════*
 *  Helpers
 *═══════════════════════════════════════════════════════════════*/
void ui_draw_text(float x, float y, float size, u32 clr, const char *text)
{
    if (!text || !text_buf) return;
    C2D_Text txt;
    if (font)
        C2D_TextFontParse(&txt, font, text_buf, text);
    else
        C2D_TextParse(&txt, text_buf, text);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_WithColor, x, y, 0.5f, size, size, clr);
}

/** Draw text truncated to max_width pixels, appending "…" if needed. */
static void ui_draw_text_truncated(float x, float y, float size, u32 clr,
                                   const char *text, float max_width)
{
    if (!text || !text_buf) return;

    /* First check if the full text fits */
    C2D_Text txt;
    if (font)
        C2D_TextFontParse(&txt, font, text_buf, text);
    else
        C2D_TextParse(&txt, text_buf, text);
    C2D_TextOptimize(&txt);

    float w, h;
    C2D_TextGetDimensions(&txt, size, size, &w, &h);

    if (w <= max_width) {
        C2D_DrawText(&txt, C2D_WithColor, x, y, 0.5f, size, size, clr);
        return;
    }

    /* Text is too wide – find how many bytes fit, respecting UTF-8 boundaries */
    char buf[128];
    int len = (int)strlen(text);
    if (len > (int)sizeof(buf) - 4) len = (int)sizeof(buf) - 4;

    /* Binary search for the right truncation point */
    int lo = 0, hi = len, best = 0;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;

        /* Don't cut in the middle of a UTF-8 multi-byte sequence */
        int pos = mid;
        while (pos > 0 && ((unsigned char)text[pos] & 0xC0) == 0x80)
            pos--;

        memcpy(buf, text, pos);
        buf[pos] = '\0';
        strcat(buf, "...");

        C2D_Text t2;
        if (font)
            C2D_TextFontParse(&t2, font, text_buf, buf);
        else
            C2D_TextParse(&t2, text_buf, buf);
        C2D_TextOptimize(&t2);

        float tw, th;
        C2D_TextGetDimensions(&t2, size, size, &tw, &th);

        if (tw <= max_width) {
            best = pos;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    /* Draw truncated text */
    memcpy(buf, text, best);
    buf[best] = '\0';
    strcat(buf, "...");

    C2D_Text t3;
    if (font)
        C2D_TextFontParse(&t3, font, text_buf, buf);
    else
        C2D_TextParse(&t3, text_buf, buf);
    C2D_TextOptimize(&t3);
    C2D_DrawText(&t3, C2D_WithColor, x, y, 0.5f, size, size, clr);
}

void ui_draw_text_centered(float y, float size, u32 clr, const char *text)
{
    if (!text || !text_buf) return;
    C2D_Text txt;
    if (font)
        C2D_TextFontParse(&txt, font, text_buf, text);
    else
        C2D_TextParse(&txt, text_buf, text);
    C2D_TextOptimize(&txt);
    float w, h;
    C2D_TextGetDimensions(&txt, size, size, &w, &h);
    float x = (BOTTOM_SCREEN_WIDTH - w) / 2.0f;
    C2D_DrawText(&txt, C2D_WithColor, x, y, 0.5f, size, size, clr);
}

static void draw_text_centered_top(float y, float size, u32 clr, const char *text)
{
    if (!text || !text_buf) return;
    C2D_Text txt;
    if (font)
        C2D_TextFontParse(&txt, font, text_buf, text);
    else
        C2D_TextParse(&txt, text_buf, text);
    C2D_TextOptimize(&txt);
    float w, h;
    C2D_TextGetDimensions(&txt, size, size, &w, &h);
    float x = (TOP_SCREEN_WIDTH - w) / 2.0f;
    C2D_DrawText(&txt, C2D_WithColor, x, y, 0.5f, size, size, clr);
}

void ui_draw_rounded_rect(float x, float y, float w, float h, float r, u32 clr)
{
    /* Simple implementation: draw main rect + corner circles */
    C2D_DrawRectSolid(x + r, y, 0.5f, w - 2*r, h, clr);
    C2D_DrawRectSolid(x, y + r, 0.5f, r, h - 2*r, clr);
    C2D_DrawRectSolid(x + w - r, y + r, 0.5f, r, h - 2*r, clr);
    /* Corner circles */
    C2D_DrawCircleSolid(x + r, y + r, 0.5f, r, clr);
    C2D_DrawCircleSolid(x + w - r, y + r, 0.5f, r, clr);
    C2D_DrawCircleSolid(x + r, y + h - r, 0.5f, r, clr);
    C2D_DrawCircleSolid(x + w - r, y + h - r, 0.5f, r, clr);
}

bool ui_draw_button(float x, float y, float w, float h,
                    const char *label, u32 bg, u32 fg,
                    touchPosition *touch, bool touched)
{
    bool hit = false;
    u32 draw_bg = bg;

    if (touched && touch) {
        if (touch->px >= x && touch->px <= x + w &&
            touch->py >= y && touch->py <= y + h)
        {
            draw_bg = CLR_BUTTON_HOVER;
            hit = true;
        }
    }

    ui_draw_rounded_rect(x, y, w, h, 4.0f, draw_bg);

    /* Center text in button */
    C2D_Text txt;
    if (font)
        C2D_TextFontParse(&txt, font, text_buf, label);
    else
        C2D_TextParse(&txt, text_buf, label);
    C2D_TextOptimize(&txt);
    float tw, th;
    C2D_TextGetDimensions(&txt, FONT_SIZE_SMALL, FONT_SIZE_SMALL, &tw, &th);
    float tx = x + (w - tw) / 2.0f;
    float ty = y + (h - th) / 2.0f;
    C2D_DrawText(&txt, C2D_WithColor, tx, ty, 0.5f,
                 FONT_SIZE_SMALL, FONT_SIZE_SMALL, fg);

    return hit;
}

void ui_draw_progress(float x, float y, float w, float h,
                      float progress, u32 bg, u32 fg)
{
    C2D_DrawRectSolid(x, y, 0.5f, w, h, bg);
    float fill_w = w * CLAMP(progress, 0.0f, 1.0f);
    C2D_DrawRectSolid(x, y, 0.5f, fill_w, h, fg);
}

/*═══════════════════════════════════════════════════════════════*
 *  Animations
 *═══════════════════════════════════════════════════════════════*/
void ui_transition_start(void)
{
    anim_transition = 1.0f;
    anim_active = true;
}

void ui_animation_update(void)
{
    if (anim_active) {
        anim_transition -= 0.05f;
        if (anim_transition <= 0.0f) {
            anim_transition = 0.0f;
            anim_active = false;
        }
        g_ctx.transition_alpha = anim_transition;
    }
}

bool ui_is_transitioning(void)
{
    return anim_active;
}

/*═══════════════════════════════════════════════════════════════*
 *  Setup Screen (Device Name)
 *═══════════════════════════════════════════════════════════════*/
void ui_draw_setup(void)
{
    /* Top screen: app title + welcome */
    C2D_TargetClear(top_target, CLR_BACKGROUND);
    C2D_SceneBegin(top_target);

    draw_text_centered_top(60.0f, FONT_SIZE_TITLE, CLR_ACCENT,
                           lang_str(STR_APP_TITLE));
    draw_text_centered_top(110.0f, FONT_SIZE_MED, CLR_TEXT,
                           lang_str(STR_WELCOME));
    draw_text_centered_top(150.0f, FONT_SIZE_SMALL, CLR_SUBTEXT,
                           APP_VERSION_STRING);

    /* Bottom screen: prompt */
    C2D_TargetClear(bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(bottom_target);

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

/*═══════════════════════════════════════════════════════════════*
 *  Connecting Screen
 *═══════════════════════════════════════════════════════════════*/
void ui_draw_connecting(void)
{
    C2D_TargetClear(top_target, CLR_BACKGROUND);
    C2D_SceneBegin(top_target);
    draw_text_centered_top(100.0f, FONT_SIZE_LARGE, CLR_ACCENT,
                           lang_str(STR_APP_TITLE));

    C2D_TargetClear(bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(bottom_target);

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

/*═══════════════════════════════════════════════════════════════*
 *  Top Screen – Game Cover Grid
 *═══════════════════════════════════════════════════════════════*/
void ui_draw_top_games(void)
{
    C2D_TargetClear(top_target, CLR_BACKGROUND);
    C2D_SceneBegin(top_target);

    /* Header */
    C2D_DrawRectSolid(0, 0, 0.5f, TOP_SCREEN_WIDTH, 28.0f, CLR_SURFACE);
    draw_text_centered_top(4.0f, FONT_SIZE_MED, CLR_TEXT,
                           lang_str(STR_INSTALLED_GAMES));

    /* Connection indicator */
    u32 status_clr = g_ctx.server_connected ? CLR_SUCCESS : CLR_ERROR;
    C2D_DrawCircleSolid(TOP_SCREEN_WIDTH - 16, 14, 0.5f, 5, status_clr);

    /* Game count */
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d", g_ctx.game_count);
    ui_draw_text(8.0f, 6.0f, FONT_SIZE_SMALL, CLR_SUBTEXT, count_str);

    if (g_ctx.game_count == 0) {
        draw_text_centered_top(120.0f, FONT_SIZE_MED, CLR_SUBTEXT,
                               lang_str(STR_NO_GAMES_FOUND));
        return;
    }

    /* Draw game grid */
    float grid_start_y = 34.0f;
    float total_cover_w = COVER_WIDTH + COVER_PADDING;
    float total_cover_h = COVER_HEIGHT + COVER_PADDING; /* no name space needed */
    float grid_x_offset = (TOP_SCREEN_WIDTH - COVERS_PER_ROW * total_cover_w) / 2.0f;

    for (int i = 0; i < g_ctx.game_count; i++) {
        int row = i / COVERS_PER_ROW;
        int col = i % COVERS_PER_ROW;
        int vis_row = row - g_ctx.scroll_offset;

        if (vis_row < 0 || vis_row >= COVERS_VISIBLE_ROWS) continue;

        float x = grid_x_offset + col * total_cover_w;
        float y = grid_start_y + vis_row * total_cover_h;

        bool selected = (i == g_ctx.selected_game);

        /* Selection highlight */
        if (selected) {
            float sel_pulse = 0.8f + 0.2f *
                ((float)((g_ctx.frame_count + i * 7) % 40) / 40.0f);
            u32 sel_clr = C2D_Color32(137, 180, 250, (u32)(200 * sel_pulse));
            ui_draw_rounded_rect(x - 3, y - 3,
                                 COVER_WIDTH + 6, COVER_HEIGHT + 6,
                                 6.0f, sel_clr);
        }

        /* Cover background */
        ui_draw_rounded_rect(x, y, COVER_WIDTH, COVER_HEIGHT, 4.0f,
                             CLR_SURFACE);

        /* Draw icon if available */
        if (g_ctx.games[i].has_icon) {
            C2D_DrawImageAt(g_ctx.games[i].icon, x, y, 0.5f, NULL,
                            COVER_WIDTH / 48.0f, COVER_HEIGHT / 48.0f);
        } else {
            /* Placeholder: first 2 chars of game name */
            char abbr[3] = { g_ctx.games[i].name[0],
                             g_ctx.games[i].name[1], '\0' };
            float ax = x + (COVER_WIDTH - 16) / 2.0f;
            float ay = y + (COVER_HEIGHT - 12) / 2.0f;
            ui_draw_text(ax, ay, FONT_SIZE_SMALL, CLR_ACCENT, abbr);
        }
    }

    /* Transition overlay */
    if (anim_active) {
        u32 overlay = C2D_Color32(30, 30, 46,
                                   (u32)(255 * anim_transition));
        C2D_DrawRectSolid(0, 0, 0.9f, TOP_SCREEN_WIDTH,
                          TOP_SCREEN_HEIGHT, overlay);
    }
}

/*═══════════════════════════════════════════════════════════════*
 *  Bottom Screen – Main (Save List / Details)
 *═══════════════════════════════════════════════════════════════*/
void ui_draw_bottom_main(void)
{
    C2D_TargetClear(bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(bottom_target);

    float content_h = BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT;

    if (g_ctx.state == STATE_MAIN_BROWSE) {
        /* Show instructions */
        if (g_ctx.selected_game >= 0 && g_ctx.selected_game < g_ctx.game_count) {
            GameTitle *game = &g_ctx.games[g_ctx.selected_game];

            /* Game info card */
            ui_draw_rounded_rect(10, 10, BOTTOM_SCREEN_WIDTH - 20, 60,
                                 6.0f, CLR_SURFACE);
            ui_draw_text_truncated(20, 16, FONT_SIZE_MED, CLR_TEXT,
                                   game->name, BOTTOM_SCREEN_WIDTH - 40);

            /* Region badge */
            const char *region_str = "";
            switch (game->region) {
            case REGION_EUR: region_str = lang_str(STR_REGION_EUR); break;
            case REGION_USA: region_str = lang_str(STR_REGION_USA); break;
            case REGION_JPN: region_str = lang_str(STR_REGION_JPN); break;
            default: region_str = lang_str(STR_REGION_UNKNOWN); break;
            }
            ui_draw_text(20, 42, FONT_SIZE_SMALL, CLR_SUBTEXT, region_str);

            /* Product code */
            ui_draw_text(80, 42, FONT_SIZE_SMALL, CLR_SUBTEXT,
                         game->product_code);

            /* Title ID */
            char tid_str[20];
            snprintf(tid_str, sizeof(tid_str), "%016llX",
                     (unsigned long long)game->title_id);
            ui_draw_text(160, 42, FONT_SIZE_SMALL, CLR_OVERLAY, tid_str);
        }

        /* Controls hint */
        ui_draw_text(10, content_h - 30, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     "A: Select  |  START: Exit");
        ui_draw_text(10, content_h - 16, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     lang_str(STR_SELECT_GAME));
    }
    else if (g_ctx.state == STATE_GAME_SELECTED) {
        /* Show save list for selected game */
        GameTitle *game = &g_ctx.games[g_ctx.selected_game];

        /* Header */
        char header[128];
        snprintf(header, sizeof(header), "%s %s",
                 lang_str(STR_SAVES_FOR), game->name);
        ui_draw_text_truncated(10, 6, FONT_SIZE_MED, CLR_ACCENT,
                               header, BOTTOM_SCREEN_WIDTH - 20);

        if (g_ctx.save_count == 0) {
            ui_draw_text_centered(80, FONT_SIZE_MED, CLR_SUBTEXT,
                                  lang_str(STR_NO_SAVES));
        } else {
            /* List saves */
            float list_y = 30.0f;
            for (int i = 0; i < g_ctx.save_count && list_y < content_h - 20; i++) {
                bool sel = (i == g_ctx.selected_save);
                u32 bg = sel ? CLR_ACCENT : CLR_SURFACE;
                u32 fg = sel ? CLR_BLACK : CLR_TEXT;

                ui_draw_rounded_rect(10, list_y,
                                     BOTTOM_SCREEN_WIDTH - 20, 32,
                                     4.0f, bg);

                /* Device name + date */
                char line[128];
                struct tm *tm_info = localtime(&g_ctx.saves[i].created_at);
                char date_str[20] = "???";
                if (tm_info) {
                    strftime(date_str, sizeof(date_str),
                             "%Y-%m-%d %H:%M", tm_info);
                }
                snprintf(line, sizeof(line), "%s  |  %s",
                         g_ctx.saves[i].device_name, date_str);
                ui_draw_text(18, list_y + 4, FONT_SIZE_SMALL, fg, line);

                /* Description preview */
                if (g_ctx.saves[i].description[0] != '\0') {
                    char desc_preview[40];
                    snprintf(desc_preview, sizeof(desc_preview), "%.36s",
                             g_ctx.saves[i].description);
                    ui_draw_text(18, list_y + 18, 0.35f,
                                 sel ? CLR_SURFACE : CLR_SUBTEXT,
                                 desc_preview);
                }

                list_y += 36.0f;
            }
        }

        /* Controls */
        ui_draw_text(10, content_h - 30, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     "A: Detail  Y: Upload  L: Download");
        ui_draw_text(10, content_h - 16, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     "R(3s): Delete  B: Back");
    }
    else if (g_ctx.state == STATE_SAVE_DETAIL && g_ctx.selected_save >= 0) {
        /* Detailed save info */
        SaveFileInfo *save = &g_ctx.saves[g_ctx.selected_save];

        ui_draw_text(10, 6, FONT_SIZE_MED, CLR_ACCENT,
                     lang_str(STR_SAVE_GAME));

        float y = 28.0f;
        float label_x = 10.0f;
        float value_x = 110.0f;
        float line_h = 22.0f;

        /* Game name */
        ui_draw_text(label_x, y, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     lang_str(STR_SAVE_GAME));
        ui_draw_text_truncated(value_x, y, FONT_SIZE_SMALL, CLR_TEXT,
                               save->game_name,
                               BOTTOM_SCREEN_WIDTH - value_x - 10);
        y += line_h;

        /* Device */
        ui_draw_text(label_x, y, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     lang_str(STR_SAVE_DEVICE));
        ui_draw_text(value_x, y, FONT_SIZE_SMALL, CLR_TEXT,
                     save->device_name);
        y += line_h;

        /* Created */
        ui_draw_text(label_x, y, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     lang_str(STR_SAVE_CREATED));
        struct tm *tm_info = localtime(&save->created_at);
        char date_str[32] = "???";
        if (tm_info) {
            strftime(date_str, sizeof(date_str),
                     "%Y-%m-%d %H:%M:%S", tm_info);
        }
        ui_draw_text(value_x, y, FONT_SIZE_SMALL, CLR_TEXT, date_str);
        y += line_h;

        /* Size */
        ui_draw_text(label_x, y, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     lang_str(STR_SAVE_SIZE));
        char size_str[32];
        if (save->file_size >= 1024 * 1024)
            snprintf(size_str, sizeof(size_str), "%.1f MB",
                     save->file_size / (1024.0 * 1024.0));
        else if (save->file_size >= 1024)
            snprintf(size_str, sizeof(size_str), "%.1f KB",
                     save->file_size / 1024.0);
        else
            snprintf(size_str, sizeof(size_str), "%zu B", save->file_size);
        ui_draw_text(value_x, y, FONT_SIZE_SMALL, CLR_TEXT, size_str);
        y += line_h;

        /* Region */
        ui_draw_text(label_x, y, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     lang_str(STR_SAVE_REGION));
        const char *reg = lang_str(STR_REGION_UNKNOWN);
        switch (save->region) {
        case REGION_EUR: reg = lang_str(STR_REGION_EUR); break;
        case REGION_USA: reg = lang_str(STR_REGION_USA); break;
        case REGION_JPN: reg = lang_str(STR_REGION_JPN); break;
        default: break;
        }
        ui_draw_text(value_x, y, FONT_SIZE_SMALL, CLR_TEXT, reg);
        y += line_h;

        /* Description */
        ui_draw_text(label_x, y, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     lang_str(STR_SAVE_DESCRIPTION));
        y += 16.0f;
        if (save->description[0] != '\0') {
            ui_draw_text(label_x + 4, y, FONT_SIZE_SMALL, CLR_TEXT,
                         save->description);
        } else {
            ui_draw_text(label_x + 4, y, FONT_SIZE_SMALL, CLR_OVERLAY,
                         "—");
        }

        /* Controls */
        ui_draw_text(10, content_h - 16, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     "L: Download  X: Edit Desc  B: Back");
    }

    /* Transition overlay */
    if (anim_active) {
        u32 overlay = C2D_Color32(30, 30, 46,
                                   (u32)(255 * anim_transition));
        C2D_DrawRectSolid(0, 0, 0.9f, BOTTOM_SCREEN_WIDTH,
                          BOTTOM_SCREEN_HEIGHT, overlay);
    }
}

/*═══════════════════════════════════════════════════════════════*
 *  Bottom Screen – Settings
 *═══════════════════════════════════════════════════════════════*/

/* Setting item IDs (order must match drawing order) */
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

void ui_draw_bottom_settings(void)
{
    C2D_TargetClear(bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(bottom_target);

    float content_h = BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT;
    float row_h     = 34.0f;
    float gap        = 3.0f;
    float pad        = 8.0f;
    float row_w      = BOTTOM_SCREEN_WIDTH - 2 * pad;
    float header_h   = 30.0f;

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

    /* Drawable area for settings rows */
    float draw_top    = header_h;
    float draw_bottom = hint_y - 2.0f;

    /* How many rows can fit on screen */
    int visible_rows = (int)((draw_bottom - draw_top) / (row_h + gap));
    if (visible_rows < 1) visible_rows = 1;

    /* Clamp scroll so selected item is visible */
    if (settings_selected < settings_scroll)
        settings_scroll = settings_selected;
    if (settings_selected >= settings_scroll + visible_rows)
        settings_scroll = settings_selected - visible_rows + 1;
    if (settings_scroll < 0) settings_scroll = 0;
    if (settings_scroll > SETTINGS_ITEM_COUNT - visible_rows)
        settings_scroll = SETTINGS_ITEM_COUNT - visible_rows;
    if (settings_scroll < 0) settings_scroll = 0;

    /* ── Draw each visible settings row ────────────────────────── */
    struct {
        const char *label;
        const char *value;
        u32 value_clr;
        char buf[32]; /* for formatted values */
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

        /* Skip rows outside visible area */
        if (ry + row_h < draw_top || ry > draw_bottom) continue;

        bool selected = (i == settings_selected);
        u32 bg_clr = selected ? CLR_BUTTON_HOVER : CLR_SURFACE;

        /* Connect button: special button-style rendering */
        if (i == SETTINGS_ITEM_CONNECT) {
            u32 btn_clr = selected ? CLR_ACCENT : C2D_Color32(0x30, 0x80, 0xE0, 0xFF);
            ui_draw_rounded_rect(pad, ry, row_w, row_h, 4.0f, btn_clr);
            ui_draw_text_centered(ry + 8, FONT_SIZE_SMALL, CLR_WHITE,
                                  items[i].label);
            continue;
        }

        /* Row background */
        ui_draw_rounded_rect(pad, ry, row_w, row_h, 4.0f, bg_clr);

        /* Selection indicator: left accent bar */
        if (selected) {
            C2D_DrawRectSolid(pad, ry + 4, 0.6f, 3.0f, row_h - 8, CLR_ACCENT);
        }

        /* Label (left side) */
        ui_draw_text(pad + 10, ry + 8, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     items[i].label);

        /* Value (right-aligned) */
        if (items[i].value) {
            /* Special: status row has a colored dot */
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

    /* Scroll indicator if needed */
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

/*═══════════════════════════════════════════════════════════════*
 *  Settings – Input handler
 *═══════════════════════════════════════════════════════════════*/

/** Edit the currently selected settings item (opens keyboard / toggles). */
static void settings_edit_item(int item)
{
    char buf[256] = {0};

    switch (item) {
    case SETTINGS_ITEM_DEVICE:
        if (config_keyboard_input(lang_str(STR_SETTINGS_DEVICE),
                g_ctx.config.device_name, buf, MAX_DEVICE_NAME)) {
            strncpy(g_ctx.config.device_name, buf, MAX_DEVICE_NAME - 1);
            config_save(&g_ctx.config);
        }
        break;

    case SETTINGS_ITEM_LANGUAGE:
        /* Cycle through languages */
        g_ctx.config.language = (Language)((g_ctx.config.language + 1) % LANG_COUNT);
        lang_set(g_ctx.config.language);
        config_save(&g_ctx.config);
        break;

    case SETTINGS_ITEM_TYPE:
        /* Cycle: None → SFTP → SMB → None */
        if (g_ctx.config.server.type == CONN_NONE)
            g_ctx.config.server.type = CONN_SFTP;
        else if (g_ctx.config.server.type == CONN_SFTP)
            g_ctx.config.server.type = CONN_SMB;
        else
            g_ctx.config.server.type = CONN_NONE;
        config_save(&g_ctx.config);
        break;

    case SETTINGS_ITEM_HOST:
        if (config_keyboard_input(lang_str(STR_SERVER_HOST),
                g_ctx.config.server.host, buf, MAX_HOST_LEN)) {
            strncpy(g_ctx.config.server.host, buf, MAX_HOST_LEN - 1);
            config_save(&g_ctx.config);
        }
        break;

    case SETTINGS_ITEM_PORT: {
        char port_buf[8];
        snprintf(port_buf, sizeof(port_buf), "%d", g_ctx.config.server.port);
        if (config_keyboard_input(lang_str(STR_SERVER_PORT),
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
        if (config_keyboard_input(lang_str(STR_SERVER_USER),
                g_ctx.config.server.username, buf, MAX_USER_LEN)) {
            strncpy(g_ctx.config.server.username, buf, MAX_USER_LEN - 1);
            config_save(&g_ctx.config);
        }
        break;

    case SETTINGS_ITEM_PASS:
        if (config_keyboard_input(lang_str(STR_SERVER_PASS),
                "", buf, MAX_PASS_LEN)) {
            strncpy(g_ctx.config.server.password, buf, MAX_PASS_LEN - 1);
            config_save(&g_ctx.config);
        }
        break;

    case SETTINGS_ITEM_PATH:
        /* Path is managed automatically by the server – not editable */
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
        /* Status row: no edit action */
        break;

    case SETTINGS_ITEM_CONNECT:
        /* Disconnect first if already connected */
        if (g_ctx.server_connected) {
            net_disconnect();
            g_ctx.server_connected = false;
        }
        /* Attempt connection */
        g_ctx.server_connected = net_connect(&g_ctx.config.server);
        break;
    }
}

bool ui_settings_handle_input(u32 kDown, touchPosition touch, bool touched)
{
    /* B = go back */
    if (kDown & KEY_B) return true;

    /* D-Pad navigation */
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

    /* A = edit selected item */
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

/*═══════════════════════════════════════════════════════════════*
 *  Bottom Screen – Sync All
 *═══════════════════════════════════════════════════════════════*/
void ui_draw_bottom_sync(void)
{
    C2D_TargetClear(bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(bottom_target);

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

/*═══════════════════════════════════════════════════════════════*
 *  Status Toast
 *═══════════════════════════════════════════════════════════════*/
void ui_draw_status_toast(void)
{
    if (g_ctx.status_message[0] == '\0') return;

    /* Draw on bottom screen (must switch scene) */
    /* We draw a floating toast near the top */
    float alpha = CLAMP(g_ctx.status_timer / 0.5f, 0.0f, 1.0f);
    u32 bg = C2D_Color32(49, 50, 68, (u32)(220 * alpha));
    u32 fg = C2D_Color32(205, 214, 244, (u32)(255 * alpha));

    float toast_w = BOTTOM_SCREEN_WIDTH - 40.0f;
    float toast_h = 28.0f;
    float toast_x = 20.0f;
    float toast_y = BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT - toast_h - 8.0f;

    /* We need to begin the bottom scene to draw on it.
       Since this is called after the main bottom draw, it layers on top. */
    C2D_SceneBegin(bottom_target);
    ui_draw_rounded_rect(toast_x, toast_y, toast_w, toast_h, 6.0f, bg);

    C2D_Text txt;
    C2D_TextFontParse(&txt, font, text_buf, g_ctx.status_message);
    C2D_TextOptimize(&txt);
    float tw, th;
    C2D_TextGetDimensions(&txt, FONT_SIZE_SMALL, FONT_SIZE_SMALL, &tw, &th);
    float tx = toast_x + (toast_w - tw) / 2.0f;
    float ty = toast_y + (toast_h - th) / 2.0f;
    C2D_DrawText(&txt, C2D_WithColor, tx, ty, 0.95f,
                 FONT_SIZE_SMALL, FONT_SIZE_SMALL, fg);
}

/*═══════════════════════════════════════════════════════════════*
 *  Navigation Bar
 *═══════════════════════════════════════════════════════════════*/
void ui_draw_navbar(void)
{
    C2D_SceneBegin(bottom_target);

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
        C2D_TextFontParse(&txt, font, text_buf, labels[i]);
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

/*═══════════════════════════════════════════════════════════════*
 *  Confirm Dialog (blocking)
 *═══════════════════════════════════════════════════════════════*/
bool ui_confirm_dialog(const char *title, const char *message)
{
    bool result = false;
    bool done = false;

    while (aptMainLoop() && !done) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_A) { result = true;  done = true; }
        if (kDown & KEY_B) { result = false; done = true; }

        C2D_TextBufClear(text_buf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(bottom_target, CLR_BACKGROUND);
        C2D_SceneBegin(bottom_target);

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
