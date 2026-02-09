/*
 * CELS Widgets - TUI Theme System
 *
 * Provides a shared color theme for all widget layout functions.
 * Colors are CEL_Color {r, g, b, a} from cels-layout. The ncurses
 * renderer maps these to terminal colors via approximate RGB matching.
 *
 * Usage:
 *   const TUI_Theme* theme = tui_get_theme();
 *   // Use theme->button_bg, theme->text_primary, etc. in layouts
 *
 *   // Optionally set a custom theme:
 *   static const TUI_Theme my_theme = { ... };
 *   tui_set_theme(&my_theme);
 */

#ifndef CELS_WIDGETS_THEME_H
#define CELS_WIDGETS_THEME_H

#include <cels-layout/types.h>

/* ============================================================================
 * TUI_Theme - Color palette for widget rendering
 * ============================================================================ */
typedef struct TUI_Theme {
    /* Background colors */
    CEL_Color bg;                  /* Default widget background */
    CEL_Color panel_bg;            /* Panel interior background */
    CEL_Color panel_border;        /* Panel border color */

    /* Text colors */
    CEL_Color fg;                  /* Primary text color */
    CEL_Color secondary;           /* Secondary/dim text */

    /* Button colors */
    CEL_Color button_bg;           /* Button default background */
    CEL_Color button_fg;           /* Button text color */
    CEL_Color button_selected_bg;  /* Selected button background */

    /* Focus indicator */
    CEL_Color focus_border;        /* Focus ring border color */

    /* Status colors */
    CEL_Color success;             /* Success/active/green */
    CEL_Color warning;             /* Warning/amber */
    CEL_Color error;               /* Error/red */

    /* Component-specific */
    CEL_Color progress_fill;       /* Progress bar filled portion */
    CEL_Color progress_empty;      /* Progress bar empty portion */
    CEL_Color divider;             /* Divider line color */
    CEL_Color tab_active_bg;       /* Active tab background */
    CEL_Color status_bar_bg;       /* Status bar background */
    CEL_Color badge_bg;            /* Default badge background */
} TUI_Theme;

/* ============================================================================
 * Default Theme - Dark terminal-friendly palette
 * ============================================================================ */
static const TUI_Theme TUI_THEME_DEFAULT = {
    .bg                 = {  30,  33,  45, 255 },
    .panel_bg           = {  25,  28,  38, 255 },
    .panel_border       = {  70,  75, 100, 255 },

    .fg                 = { 200, 205, 220, 255 },
    .secondary          = { 120, 130, 160, 255 },

    .button_bg          = {  40,  44,  60, 255 },
    .button_fg          = { 200, 205, 220, 255 },
    .button_selected_bg = {  60,  70, 110, 255 },

    .focus_border       = {  80, 100, 180, 255 },

    .success            = {  80, 200, 100, 255 },
    .warning            = { 220, 180,  60, 255 },
    .error              = { 220,  80,  80, 255 },

    .progress_fill      = {  80, 160, 255, 255 },
    .progress_empty     = {  50,  55,  70, 255 },
    .divider            = {  60,  65,  85, 255 },
    .tab_active_bg      = {  50,  55,  80, 255 },
    .status_bar_bg      = {  20,  22,  35, 255 },
    .badge_bg           = {  60,  70, 110, 255 },
};

/* ============================================================================
 * Theme API
 * ============================================================================ */

/* Get current active theme (never returns NULL) */
extern const TUI_Theme* tui_get_theme(void);

/* Set active theme. Pass NULL to restore default. */
extern void tui_set_theme(const TUI_Theme* theme);

#endif /* CELS_WIDGETS_THEME_H */
