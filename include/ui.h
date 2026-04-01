/**
 * 3DS CloudSaver - UI / Graphics Module
 * Handles all rendering on both screens.
 */

#ifndef CLOUDSAVER_UI_H
#define CLOUDSAVER_UI_H

#include "common.h"

/*─────────────── Initialization ────────────────*/
/** Initialise citro2d, load fonts, prepare render targets. */
bool ui_init(void);

/** Free all UI resources. */
void ui_exit(void);

/*─────────────── Main Render Loop ──────────────*/
/** Start a new frame. */
void ui_frame_begin(void);

/** Finish the current frame and present. */
void ui_frame_end(void);

/*─────────────── Screen Renderers ──────────────*/
/** Draw the device-name setup screen (bottom). */
void ui_draw_setup(void);

/** Draw the top screen – game cover grid. */
void ui_draw_top_games(void);

/** Draw the bottom screen – save list / details. */
void ui_draw_bottom_main(void);

/** Draw the bottom screen – settings page. */
void ui_draw_bottom_settings(void);

/** Draw the bottom screen – sync-all screen. */
void ui_draw_bottom_sync(void);

/** Draw the connecting overlay. */
void ui_draw_connecting(void);

/** Draw status toast message. */
void ui_draw_status_toast(void);

/** Draw the navigation bar on the bottom screen. */
void ui_draw_navbar(void);

/*─────────────── Animations ────────────────────*/
/** Start a screen transition animation. */
void ui_transition_start(void);

/** Update animation timers (call each frame). */
void ui_animation_update(void);

/** Is a transition currently playing? */
bool ui_is_transitioning(void);

/*─────────────── UI Helpers ────────────────────*/
/** Draw a rounded rectangle. */
void ui_draw_rounded_rect(float x, float y, float w, float h, float r, u32 clr);

/** Draw text centred horizontally. */
void ui_draw_text_centered(float y, float size, u32 clr, const char *text);

/** Draw text at position. */
void ui_draw_text(float x, float y, float size, u32 clr, const char *text);

/** Draw a button (returns true if within touch area). */
bool ui_draw_button(float x, float y, float w, float h,
                    const char *label, u32 bg, u32 fg,
                    touchPosition *touch, bool touched);

/** Draw a progress bar. */
void ui_draw_progress(float x, float y, float w, float h,
                      float progress, u32 bg, u32 fg);

/** Handle input in settings screen (D-Pad, A, touch).
 *  Returns true if the user pressed B (wants to go back). */
bool ui_settings_handle_input(u32 kDown, touchPosition touch, bool touched);

/** Show a confirmation dialog. Returns true if user pressed A/Yes. */
bool ui_confirm_dialog(const char *title, const char *message);

#endif /* CLOUDSAVER_UI_H */
