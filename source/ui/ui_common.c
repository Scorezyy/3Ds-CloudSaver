/*
 * ui_common.c — Shared UI state, init/exit, text helpers, draw primitives.
 */

#include "ui.h"
#include "ui_common.h"
#include "lang.h"

/* Shared render state */
C3D_RenderTarget *ui_top_target    = NULL;
C3D_RenderTarget *ui_bottom_target = NULL;
C2D_TextBuf       ui_text_buf      = NULL;
C2D_Font          ui_font          = NULL;

/* Animation state */
float ui_anim_transition = 0.0f;
bool  ui_anim_active     = false;

/* Init / Exit */

bool ui_init(void)
{
    ui_top_target    = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    ui_bottom_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    ui_text_buf      = C2D_TextBufNew(8192);
    ui_font          = C2D_FontLoadSystem(CFG_REGION_USA);
    return true;
}

void ui_exit(void)
{
    if (ui_font)     { C2D_FontFree(ui_font); ui_font = NULL; }
    if (ui_text_buf) { C2D_TextBufDelete(ui_text_buf); ui_text_buf = NULL; }
}

C3D_RenderTarget *ui_get_top_target(void)    { return ui_top_target; }
C3D_RenderTarget *ui_get_bottom_target(void) { return ui_bottom_target; }

/* Frame management */

void ui_frame_begin(void)
{
    C2D_TextBufClear(ui_text_buf);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
}

void ui_frame_end(void)
{
    C3D_FrameEnd(0);
}

/* Text helpers */

void ui_draw_text(float x, float y, float size, u32 clr, const char *text)
{
    if (!text || !ui_text_buf) return;
    C2D_Text txt;
    if (ui_font)
        C2D_TextFontParse(&txt, ui_font, ui_text_buf, text);
    else
        C2D_TextParse(&txt, ui_text_buf, text);
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, C2D_WithColor, x, y, 0.5f, size, size, clr);
}

void ui_draw_text_truncated(float x, float y, float size, u32 clr,
                            const char *text, float max_width)
{
    if (!text || !ui_text_buf) return;

    C2D_Text txt;
    if (ui_font)
        C2D_TextFontParse(&txt, ui_font, ui_text_buf, text);
    else
        C2D_TextParse(&txt, ui_text_buf, text);
    C2D_TextOptimize(&txt);

    float w, h;
    C2D_TextGetDimensions(&txt, size, size, &w, &h);
    if (w <= max_width) {
        C2D_DrawText(&txt, C2D_WithColor, x, y, 0.5f, size, size, clr);
        return;
    }

    /* Binary search for truncation point (UTF-8 safe) */
    char buf[128];
    int len = (int)strlen(text);
    if (len > (int)sizeof(buf) - 4) len = (int)sizeof(buf) - 4;

    int lo = 0, hi = len, best = 0;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int pos = mid;
        while (pos > 0 && ((unsigned char)text[pos] & 0xC0) == 0x80) pos--;

        memcpy(buf, text, pos);
        buf[pos] = '\0';
        strcat(buf, "...");

        C2D_Text t2;
        if (ui_font)
            C2D_TextFontParse(&t2, ui_font, ui_text_buf, buf);
        else
            C2D_TextParse(&t2, ui_text_buf, buf);
        C2D_TextOptimize(&t2);

        float tw, th;
        C2D_TextGetDimensions(&t2, size, size, &tw, &th);
        if (tw <= max_width) { best = pos; lo = mid + 1; }
        else                 { hi = mid - 1; }
    }

    memcpy(buf, text, best);
    buf[best] = '\0';
    strcat(buf, "...");

    C2D_Text t3;
    if (ui_font)
        C2D_TextFontParse(&t3, ui_font, ui_text_buf, buf);
    else
        C2D_TextParse(&t3, ui_text_buf, buf);
    C2D_TextOptimize(&t3);
    C2D_DrawText(&t3, C2D_WithColor, x, y, 0.5f, size, size, clr);
}

void ui_draw_text_centered(float y, float size, u32 clr, const char *text)
{
    if (!text || !ui_text_buf) return;
    C2D_Text txt;
    if (ui_font)
        C2D_TextFontParse(&txt, ui_font, ui_text_buf, text);
    else
        C2D_TextParse(&txt, ui_text_buf, text);
    C2D_TextOptimize(&txt);
    float w, h;
    C2D_TextGetDimensions(&txt, size, size, &w, &h);
    C2D_DrawText(&txt, C2D_WithColor,
                 (BOTTOM_SCREEN_WIDTH - w) / 2.0f, y, 0.5f,
                 size, size, clr);
}

void ui_draw_text_centered_top(float y, float size, u32 clr, const char *text)
{
    if (!text || !ui_text_buf) return;
    C2D_Text txt;
    if (ui_font)
        C2D_TextFontParse(&txt, ui_font, ui_text_buf, text);
    else
        C2D_TextParse(&txt, ui_text_buf, text);
    C2D_TextOptimize(&txt);
    float w, h;
    C2D_TextGetDimensions(&txt, size, size, &w, &h);
    C2D_DrawText(&txt, C2D_WithColor,
                 (TOP_SCREEN_WIDTH - w) / 2.0f, y, 0.5f,
                 size, size, clr);
}

/* Draw primitives */

void ui_draw_rounded_rect(float x, float y, float w, float h, float r, u32 clr)
{
    C2D_DrawRectSolid(x + r, y, 0.5f, w - 2*r, h, clr);
    C2D_DrawRectSolid(x, y + r, 0.5f, r, h - 2*r, clr);
    C2D_DrawRectSolid(x + w - r, y + r, 0.5f, r, h - 2*r, clr);
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
            touch->py >= y && touch->py <= y + h) {
            draw_bg = CLR_BUTTON_HOVER;
            hit = true;
        }
    }

    ui_draw_rounded_rect(x, y, w, h, 4.0f, draw_bg);

    C2D_Text txt;
    if (ui_font)
        C2D_TextFontParse(&txt, ui_font, ui_text_buf, label);
    else
        C2D_TextParse(&txt, ui_text_buf, label);
    C2D_TextOptimize(&txt);
    float tw, th;
    C2D_TextGetDimensions(&txt, FONT_SIZE_SMALL, FONT_SIZE_SMALL, &tw, &th);
    C2D_DrawText(&txt, C2D_WithColor,
                 x + (w - tw) / 2.0f, y + (h - th) / 2.0f, 0.5f,
                 FONT_SIZE_SMALL, FONT_SIZE_SMALL, fg);
    return hit;
}

void ui_draw_progress(float x, float y, float w, float h,
                      float progress, u32 bg, u32 fg)
{
    C2D_DrawRectSolid(x, y, 0.5f, w, h, bg);
    C2D_DrawRectSolid(x, y, 0.5f, w * CLAMP(progress, 0.0f, 1.0f), h, fg);
}

/* Animations */

void ui_transition_start(void)
{
    ui_anim_transition = 1.0f;
    ui_anim_active = true;
}

void ui_animation_update(void)
{
    if (ui_anim_active) {
        ui_anim_transition -= 0.05f;
        if (ui_anim_transition <= 0.0f) {
            ui_anim_transition = 0.0f;
            ui_anim_active = false;
        }
        g_ctx.transition_alpha = ui_anim_transition;
    }
}

bool ui_is_transitioning(void) { return ui_anim_active; }

/* Transition overlay (reusable by any screen) */

void ui_draw_transition_overlay(float screen_w, float screen_h)
{
    if (!ui_anim_active) return;
    u32 overlay = C2D_Color32(30, 30, 46, (u32)(255 * ui_anim_transition));
    C2D_DrawRectSolid(0, 0, 0.9f, screen_w, screen_h, overlay);
}

/* Region string helper */

const char *ui_region_str(GameRegion region)
{
    switch (region) {
    case REGION_EUR: return lang_str(STR_REGION_EUR);
    case REGION_USA: return lang_str(STR_REGION_USA);
    case REGION_JPN: return lang_str(STR_REGION_JPN);
    default:         return lang_str(STR_REGION_UNKNOWN);
    }
}
