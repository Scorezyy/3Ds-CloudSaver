/*
 * ui_common.h — Shared UI state, primitives, and text helpers.
 * Included by all ui_*.c files. Not for external use — use ui.h instead.
 */

#ifndef CLOUDSAVER_UI_COMMON_H
#define CLOUDSAVER_UI_COMMON_H

#include "common.h"

/* Shared render state (owned by ui_common.c) */
extern C3D_RenderTarget *ui_top_target;
extern C3D_RenderTarget *ui_bottom_target;
extern C2D_TextBuf       ui_text_buf;
extern C2D_Font          ui_font;

/* Animation state */
extern float ui_anim_transition;
extern bool  ui_anim_active;

/* Layout constants */
#define NAVBAR_HEIGHT   40
#define FONT_SIZE_SMALL 0.45f
#define FONT_SIZE_MED   0.55f
#define FONT_SIZE_LARGE 0.70f
#define FONT_SIZE_TITLE 0.85f

/* Draw text truncated to max_width, appending "..." if needed. */
void ui_draw_text_truncated(float x, float y, float size, u32 clr,
                            const char *text, float max_width);

/* Draw a transition overlay on a given screen. */
void ui_draw_transition_overlay(float screen_w, float screen_h);

/* Region string helper (used by savelist and settings). */
const char *ui_region_str(GameRegion region);

#endif /* CLOUDSAVER_UI_COMMON_H */
