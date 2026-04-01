/**
 * 3DS CloudSaver - Loading Screen Module
 * ───────────────────────────────────────
 * Handles the startup loading screen with:
 *  - Top screen:    Game scan progress bar + count
 *  - Bottom screen: Cloud connection status (parallel)
 *
 * The scan is done incrementally: one title per frame so the UI
 * stays responsive and the progress bar updates smoothly.
 */

#ifndef CLOUDSAVER_LOADING_H
#define CLOUDSAVER_LOADING_H

#include "common.h"

/*─────────────────── Loading Phase ─────────────────────────────*/
typedef enum {
    LOAD_PHASE_INIT = 0,    /* Allocating title list from AM */
    LOAD_PHASE_SCANNING,    /* Scanning one title per frame */
    LOAD_PHASE_DONE         /* All titles scanned, ready */
} LoadPhase;

/*─────────────────── Cloud Connect State ───────────────────────*/
typedef enum {
    CLOUD_IDLE = 0,         /* Not started */
    CLOUD_CONNECTING,       /* Attempting connection */
    CLOUD_CONNECTED,        /* Successfully connected */
    CLOUD_FAILED,           /* Connection failed */
    CLOUD_SKIPPED           /* No server configured / auto-connect off */
} CloudState;

/*─────────────────── Loading Context ───────────────────────────*/
typedef struct {
    /* Title scanning */
    LoadPhase   phase;
    u64        *title_ids;      /* All SD title IDs (allocated once) */
    u32         total_titles;   /* Total number of SD titles */
    u32         scan_index;     /* Current index being scanned */
    int         found_games;    /* Number of valid games found so far */

    /* Cloud connection */
    CloudState  cloud_state;
    char        cloud_msg[128]; /* Status message for bottom screen */
    bool        cloud_started;  /* Has the connection attempt been kicked off? */

    /* Animation */
    float       spinner_angle;  /* Rotating spinner for visual feedback */
} LoadingContext;

/*─────────────────── API ───────────────────────────────────────*/

/** Initialise the loading screen (call once when entering STATE_LOADING). */
void loading_init(LoadingContext *lctx);

/** Process one step of the loading sequence.
 *  Call this once per frame while in STATE_LOADING.
 *  Returns true when loading is completely finished. */
bool loading_update(LoadingContext *lctx);

/** Draw the top screen (progress bar, game count, spinner). */
void loading_draw_top(const LoadingContext *lctx);

/** Draw the bottom screen (cloud connection status). */
void loading_draw_bottom(const LoadingContext *lctx);

/** Clean up any resources allocated during loading. */
void loading_cleanup(LoadingContext *lctx);

#endif /* CLOUDSAVER_LOADING_H */
