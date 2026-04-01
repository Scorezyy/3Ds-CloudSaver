/*
 * ui_games.c — Top screen game cover grid.
 */

#include "ui.h"
#include "ui_common.h"
#include "lang.h"

void ui_draw_top_games(void)
{
    C2D_TargetClear(ui_top_target, CLR_BACKGROUND);
    C2D_SceneBegin(ui_top_target);

    /* Header */
    C2D_DrawRectSolid(0, 0, 0.5f, TOP_SCREEN_WIDTH, 28.0f, CLR_SURFACE);
    ui_draw_text_centered_top(4.0f, FONT_SIZE_MED, CLR_TEXT,
                              lang_str(STR_INSTALLED_GAMES));

    /* Connection indicator */
    u32 status_clr = g_ctx.server_connected ? CLR_SUCCESS : CLR_ERROR;
    C2D_DrawCircleSolid(TOP_SCREEN_WIDTH - 16, 14, 0.5f, 5, status_clr);

    /* Game count */
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d", g_ctx.game_count);
    ui_draw_text(8.0f, 6.0f, FONT_SIZE_SMALL, CLR_SUBTEXT, count_str);

    if (g_ctx.game_count == 0) {
        ui_draw_text_centered_top(120.0f, FONT_SIZE_MED, CLR_SUBTEXT,
                                  lang_str(STR_NO_GAMES_FOUND));
        return;
    }

    /* Draw game grid */
    float grid_start_y = 34.0f;
    float total_cover_w = COVER_WIDTH + COVER_PADDING;
    float total_cover_h = COVER_HEIGHT + COVER_PADDING;
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
    ui_draw_transition_overlay(TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT);
}
