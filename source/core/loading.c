/**
 * 3DS CloudSaver - Loading Screen Implementation
 *
 * Provides a non-blocking startup sequence:
 *  - Fetches the title list once (LOAD_PHASE_INIT)
 *  - Scans one title per frame (LOAD_PHASE_SCANNING)
 *  - Attempts cloud connection in parallel on bottom screen
 *
 * Top screen:    "Lade Spiele X/Y" + progress bar + spinner
 * Bottom screen: Cloud connection status + animated indicator
 */

#include "loading.h"
#include "save.h"
#include "network.h"
#include "config.h"
#include "lang.h"
#include "ui.h"

#include <math.h>

/* Init */
void loading_init(LoadingContext *lctx)
{
    memset(lctx, 0, sizeof(LoadingContext));
    lctx->phase         = LOAD_PHASE_INIT;
    lctx->cloud_state   = CLOUD_IDLE;
    lctx->cloud_started = false;
    lctx->spinner_angle = 0.0f;
    snprintf(lctx->cloud_msg, sizeof(lctx->cloud_msg), "Warte...");
}

/* Update — call once per frame */
bool loading_update(LoadingContext *lctx)
{
    lctx->spinner_angle += 4.0f;
    if (lctx->spinner_angle >= 360.0f)
        lctx->spinner_angle -= 360.0f;

    /* ── Cloud connection (kicked off once, non-blocking style) ── */
    if (!lctx->cloud_started) {
        lctx->cloud_started = true;

        if (g_ctx.config.server.type == CONN_NONE) {
            lctx->cloud_state = CLOUD_SKIPPED;
            snprintf(lctx->cloud_msg, sizeof(lctx->cloud_msg),
                     "%s", lang_str(STR_SERVER_NOT_CONFIGURED));
        } else if (!g_ctx.config.auto_connect) {
            lctx->cloud_state = CLOUD_SKIPPED;
            snprintf(lctx->cloud_msg, sizeof(lctx->cloud_msg),
                     "Auto-Connect deaktiviert");
        } else {
            lctx->cloud_state = CLOUD_CONNECTING;
            snprintf(lctx->cloud_msg, sizeof(lctx->cloud_msg),
                     "%s %s:%d ...",
                     lang_str(STR_CONNECTING),
                     g_ctx.config.server.host,
                     g_ctx.config.server.port);

            /* Attempt connection (blocking, but only ~1-2s typically) */
            bool ok = net_connect(&g_ctx.config.server);
            g_ctx.server_connected           = ok;
            g_ctx.config.server.connected    = ok;

            if (ok) {
                lctx->cloud_state = CLOUD_CONNECTED;
                snprintf(lctx->cloud_msg, sizeof(lctx->cloud_msg),
                         "%s", lang_str(STR_CONNECTION_SUCCESS));
            } else {
                lctx->cloud_state = CLOUD_FAILED;
                snprintf(lctx->cloud_msg, sizeof(lctx->cloud_msg),
                         "%s: %s", lang_str(STR_CONNECTION_FAILED),
                         net_last_error());
            }
        }
    }

    switch (lctx->phase) {

    case LOAD_PHASE_INIT: {
        /* Fetch the full list of SD title IDs (fast, < 1ms) */
        lctx->total_titles = save_get_title_ids(&lctx->title_ids);
        lctx->scan_index   = 0;
        lctx->found_games  = 0;

        if (lctx->total_titles == 0 || !lctx->title_ids) {
            /* No titles installed – skip straight to done */
            lctx->phase = LOAD_PHASE_DONE;
        } else {
            lctx->phase = LOAD_PHASE_SCANNING;
        }
        break;
    }

    case LOAD_PHASE_SCANNING: {
        /* Scan a batch of titles per frame for speed.
         * SMDH reads are ~5-10ms each, doing 3-4 per frame keeps
         * us well under the 16ms budget while being 3-4× faster. */
        int per_frame = 4;

        for (int n = 0; n < per_frame && lctx->scan_index < lctx->total_titles; n++) {
            if (lctx->found_games >= MAX_GAMES) {
                lctx->scan_index = lctx->total_titles; /* stop */
                break;
            }

            u64 tid = lctx->title_ids[lctx->scan_index];
            GameTitle tmp;

            if (save_scan_one_title(tid, &tmp)) {
                g_ctx.games[lctx->found_games] = tmp;
                lctx->found_games++;
            }

            lctx->scan_index++;
        }

        /* Check if we're done */
        if (lctx->scan_index >= lctx->total_titles) {
            lctx->phase = LOAD_PHASE_DONE;
        }
        break;
    }

    case LOAD_PHASE_DONE:
        /* Insert cartridge at position 0 if present */
        {
            GameTitle cart;
            if (cart_read_info(&cart)) {
                /* Shift all existing games right by one */
                if (lctx->found_games < MAX_GAMES) {
                    memmove(&g_ctx.games[1], &g_ctx.games[0],
                            lctx->found_games * sizeof(GameTitle));
                    g_ctx.games[0] = cart;
                    lctx->found_games++;
                    g_ctx.cart_title_id = cart.title_id;
                } else {
                    /* At max — replace last entry */
                    memmove(&g_ctx.games[1], &g_ctx.games[0],
                            (MAX_GAMES - 1) * sizeof(GameTitle));
                    g_ctx.games[0] = cart;
                    g_ctx.cart_title_id = cart.title_id;
                }
            }
        }

        /* Finalise */
        g_ctx.game_count = lctx->found_games;
        g_ctx.selected_game = (g_ctx.game_count > 0) ? 0 : -1;
        return true; /* Loading finished! */
    }

    return false;
}

/* Draw – Top Screen (progress bar, game count, spinner) */
void loading_draw_top(const LoadingContext *lctx)
{
    ui_draw_text_centered_top(40.0f, 0.85f, CLR_ACCENT,
                              lang_str(STR_APP_TITLE));

    char status[64];
    if (lctx->phase == LOAD_PHASE_INIT) {
        snprintf(status, sizeof(status), "Initialisiere...");
    } else if (lctx->phase == LOAD_PHASE_SCANNING) {
        snprintf(status, sizeof(status), "Lade Spiele  %d / %lu",
                 lctx->found_games, (unsigned long)lctx->total_titles);
    } else {
        snprintf(status, sizeof(status), "%d Spiele geladen!",
                 lctx->found_games);
    }
    ui_draw_text_centered_top(95.0f, 0.55f, CLR_TEXT, status);

    float bar_x = 50.0f;
    float bar_w = TOP_SCREEN_WIDTH - 100.0f;
    float bar_y = 130.0f;
    float bar_h = 10.0f;

    float progress = 0.0f;
    if (lctx->total_titles > 0)
        progress = (float)lctx->scan_index / (float)lctx->total_titles;

    /* Background */
    C2D_DrawRectSolid(bar_x, bar_y, 0.5f, bar_w, bar_h, CLR_SURFACE);
    /* Fill */
    float fill_w = bar_w * CLAMP(progress, 0.0f, 1.0f);
    if (fill_w > 0.0f)
        C2D_DrawRectSolid(bar_x, bar_y, 0.5f, fill_w, bar_h, CLR_ACCENT);

    char pct[16];
    snprintf(pct, sizeof(pct), "%d%%", (int)(progress * 100.0f));
    ui_draw_text_centered_top(145.0f, 0.45f, CLR_SUBTEXT, pct);

    float cx = TOP_SCREEN_WIDTH / 2.0f;
    float cy = 190.0f;
    float sz = 8.0f;
    float rad = lctx->spinner_angle * (M_PI / 180.0f);

    /* Draw 4 dots in a circle, one highlighted */
    for (int i = 0; i < 4; i++) {
        float angle = rad + (float)i * (M_PI / 2.0f);
        float dx = cosf(angle) * 14.0f;
        float dy = sinf(angle) * 14.0f;
        u32 clr = (i == 0) ? CLR_ACCENT : CLR_OVERLAY;
        C2D_DrawRectSolid(cx + dx - sz/2, cy + dy - sz/2, 0.5f, sz, sz, clr);
    }

    if (lctx->phase == LOAD_PHASE_SCANNING && lctx->found_games > 0) {
        const char *last_name = g_ctx.games[lctx->found_games - 1].name;
        ui_draw_text_centered_top(115.0f, 0.40f, CLR_SUBTEXT, last_name);
    }
}

/* Draw – Bottom Screen (cloud connection status) */
void loading_draw_bottom(const LoadingContext *lctx)
{
    C2D_DrawRectSolid(0, 0, 0.5f, BOTTOM_SCREEN_WIDTH, 28.0f, CLR_SURFACE);
    ui_draw_text_centered(4.0f, 0.55f, CLR_TEXT, "Cloud Verbindung");

    float icon_y = 80.0f;
    float icon_x = BOTTOM_SCREEN_WIDTH / 2.0f;

    u32 status_clr;
    const char *status_icon;
    switch (lctx->cloud_state) {
    case CLOUD_IDLE:
    case CLOUD_CONNECTING:
        status_clr  = CLR_WARNING;
        status_icon = "...";
        break;
    case CLOUD_CONNECTED:
        status_clr  = CLR_SUCCESS;
        status_icon = "OK";
        break;
    case CLOUD_FAILED:
        status_clr  = CLR_ERROR;
        status_icon = "X";
        break;
    case CLOUD_SKIPPED:
        status_clr  = CLR_OVERLAY;
        status_icon = "-";
        break;
    default:
        status_clr  = CLR_OVERLAY;
        status_icon = "?";
        break;
    }

    /* Large status circle */
    C2D_DrawCircleSolid(icon_x, icon_y, 0.5f, 24.0f, status_clr);
    /* Status letter inside circle */
    ui_draw_text(icon_x - 8.0f, icon_y - 10.0f, 0.55f, CLR_BLACK, status_icon);

    ui_draw_text_centered(120.0f, 0.50f, CLR_TEXT, lctx->cloud_msg);

    if (g_ctx.config.server.host[0] != '\0') {
        char info[128];
        snprintf(info, sizeof(info), "%s:%d",
                 g_ctx.config.server.host, g_ctx.config.server.port);
        ui_draw_text_centered(145.0f, 0.40f, CLR_SUBTEXT, info);
    }

    if (lctx->cloud_state == CLOUD_CONNECTING) {
        int dots = ((int)(lctx->spinner_angle / 30.0f)) % 4;
        char anim[16];
        snprintf(anim, sizeof(anim), "%.*s", dots, "...");
        ui_draw_text_centered(170.0f, 0.45f, CLR_ACCENT, anim);
    }

    char ver[32];
    snprintf(ver, sizeof(ver), "v%s", APP_VERSION_STRING);
    ui_draw_text_centered(BOTTOM_SCREEN_HEIGHT - 20.0f, 0.35f,
                          CLR_OVERLAY, ver);
}

/* Cleanup */
void loading_cleanup(LoadingContext *lctx)
{
    if (lctx->title_ids) {
        free(lctx->title_ids);
        lctx->title_ids = NULL;
    }
}
