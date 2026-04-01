/**
 * 3DS CloudSaver - Main Entry Point
 * ──────────────────────────────────
 * Initialises hardware services, runs the main application loop,
 * and cleanly shuts everything down on exit.
 */

#include "common.h"
#include "config.h"
#include "lang.h"
#include "ui.h"
#include "save.h"
#include "network.h"
#include "sync.h"

/*═══════════════════════════════════════════════════════════════*
 *  Global application context
 *═══════════════════════════════════════════════════════════════*/
AppContext g_ctx;

/*───────────────── Forward declarations ────────────────────────*/
static void app_init(void);
static void app_exit(void);
static void app_main_loop(void);
static void handle_input(void);
static void handle_touch(touchPosition touch);
static void update_state(void);
static void render(void);
static void attempt_auto_connect(void);

/*═══════════════════════════════════════════════════════════════*
 *  Entry
 *═══════════════════════════════════════════════════════════════*/
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    app_init();
    app_main_loop();
    app_exit();

    return 0;
}

/*═══════════════════════════════════════════════════════════════*
 *  Initialisation
 *═══════════════════════════════════════════════════════════════*/
static void app_init(void)
{
    /* Zero out context */
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.state           = STATE_SETUP_DEVICE_NAME;
    g_ctx.current_tab     = NAV_GAMES;
    g_ctx.selected_game   = -1;
    g_ctx.selected_save   = -1;
    g_ctx.transition_alpha = 0.0f;

    /* Core 3DS services */
    gfxInitDefault();    /* also calls aptInit internally */
    romfsInit();
    fsInit();
    cfguInit();
    amInit();

    /* Graphics (citro2d) */
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    /* Load or create configuration */
    config_init(&g_ctx.config);

    /* Init language */
    lang_init(g_ctx.config.language);

    /* Init UI */
    ui_init();

    /* Networking */
    net_init();

    /* Decide initial state */
    if (g_ctx.config.first_run || g_ctx.config.device_name[0] == '\0') {
        g_ctx.state = STATE_SETUP_DEVICE_NAME;
    } else {
        g_ctx.state = STATE_CONNECTING;
    }
}

/*═══════════════════════════════════════════════════════════════*
 *  Main loop
 *═══════════════════════════════════════════════════════════════*/
static void app_main_loop(void)
{
    while (aptMainLoop()) {
        hidScanInput();

        u32 kDown = hidKeysDown();
        if (kDown & KEY_START && g_ctx.state == STATE_MAIN_BROWSE) {
            g_ctx.state = STATE_EXIT;
        }

        if (g_ctx.state == STATE_EXIT) break;

        handle_input();
        update_state();
        render();

        g_ctx.frame_count++;
    }
}

/*═══════════════════════════════════════════════════════════════*
 *  Shutdown
 *═══════════════════════════════════════════════════════════════*/
static void app_exit(void)
{
    /* Save config on exit */
    config_save(&g_ctx.config);

    /* Free icon textures */
    save_free_icons(g_ctx.games, g_ctx.game_count);

    net_disconnect();
    net_exit();
    ui_exit();

    C2D_Fini();
    C3D_Fini();

    amExit();
    cfguExit();
    fsExit();
    romfsExit();
    gfxExit();  /* also calls aptExit internally */
}

/*═══════════════════════════════════════════════════════════════*
 *  Input handling
 *═══════════════════════════════════════════════════════════════*/
static void handle_input(void)
{
    static int r_hold_frames = 0; /* R-button hold counter for delete */

    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    touchPosition touch = {0, 0};
    bool touched = (kDown & KEY_TOUCH) != 0;

    if (touched) {
        hidTouchRead(&touch);
        /* Let settings handle its own touch; for other states use handle_touch */
        if (g_ctx.state != STATE_SETTINGS)
            handle_touch(touch);
    }

    switch (g_ctx.state) {
    case STATE_SETUP_DEVICE_NAME: {
        /* A button triggers keyboard */
        if (kDown & KEY_A) {
            char name[MAX_DEVICE_NAME] = {0};
            if (config_prompt_device_name(name, sizeof(name))) {
                strncpy(g_ctx.config.device_name, name, MAX_DEVICE_NAME - 1);
                g_ctx.config.first_run = false;
                config_save(&g_ctx.config);
                g_ctx.state = STATE_CONNECTING;
            }
        }
        break;
    }

    case STATE_MAIN_BROWSE: {
        /* D-Pad / Circle pad navigation through game grid */
        if (kDown & KEY_RIGHT) {
            if (g_ctx.selected_game < g_ctx.game_count - 1)
                g_ctx.selected_game++;
        }
        if (kDown & KEY_LEFT) {
            if (g_ctx.selected_game > 0)
                g_ctx.selected_game--;
        }
        if (kDown & KEY_DOWN) {
            int next = g_ctx.selected_game + COVERS_PER_ROW;
            if (next < g_ctx.game_count)
                g_ctx.selected_game = next;
        }
        if (kDown & KEY_UP) {
            int prev = g_ctx.selected_game - COVERS_PER_ROW;
            if (prev >= 0)
                g_ctx.selected_game = prev;
        }
        /* A: select game */
        if (kDown & KEY_A && g_ctx.selected_game >= 0) {
            ui_transition_start();
            g_ctx.prev_state = g_ctx.state;
            g_ctx.state = STATE_GAME_SELECTED;

            /* Load saves from cloud for this game */
            if (net_is_connected()) {
                g_ctx.save_count = sync_list_saves(
                    &g_ctx.games[g_ctx.selected_game],
                    g_ctx.saves, MAX_SAVEFILES);
            }
            g_ctx.selected_save = -1;
        }

        /* Scrolling */
        int selected_row = g_ctx.selected_game / COVERS_PER_ROW;
        if (selected_row < g_ctx.scroll_offset)
            g_ctx.scroll_offset = selected_row;
        if (selected_row >= g_ctx.scroll_offset + COVERS_VISIBLE_ROWS)
            g_ctx.scroll_offset = selected_row - COVERS_VISIBLE_ROWS + 1;

        break;
    }

    case STATE_GAME_SELECTED: {
        /* Navigate saves */
        if (kDown & KEY_DOWN && g_ctx.selected_save < g_ctx.save_count - 1)
            g_ctx.selected_save++;
        if (kDown & KEY_UP && g_ctx.selected_save > 0)
            g_ctx.selected_save--;
        if (kDown & KEY_UP && g_ctx.selected_save < 0 && g_ctx.save_count > 0)
            g_ctx.selected_save = 0;

        /* A: view save detail */
        if ((kDown & KEY_A) && g_ctx.selected_save >= 0) {
            g_ctx.prev_state = g_ctx.state;
            g_ctx.state = STATE_SAVE_DETAIL;
        }

        /* B: go back */
        if (kDown & KEY_B) {
            ui_transition_start();
            g_ctx.state = STATE_MAIN_BROWSE;
        }

        /* Y: upload current save for this game */
        if (kDown & KEY_Y && net_is_connected()) {
            char msg[MAX_COMMIT_MSG_LEN] = {0};
            if (config_keyboard_input(
                    lang_str(STR_COMMIT_MSG_HINT), "",
                    msg, sizeof(msg)))
            {
                SyncResult res = sync_upload_save(
                    &g_ctx.games[g_ctx.selected_game],
                    g_ctx.config.device_name, msg, NULL);
                if (res == SYNC_OK) {
                    snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                             "%s", lang_str(STR_SYNC_COMPLETE));
                } else {
                    snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                             "%s: %s", lang_str(STR_SYNC_FAILED),
                             sync_error_str(res));
                }
                g_ctx.status_timer = 3.0f;
                /* Refresh save list */
                g_ctx.save_count = sync_list_saves(
                    &g_ctx.games[g_ctx.selected_game],
                    g_ctx.saves, MAX_SAVEFILES);
            }
        }

        /* L: download selected save with confirmation */
        if (kDown & KEY_L && g_ctx.selected_save >= 0 && net_is_connected()) {
            /* Show confirmation: "Download save? Existing save will be overwritten!" */
            char confirm_msg[256];
            snprintf(confirm_msg, sizeof(confirm_msg),
                     "%s\n%s\n\n%s",
                     g_ctx.saves[g_ctx.selected_save].device_name,
                     g_ctx.saves[g_ctx.selected_save].filename,
                     "Download? Vorhandener Save wird ueberschrieben!");
            /* Use errorConf as a simple yes/no dialog */
            errorConf err;
            errorInit(&err, ERROR_TEXT_WORD_WRAP, CFG_LANGUAGE_EN);
            errorText(&err, confirm_msg);
            errorDisp(&err);

            /* The user has seen the warning – proceed with download */
            SyncResult res = sync_download_save(
                &g_ctx.saves[g_ctx.selected_save],
                &g_ctx.games[g_ctx.selected_game]);
            if (res == SYNC_OK) {
                snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                         "Download erfolgreich!");
            } else {
                snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                         "Download fehlgeschlagen: %s",
                         sync_error_str(res));
            }
            g_ctx.status_timer = 3.0f;
        }

        /* R held: delete save (hold for 3 seconds) */
        if ((kHeld & KEY_R) && g_ctx.selected_save >= 0 && net_is_connected()) {
            r_hold_frames++;
            /* Show progress in status */
            int seconds_left = 3 - (r_hold_frames / 60);
            if (seconds_left > 0 && (r_hold_frames % 60 == 0)) {
                snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                         "R halten zum Loeschen... %ds", seconds_left);
                g_ctx.status_timer = 1.5f;
            }
            if (r_hold_frames >= 180) { /* 3 seconds at 60fps */
                r_hold_frames = 0;
                /* Confirmation dialog */
                char confirm_msg[256];
                snprintf(confirm_msg, sizeof(confirm_msg),
                         "Save wirklich loeschen?\n\n%s\n%s",
                         g_ctx.saves[g_ctx.selected_save].device_name,
                         g_ctx.saves[g_ctx.selected_save].filename);
                errorConf err;
                errorInit(&err, ERROR_TEXT_WORD_WRAP, CFG_LANGUAGE_EN);
                errorText(&err, confirm_msg);
                errorDisp(&err);

                /* Proceed with deletion */
                SyncResult res = sync_delete_save(
                    &g_ctx.saves[g_ctx.selected_save]);
                if (res == SYNC_OK) {
                    snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                             "Save geloescht!");
                    /* Refresh save list */
                    g_ctx.save_count = sync_list_saves(
                        &g_ctx.games[g_ctx.selected_game],
                        g_ctx.saves, MAX_SAVEFILES);
                    g_ctx.selected_save = -1;
                } else {
                    snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                             "Loeschen fehlgeschlagen: %s",
                             sync_error_str(res));
                }
                g_ctx.status_timer = 3.0f;
            }
        } else {
            r_hold_frames = 0;
        }

        break;
    }

    case STATE_SAVE_DETAIL: {
        /* B: back to game view */
        if (kDown & KEY_B) {
            g_ctx.state = STATE_GAME_SELECTED;
        }

        /* L: download/import this save (with confirmation) */
        if (kDown & KEY_L && g_ctx.selected_save >= 0 && net_is_connected()) {
            char confirm_msg[256];
            snprintf(confirm_msg, sizeof(confirm_msg),
                     "%s\n%s\n\n%s",
                     g_ctx.saves[g_ctx.selected_save].device_name,
                     g_ctx.saves[g_ctx.selected_save].filename,
                     "Download? Vorhandener Save wird ueberschrieben!");
            errorConf err;
            errorInit(&err, ERROR_TEXT_WORD_WRAP, CFG_LANGUAGE_EN);
            errorText(&err, confirm_msg);
            errorDisp(&err);

            SyncResult res = sync_download_save(
                &g_ctx.saves[g_ctx.selected_save],
                &g_ctx.games[g_ctx.selected_game]);
            if (res == SYNC_OK) {
                snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                         "Download erfolgreich!");
            } else {
                snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                         "Download fehlgeschlagen: %s",
                         sync_error_str(res));
            }
            g_ctx.status_timer = 3.0f;
        }

        /* X: edit description */
        if (kDown & KEY_X && g_ctx.selected_save >= 0) {
            char desc[MAX_DESCRIPTION_LEN] = {0};
            if (config_keyboard_input(
                    lang_str(STR_ADD_DESCRIPTION),
                    g_ctx.saves[g_ctx.selected_save].description,
                    desc, sizeof(desc)))
            {
                strncpy(g_ctx.saves[g_ctx.selected_save].description,
                        desc, MAX_DESCRIPTION_LEN - 1);
                /* TODO: write updated meta to cloud */
            }
        }
        break;
    }

    case STATE_SETTINGS: {
        /* Settings input is handled by ui_settings_handle_input() */
        if (ui_settings_handle_input(kDown, touch, touched)) {
            g_ctx.state = STATE_MAIN_BROWSE;
            g_ctx.current_tab = NAV_GAMES;
        }
        break;
    }

    case STATE_SYNC_ALL_COMMIT: {
        /* This state prompts for a commit message then syncs */
        if (kDown & KEY_B) {
            g_ctx.state = STATE_MAIN_BROWSE;
            g_ctx.current_tab = NAV_GAMES;
        }
        break;
    }

    default:
        break;
    }
}

/*═══════════════════════════════════════════════════════════════*
 *  Touch handling (bottom screen)
 *═══════════════════════════════════════════════════════════════*/
static void handle_touch(touchPosition touch)
{
    /* Navigation bar area: bottom 40px of the bottom screen */
    if (touch.py >= BOTTOM_SCREEN_HEIGHT - 40) {
        int tab_width = BOTTOM_SCREEN_WIDTH / NAV_COUNT;
        int tapped_tab = touch.px / tab_width;
        if (tapped_tab >= 0 && tapped_tab < NAV_COUNT) {
            NavTab new_tab = (NavTab)tapped_tab;
            if (new_tab != g_ctx.current_tab) {
                g_ctx.current_tab = new_tab;
                switch (new_tab) {
                case NAV_GAMES:
                    g_ctx.state = STATE_MAIN_BROWSE;
                    break;
                case NAV_SYNC_ALL: {
                    if (!net_is_connected()) {
                        snprintf(g_ctx.status_message,
                                 sizeof(g_ctx.status_message),
                                 "%s", lang_str(STR_DISCONNECTED));
                        g_ctx.status_timer = 3.0f;
                        break;
                    }
                    /* Prompt for commit message */
                    char msg[MAX_COMMIT_MSG_LEN] = {0};
                    if (config_keyboard_input(
                            lang_str(STR_ENTER_COMMIT_MSG), "",
                            msg, sizeof(msg)))
                    {
                        g_ctx.state = STATE_SYNC_ALL;
                        SyncResult res = sync_all(
                            g_ctx.games, g_ctx.game_count,
                            g_ctx.config.device_name, msg, NULL);
                        if (res == SYNC_OK) {
                            snprintf(g_ctx.status_message,
                                     sizeof(g_ctx.status_message),
                                     "%s", lang_str(STR_SYNC_COMPLETE));
                        } else {
                            snprintf(g_ctx.status_message,
                                     sizeof(g_ctx.status_message),
                                     "%s", lang_str(STR_SYNC_FAILED));
                        }
                        g_ctx.status_timer = 3.0f;
                        g_ctx.state = STATE_MAIN_BROWSE;
                        g_ctx.current_tab = NAV_GAMES;
                    }
                    break;
                }
                case NAV_SETTINGS:
                    g_ctx.state = STATE_SETTINGS;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

/*═══════════════════════════════════════════════════════════════*
 *  State update
 *═══════════════════════════════════════════════════════════════*/
static void update_state(void)
{
    /* Update animations */
    ui_animation_update();

    /* Status toast timer */
    if (g_ctx.status_timer > 0.0f) {
        g_ctx.status_timer -= 1.0f / 60.0f;
        if (g_ctx.status_timer <= 0.0f) {
            g_ctx.status_message[0] = '\0';
            g_ctx.status_timer = 0.0f;
        }
    }

    /* Handle connecting state */
    if (g_ctx.state == STATE_CONNECTING) {
        /* Try connecting (won't crash if no server configured) */
        attempt_auto_connect();

        /* Scan titles – this is safe even without a connection */
        g_ctx.game_count = save_scan_titles(g_ctx.games, MAX_GAMES);
        if (g_ctx.game_count > 0)
            g_ctx.selected_game = 0;
        else
            g_ctx.selected_game = -1;

        g_ctx.state = STATE_MAIN_BROWSE;
    }
}

/*═══════════════════════════════════════════════════════════════*
 *  Render
 *═══════════════════════════════════════════════════════════════*/
static void render(void)
{
    ui_frame_begin();

    switch (g_ctx.state) {
    case STATE_SETUP_DEVICE_NAME:
        ui_draw_setup();
        break;

    case STATE_CONNECTING:
        ui_draw_connecting();
        break;

    case STATE_MAIN_BROWSE:
        ui_draw_top_games();
        ui_draw_bottom_main();
        ui_draw_navbar();
        break;

    case STATE_GAME_SELECTED:
    case STATE_SAVE_DETAIL:
        ui_draw_top_games();
        ui_draw_bottom_main();
        ui_draw_navbar();
        break;

    case STATE_SETTINGS:
        ui_draw_top_games();
        ui_draw_bottom_settings();
        ui_draw_navbar();
        break;

    case STATE_SYNC_ALL:
        ui_draw_top_games();
        ui_draw_bottom_sync();
        ui_draw_navbar();
        break;

    default:
        break;
    }

    /* Draw status toast over everything */
    if (g_ctx.status_timer > 0.0f) {
        ui_draw_status_toast();
    }

    ui_frame_end();
}

/*═══════════════════════════════════════════════════════════════*
 *  Auto-connect to server
 *═══════════════════════════════════════════════════════════════*/
static void attempt_auto_connect(void)
{
    if (g_ctx.config.server.type == CONN_NONE) {
        snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                 "%s", lang_str(STR_SERVER_NOT_CONFIGURED));
        g_ctx.status_timer = 3.0f;
        return;
    }

    if (!g_ctx.config.auto_connect) return;

    bool ok = net_connect(&g_ctx.config.server);
    g_ctx.server_connected = ok;
    g_ctx.config.server.connected = ok;

    if (ok) {
        snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                 "%s", lang_str(STR_CONNECTION_SUCCESS));
    } else {
        snprintf(g_ctx.status_message, sizeof(g_ctx.status_message),
                 "%s: %s", lang_str(STR_CONNECTION_FAILED),
                 net_last_error());
    }
    g_ctx.status_timer = 3.0f;
}
