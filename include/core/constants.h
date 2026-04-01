#ifndef CLOUDSAVER_CONSTANTS_H
#define CLOUDSAVER_CONSTANTS_H

#include <citro2d.h>

/* ── Version ── */

#define APP_VERSION_MAJOR   1
#define APP_VERSION_MINOR   0
#define APP_VERSION_PATCH   0
#define APP_VERSION_STRING  "1.0.0"

/* ── Limits ── */

#define MAX_DEVICE_NAME     32
#define MAX_GAMES           128
#define MAX_SAVEFILES       32
#define MAX_PATH_LEN        256
#define MAX_DESCRIPTION_LEN 128
#define MAX_COMMIT_MSG_LEN  64
#define MAX_HOST_LEN        128
#define MAX_USER_LEN        64
#define MAX_PASS_LEN        64

/* ── Screen ── */

#define TOP_SCREEN_WIDTH    400
#define TOP_SCREEN_HEIGHT   240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240

#define COVER_WIDTH         48
#define COVER_HEIGHT        48
#define COVER_PADDING       8
#define COVERS_PER_ROW      6
#define COVERS_VISIBLE_ROWS 3

/* ── Colors (Catppuccin Mocha) ── */

#define CLR_BACKGROUND      C2D_Color32(30, 30, 46, 255)
#define CLR_SURFACE         C2D_Color32(49, 50, 68, 255)
#define CLR_OVERLAY         C2D_Color32(108, 112, 134, 255)
#define CLR_TEXT            C2D_Color32(205, 214, 244, 255)
#define CLR_SUBTEXT         C2D_Color32(166, 173, 200, 255)
#define CLR_ACCENT          C2D_Color32(137, 180, 250, 255)
#define CLR_ACCENT2         C2D_Color32(166, 227, 161, 255)
#define CLR_WARNING         C2D_Color32(249, 226, 175, 255)
#define CLR_ERROR           C2D_Color32(243, 139, 168, 255)
#define CLR_SUCCESS         C2D_Color32(166, 227, 161, 255)
#define CLR_NAVBAR          C2D_Color32(24, 24, 37, 255)
#define CLR_NAVBAR_ACTIVE   C2D_Color32(137, 180, 250, 255)
#define CLR_BUTTON          C2D_Color32(69, 71, 90, 255)
#define CLR_BUTTON_HOVER    C2D_Color32(88, 91, 112, 255)
#define CLR_WHITE           C2D_Color32(255, 255, 255, 255)
#define CLR_BLACK           C2D_Color32(0, 0, 0, 255)
#define CLR_TRANSPARENT     C2D_Color32(0, 0, 0, 0)

/* ── Utility Macros ── */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi) MIN(MAX(v, lo), hi)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif /* CLOUDSAVER_CONSTANTS_H */
