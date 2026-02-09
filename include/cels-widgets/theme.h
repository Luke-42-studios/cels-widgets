/*
 * CELS Widgets - Theme System
 *
 * Provides semantic visual tokens for all widget rendering. Each token is
 * a CEL_Visual (color + text attributes), enabling themes to carry both
 * color and text styling (bold, dim, italic, etc.) per role.
 *
 * Widget_Theme is organized as flat prefixed fields (Compose-style):
 *   surface, surface_alt, surface_raised     -- backgrounds
 *   primary, primary_content                 -- accent/focus
 *   content, content_muted, content_title    -- text
 *   interactive, interactive_active, ...     -- widget states
 *   border, border_focused                   -- borders
 *   status_success/warning/error/info        -- status indicators
 *
 * 5 built-in presets: DARK (default), LIGHT, DRACULA, GRUVBOX, NORD.
 * Custom themes use the same struct with designated initializer syntax.
 *
 * Usage:
 *   const Widget_Theme* theme = Widget_get_theme();
 *   CEL_Visual v = theme->content;   // body text visual
 *   // v.color is the RGB, v.attr has bold/dim/etc.
 *
 *   Widget_set_theme(&WIDGET_THEME_DRACULA);  // switch theme
 */

#ifndef CELS_WIDGETS_THEME_H
#define CELS_WIDGETS_THEME_H

#include <cels-layout/types.h>

/* ============================================================================
 * Widget_Theme - Semantic visual tokens for widget rendering
 *
 * All fields are CEL_Visual = { CEL_Color color; CEL_TextAttr attr; }.
 * Zero-initialized fields = inherit/unset.
 * Alpha-based color sentinel: color.a > 0 means set, {0,0,0,0} = unset.
 * ============================================================================ */

typedef struct Widget_Theme {
    /* Surface (backgrounds) */
    CEL_Visual surface;             /* Default background */
    CEL_Visual surface_alt;         /* Recessed areas (status bar, inactive) */
    CEL_Visual surface_raised;      /* Elevated areas (panel, popup) */

    /* Primary (accent) */
    CEL_Visual primary;             /* Focus borders, active indicators */
    CEL_Visual primary_content;     /* Text on primary backgrounds */

    /* Content (text) */
    CEL_Visual content;             /* Body text */
    CEL_Visual content_muted;       /* Dim/secondary text */
    CEL_Visual content_title;       /* Title/heading text */

    /* Interactive (widget states) */
    CEL_Visual interactive;         /* Default interactive bg */
    CEL_Visual interactive_active;  /* Selected/highlighted */
    CEL_Visual interactive_disabled;/* Disabled */

    /* Border */
    CEL_Visual border;              /* Default border */
    CEL_Visual border_focused;      /* Focused border */

    /* Status */
    CEL_Visual status_success;
    CEL_Visual status_warning;
    CEL_Visual status_error;
    CEL_Visual status_info;

    /* Structural */
    CEL_Visual divider;
    CEL_Visual progress_fill;
    CEL_Visual progress_track;

    /* Expanded semantic roles */
    CEL_Visual danger;              /* Destructive actions */
    CEL_Visual accent;              /* Secondary accent */
    CEL_Visual muted;               /* De-emphasized elements */

    /* -----------------------------------------------------------------------
     * Backward Compatibility: old CEL_Color field names
     *
     * These map to the color component of the semantic tokens above.
     * Used by layouts.c until Plan 12-03 migrates to w_resolve_visual().
     * DEPRECATED -- will be removed after Plan 12-03 migration.
     * ----------------------------------------------------------------------- */
    CEL_Color bg;                  /* = surface.color */
    CEL_Color panel_bg;            /* = surface_raised.color */
    CEL_Color panel_border;        /* = border.color */
    CEL_Color fg;                  /* = content.color */
    CEL_Color secondary;           /* = content_muted.color */
    CEL_Color button_bg;           /* = interactive.color */
    CEL_Color button_fg;           /* = content.color */
    CEL_Color button_selected_bg;  /* = interactive_active.color */
    CEL_Color focus_border;        /* = primary.color */
    CEL_Color success;             /* = status_success.color */
    CEL_Color warning;             /* = status_warning.color */
    CEL_Color error;               /* = status_error.color */
    CEL_Color progress_empty;      /* = progress_track.color */
    CEL_Color tab_active_bg;       /* = interactive_active.color */
    CEL_Color status_bar_bg;       /* = surface_alt.color */
    CEL_Color badge_bg;            /* = accent.color */
} Widget_Theme;

/* ============================================================================
 * Helper: CEL_Visual from RGB (alpha=255, no text attrs)
 * ============================================================================ */

#define _W_V(r, g, b)  ((CEL_Visual){ .color = CEL_RGB(r, g, b), .attr = {0} })
#define _W_VB(r, g, b) ((CEL_Visual){ .color = CEL_RGB(r, g, b), .attr = { .bold = true } })

/* ============================================================================
 * WIDGET_THEME_DARK - Default dark terminal palette
 *
 * Preserves the visual appearance of the old Widget_THEME_DEFAULT.
 * Colors mapped from old 17-field palette to semantic tokens.
 * ============================================================================ */

static const Widget_Theme WIDGET_THEME_DARK = {
    /* Surface */
    .surface        = _W_V( 30,  33,  45),
    .surface_alt    = _W_V( 20,  22,  35),
    .surface_raised = _W_V( 25,  28,  38),

    /* Primary */
    .primary         = _W_VB( 80, 100, 180),
    .primary_content = _W_V(200, 205, 220),

    /* Content */
    .content       = _W_V(200, 205, 220),
    .content_muted = _W_V(120, 130, 160),
    .content_title = _W_VB( 80, 100, 180),

    /* Interactive */
    .interactive          = _W_V( 40,  44,  60),
    .interactive_active   = _W_V( 60,  70, 110),
    .interactive_disabled = _W_V( 50,  53,  65),

    /* Border */
    .border         = _W_V( 70,  75, 100),
    .border_focused = _W_V( 80, 100, 180),

    /* Status */
    .status_success = _W_V( 80, 200, 100),
    .status_warning = _W_V(220, 180,  60),
    .status_error   = _W_V(220,  80,  80),
    .status_info    = _W_V( 80, 160, 255),

    /* Structural */
    .divider        = _W_V( 60,  65,  85),
    .progress_fill  = _W_V( 80, 160, 255),
    .progress_track = _W_V( 50,  55,  70),

    /* Extended */
    .danger = _W_V(220,  80,  80),
    .accent = _W_V( 60,  70, 110),
    .muted  = _W_V( 50,  55,  70),

    /* Backward compat (DEPRECATED) */
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
    .progress_empty     = {  50,  55,  70, 255 },
    .tab_active_bg      = {  50,  55,  80, 255 },
    .status_bar_bg      = {  20,  22,  35, 255 },
    .badge_bg           = {  60,  70, 110, 255 },
};

/* ============================================================================
 * WIDGET_THEME_LIGHT - Light terminal palette
 * ============================================================================ */

static const Widget_Theme WIDGET_THEME_LIGHT = {
    /* Surface */
    .surface        = _W_V(245, 245, 245),
    .surface_alt    = _W_V(228, 228, 228),
    .surface_raised = _W_V(255, 255, 255),

    /* Primary */
    .primary         = _W_VB( 25, 118, 210),
    .primary_content = _W_V(255, 255, 255),

    /* Content */
    .content       = _W_V( 33,  33,  33),
    .content_muted = _W_V(117, 117, 117),
    .content_title = _W_VB( 25, 118, 210),

    /* Interactive */
    .interactive          = _W_V(224, 224, 224),
    .interactive_active   = _W_V(187, 222, 251),
    .interactive_disabled = _W_V(238, 238, 238),

    /* Border */
    .border         = _W_V(189, 189, 189),
    .border_focused = _W_V( 25, 118, 210),

    /* Status */
    .status_success = _W_V( 46, 125,  50),
    .status_warning = _W_V(245, 124,   0),
    .status_error   = _W_V(211,  47,  47),
    .status_info    = _W_V(  2, 136, 209),

    /* Structural */
    .divider        = _W_V(189, 189, 189),
    .progress_fill  = _W_V( 25, 118, 210),
    .progress_track = _W_V(224, 224, 224),

    /* Extended */
    .danger = _W_V(211,  47,  47),
    .accent = _W_V(156,  39, 176),
    .muted  = _W_V(158, 158, 158),

    /* Backward compat (DEPRECATED) */
    .bg                 = { 245, 245, 245, 255 },
    .panel_bg           = { 255, 255, 255, 255 },
    .panel_border       = { 189, 189, 189, 255 },
    .fg                 = {  33,  33,  33, 255 },
    .secondary          = { 117, 117, 117, 255 },
    .button_bg          = { 224, 224, 224, 255 },
    .button_fg          = {  33,  33,  33, 255 },
    .button_selected_bg = { 187, 222, 251, 255 },
    .focus_border       = {  25, 118, 210, 255 },
    .success            = {  46, 125,  50, 255 },
    .warning            = { 245, 124,   0, 255 },
    .error              = { 211,  47,  47, 255 },
    .progress_empty     = { 224, 224, 224, 255 },
    .tab_active_bg      = { 187, 222, 251, 255 },
    .status_bar_bg      = { 228, 228, 228, 255 },
    .badge_bg           = { 156,  39, 176, 255 },
};

/* ============================================================================
 * WIDGET_THEME_DRACULA - Dracula color scheme (draculatheme.com)
 *
 * Background: #282a36  Current Line: #44475a  Foreground: #f8f8f2
 * Comment: #6272a4     Cyan: #8be9fd         Green: #50fa7b
 * Orange: #ffb86c      Pink: #ff79c6         Purple: #bd93f9
 * Red: #ff5555         Yellow: #f1fa8c
 * ============================================================================ */

static const Widget_Theme WIDGET_THEME_DRACULA = {
    /* Surface */
    .surface        = _W_V( 40,  42,  54),   /* #282a36 */
    .surface_alt    = _W_V( 33,  34,  44),   /* darker bg */
    .surface_raised = _W_V( 68,  71,  90),   /* #44475a */

    /* Primary */
    .primary         = _W_VB(189, 147, 249),  /* #bd93f9 Purple */
    .primary_content = _W_V(248, 248, 242),   /* #f8f8f2 */

    /* Content */
    .content       = _W_V(248, 248, 242),     /* #f8f8f2 */
    .content_muted = _W_V( 98, 114, 164),     /* #6272a4 Comment */
    .content_title = _W_VB(189, 147, 249),     /* #bd93f9 Purple */

    /* Interactive */
    .interactive          = _W_V( 68,  71,  90),   /* #44475a */
    .interactive_active   = _W_V( 98, 114, 164),   /* #6272a4 */
    .interactive_disabled = _W_V( 55,  57,  72),

    /* Border */
    .border         = _W_V( 98, 114, 164),    /* #6272a4 */
    .border_focused = _W_V(189, 147, 249),     /* #bd93f9 */

    /* Status */
    .status_success = _W_V( 80, 250, 123),    /* #50fa7b */
    .status_warning = _W_V(241, 250, 140),     /* #f1fa8c */
    .status_error   = _W_V(255,  85,  85),     /* #ff5555 */
    .status_info    = _W_V(139, 233, 253),     /* #8be9fd */

    /* Structural */
    .divider        = _W_V( 68,  71,  90),    /* #44475a */
    .progress_fill  = _W_V(189, 147, 249),     /* #bd93f9 */
    .progress_track = _W_V( 55,  57,  72),

    /* Extended */
    .danger = _W_V(255,  85,  85),             /* #ff5555 */
    .accent = _W_V(255, 121, 198),             /* #ff79c6 Pink */
    .muted  = _W_V( 55,  57,  72),

    /* Backward compat (DEPRECATED) */
    .bg                 = {  40,  42,  54, 255 },
    .panel_bg           = {  68,  71,  90, 255 },
    .panel_border       = {  98, 114, 164, 255 },
    .fg                 = { 248, 248, 242, 255 },
    .secondary          = {  98, 114, 164, 255 },
    .button_bg          = {  68,  71,  90, 255 },
    .button_fg          = { 248, 248, 242, 255 },
    .button_selected_bg = {  98, 114, 164, 255 },
    .focus_border       = { 189, 147, 249, 255 },
    .success            = {  80, 250, 123, 255 },
    .warning            = { 241, 250, 140, 255 },
    .error              = { 255,  85,  85, 255 },
    .progress_empty     = {  55,  57,  72, 255 },
    .tab_active_bg      = {  98, 114, 164, 255 },
    .status_bar_bg      = {  33,  34,  44, 255 },
    .badge_bg           = { 255, 121, 198, 255 },
};

/* ============================================================================
 * WIDGET_THEME_GRUVBOX - Gruvbox dark palette (github.com/morhetz/gruvbox)
 *
 * bg: #282828  bg1: #3c3836  bg2: #504945  bg3: #665c54
 * fg: #ebdbb2  fg4: #a89984
 * Red: #fb4934  Green: #b8bb26  Yellow: #fabd2f
 * Blue: #83a598  Purple: #d3869b  Aqua: #8ec07c  Orange: #fe8019
 * ============================================================================ */

static const Widget_Theme WIDGET_THEME_GRUVBOX = {
    /* Surface */
    .surface        = _W_V( 40,  40,  40),   /* #282828 */
    .surface_alt    = _W_V( 29,  32,  33),   /* #1d2021 hard bg */
    .surface_raised = _W_V( 60,  56,  54),   /* #3c3836 */

    /* Primary */
    .primary         = _W_VB(250, 189,  47),  /* #fabd2f Yellow */
    .primary_content = _W_V( 40,  40,  40),   /* #282828 */

    /* Content */
    .content       = _W_V(235, 219, 178),     /* #ebdbb2 */
    .content_muted = _W_V(168, 153, 132),     /* #a89984 */
    .content_title = _W_VB(250, 189,  47),     /* #fabd2f Yellow */

    /* Interactive */
    .interactive          = _W_V( 60,  56,  54),   /* #3c3836 */
    .interactive_active   = _W_V( 80,  73,  69),   /* #504945 */
    .interactive_disabled = _W_V( 50,  48,  47),

    /* Border */
    .border         = _W_V(102,  92,  84),    /* #665c54 */
    .border_focused = _W_V(250, 189,  47),     /* #fabd2f */

    /* Status */
    .status_success = _W_V(184, 187,  38),    /* #b8bb26 */
    .status_warning = _W_V(254, 128,  25),     /* #fe8019 */
    .status_error   = _W_V(251,  73,  52),     /* #fb4934 */
    .status_info    = _W_V(131, 165, 152),     /* #83a598 */

    /* Structural */
    .divider        = _W_V( 80,  73,  69),    /* #504945 */
    .progress_fill  = _W_V(184, 187,  38),     /* #b8bb26 */
    .progress_track = _W_V( 50,  48,  47),

    /* Extended */
    .danger = _W_V(251,  73,  52),             /* #fb4934 */
    .accent = _W_V(211, 134, 155),             /* #d3869b Purple */
    .muted  = _W_V( 80,  73,  69),            /* #504945 */

    /* Backward compat (DEPRECATED) */
    .bg                 = {  40,  40,  40, 255 },
    .panel_bg           = {  60,  56,  54, 255 },
    .panel_border       = { 102,  92,  84, 255 },
    .fg                 = { 235, 219, 178, 255 },
    .secondary          = { 168, 153, 132, 255 },
    .button_bg          = {  60,  56,  54, 255 },
    .button_fg          = { 235, 219, 178, 255 },
    .button_selected_bg = {  80,  73,  69, 255 },
    .focus_border       = { 250, 189,  47, 255 },
    .success            = { 184, 187,  38, 255 },
    .warning            = { 254, 128,  25, 255 },
    .error              = { 251,  73,  52, 255 },
    .progress_empty     = {  50,  48,  47, 255 },
    .tab_active_bg      = {  80,  73,  69, 255 },
    .status_bar_bg      = {  29,  32,  33, 255 },
    .badge_bg           = { 211, 134, 155, 255 },
};

/* ============================================================================
 * WIDGET_THEME_NORD - Nord palette (nordtheme.com)
 *
 * Polar Night: #2E3440 #3B4252 #434C5E #4C566A
 * Snow Storm:  #D8DEE9 #E5E9F0 #ECEFF4
 * Frost:       #8FBCBB #88C0D0 #81A1C1 #5E81AC
 * Aurora:      #BF616A #D08770 #EBCB8B #A3BE8C #B48EAD
 * ============================================================================ */

static const Widget_Theme WIDGET_THEME_NORD = {
    /* Surface */
    .surface        = _W_V( 46,  52,  64),   /* #2E3440 */
    .surface_alt    = _W_V( 59,  66,  82),   /* #3B4252 */
    .surface_raised = _W_V( 67,  76,  94),   /* #434C5E */

    /* Primary */
    .primary         = _W_VB(136, 192, 208),  /* #88C0D0 Frost */
    .primary_content = _W_V( 46,  52,  64),   /* #2E3440 */

    /* Content */
    .content       = _W_V(216, 222, 233),     /* #D8DEE9 */
    .content_muted = _W_V(127, 140, 162),     /* approx muted */
    .content_title = _W_VB(136, 192, 208),     /* #88C0D0 */

    /* Interactive */
    .interactive          = _W_V( 67,  76,  94),   /* #434C5E */
    .interactive_active   = _W_V( 76,  86, 106),   /* #4C566A */
    .interactive_disabled = _W_V( 55,  62,  78),

    /* Border */
    .border         = _W_V( 76,  86, 106),    /* #4C566A */
    .border_focused = _W_V(136, 192, 208),     /* #88C0D0 */

    /* Status */
    .status_success = _W_V(163, 190, 140),    /* #A3BE8C */
    .status_warning = _W_V(235, 203, 139),     /* #EBCB8B */
    .status_error   = _W_V(191,  97, 106),     /* #BF616A */
    .status_info    = _W_V(129, 161, 193),     /* #81A1C1 */

    /* Structural */
    .divider        = _W_V( 76,  86, 106),    /* #4C566A */
    .progress_fill  = _W_V(136, 192, 208),     /* #88C0D0 */
    .progress_track = _W_V( 55,  62,  78),

    /* Extended */
    .danger = _W_V(191,  97, 106),             /* #BF616A */
    .accent = _W_V(180, 142, 173),             /* #B48EAD */
    .muted  = _W_V( 67,  76,  94),            /* #434C5E */

    /* Backward compat (DEPRECATED) */
    .bg                 = {  46,  52,  64, 255 },
    .panel_bg           = {  67,  76,  94, 255 },
    .panel_border       = {  76,  86, 106, 255 },
    .fg                 = { 216, 222, 233, 255 },
    .secondary          = { 127, 140, 162, 255 },
    .button_bg          = {  67,  76,  94, 255 },
    .button_fg          = { 216, 222, 233, 255 },
    .button_selected_bg = {  76,  86, 106, 255 },
    .focus_border       = { 136, 192, 208, 255 },
    .success            = { 163, 190, 140, 255 },
    .warning            = { 235, 203, 139, 255 },
    .error              = { 191,  97, 106, 255 },
    .progress_empty     = {  55,  62,  78, 255 },
    .tab_active_bg      = {  76,  86, 106, 255 },
    .status_bar_bg      = {  59,  66,  82, 255 },
    .badge_bg           = { 180, 142, 173, 255 },
};

/* ============================================================================
 * Default Alias
 * ============================================================================ */

#define Widget_THEME_DEFAULT WIDGET_THEME_DARK

/* ============================================================================
 * Theme API
 * ============================================================================ */

/* Get current active theme (never returns NULL) */
extern const Widget_Theme* Widget_get_theme(void);

/* Set active theme. Pass NULL to restore default. */
extern void Widget_set_theme(const Widget_Theme* theme);

/* ============================================================================
 * Backward Compatibility (v0.2 -> v0.3)
 * ============================================================================ */
typedef Widget_Theme TUI_Theme;
#define TUI_THEME_DEFAULT Widget_THEME_DEFAULT
#define tui_get_theme Widget_get_theme
#define tui_set_theme Widget_set_theme

#endif /* CELS_WIDGETS_THEME_H */
