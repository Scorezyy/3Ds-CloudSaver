/*
 * ui_savelist.c — Bottom screen save list & detail views.
 *
 * Handles STATE_MAIN_BROWSE, STATE_GAME_SELECTED, STATE_SAVE_DETAIL.
 */

#include "ui.h"
#include "ui_common.h"
#include "lang.h"

void ui_draw_bottom_main(void)
{
    C2D_TargetClear(ui_bottom_target, CLR_BACKGROUND);
    C2D_SceneBegin(ui_bottom_target);

    float content_h = BOTTOM_SCREEN_HEIGHT - NAVBAR_HEIGHT;

    if (g_ctx.state == STATE_MAIN_BROWSE) {
        if (g_ctx.selected_game >= 0 && g_ctx.selected_game < g_ctx.game_count) {
            GameTitle *game = &g_ctx.games[g_ctx.selected_game];

            /* Game info card */
            ui_draw_rounded_rect(10, 10, BOTTOM_SCREEN_WIDTH - 20, 60,
                                 6.0f, CLR_SURFACE);
            ui_draw_text_truncated(20, 16, FONT_SIZE_MED, CLR_TEXT,
                                   game->name, BOTTOM_SCREEN_WIDTH - 40);

            /* Region badge */
            ui_draw_text(20, 42, FONT_SIZE_SMALL, CLR_SUBTEXT,
                         ui_region_str(game->region));

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
        ui_draw_text(value_x, y, FONT_SIZE_SMALL, CLR_TEXT,
                     ui_region_str(save->region));
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
                         "\xE2\x80\x94");
        }

        /* Controls */
        ui_draw_text(10, content_h - 16, FONT_SIZE_SMALL, CLR_SUBTEXT,
                     "L: Download  X: Edit Desc  B: Back");
    }

    /* Transition overlay */
    ui_draw_transition_overlay(BOTTOM_SCREEN_WIDTH, BOTTOM_SCREEN_HEIGHT);
}
