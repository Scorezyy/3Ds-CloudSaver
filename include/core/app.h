#ifndef CLOUDSAVER_APP_H
#define CLOUDSAVER_APP_H

#include "types.h"

/* Global application context */
typedef struct {
    AppState state;
    AppState prev_state;
    AppConfig config;

    /* Games */
    GameTitle games[MAX_GAMES];
    int game_count;
    int selected_game;
    int scroll_offset;

    /* Saves */
    SaveFileInfo saves[MAX_SAVEFILES];
    int save_count;
    int selected_save;

    /* Navigation */
    NavTab current_tab;

    /* Animation */
    float transition_alpha;
    float transition_target;
    bool transitioning;

    /* Connection */
    bool server_connected;
    char status_message[256];
    float status_timer;

    /* Frame counter */
    u32 frame_count;
} AppContext;

extern AppContext g_ctx;

#endif /* CLOUDSAVER_APP_H */
