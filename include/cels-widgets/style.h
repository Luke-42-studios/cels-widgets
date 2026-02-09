/*
 * CELS Widgets - Style System
 *
 * Provides per-instance visual customization for widgets. Each widget
 * accepts an optional `const Widget_*Style*` pointer via its `.style` prop.
 * NULL = use widget defaults (zero breakage for existing code).
 *
 * Style structs use CELS-native types (CEL_Color, CEL_Sizing, CEL_Padding,
 * CEL_BorderMode) from cels-layout. Sizing, padding, border mode, and
 * color overrides are all optional. Zero-initialized fields fall back to
 * the widget's built-in defaults and the active Widget_Theme palette.
 *
 * Style structs are designed for static const definitions at file scope:
 *
 *   static const Widget_ButtonStyle menu_btn = {
 *       .width  = CEL_FIXED(30),
 *       .border = CEL_BORDER_ON_SELECT,
 *   };
 *   Widget_Button(.label = "Play", .selected = true, .style = &menu_btn) {}
 *
 * These types are TUI-backend-specific. When SDL3 or other backends
 * mature, they will define their own style types with backend-appropriate
 * properties (pixel-based sizing, etc.).
 */

#ifndef CELS_WIDGETS_STYLE_H
#define CELS_WIDGETS_STYLE_H

#include <cels-layout/layout.h>

/* ============================================================================
 * Per-Widget Style Structs
 *
 * All sizing/padding/border/color fields use CEL_* types from cels-layout.
 * ============================================================================ */

/* Button style -- all fields optional (zero = use defaults) */
typedef struct Widget_ButtonStyle {
    CEL_Sizing      width;        /* Default: GROW */
    CEL_Sizing      height;       /* Default: FIXED(1) */
    CEL_Padding     padding;      /* Default: {1, 1, 0, 0} */
    int             align;        /* 0=default(center), 1=left, 2=center, 3=right */
    CEL_BorderMode  border;       /* Default: border on select */
    CEL_Color       border_color; /* {0} = use theme focus_border */
    CEL_Color       bg;           /* {0} = use theme button_bg */
    CEL_Color       bg_selected;  /* {0} = use theme button_selected_bg */
    CEL_Color       fg;           /* {0} = use theme button_fg */
    CEL_Color       fg_selected;  /* {0} = use theme focus_border */
} Widget_ButtonStyle;

/* Panel style */
typedef struct Widget_PanelStyle {
    CEL_Sizing      width;        /* Default: GROW */
    CEL_Sizing      height;       /* Default: GROW */
    CEL_Padding     padding;      /* Default: ALL(1) */
    CEL_BorderMode  border;       /* Default: ALWAYS */
    CEL_Color       bg;           /* {0} = use theme panel_bg */
    CEL_Color       border_color; /* {0} = use theme panel_border */
} Widget_PanelStyle;

/* Canvas style */
typedef struct Widget_CanvasStyle {
    CEL_Sizing      width;        /* Default: GROW */
    CEL_Sizing      height;       /* Default: FIXED(3) */
    CEL_Color       bg;           /* {0} = use theme panel_bg */
    CEL_Color       border_color; /* {0} = use theme panel_border */
} Widget_CanvasStyle;

/* ============================================================================
 * Helpers -- resolve style overrides with fallbacks
 *
 * These thin wrappers delegate to the conversion functions in
 * cels-layout/backend_clay.h, providing module-namespaced function
 * names for widget layout code.
 * ============================================================================ */

/* Resolve height sizing (no aspect ratio adjustment) */
static inline Clay_SizingAxis Widget_resolve_sizing(CEL_Sizing s, Clay_SizingAxis fallback) {
    return _cel_resolve_sizing(s, fallback);
}

/* Resolve width sizing (divides by aspect ratio so CEL_FIXED(30) = 30 terminal columns) */
static inline Clay_SizingAxis Widget_resolve_width(CEL_Sizing s, Clay_SizingAxis fallback) {
    return _cel_resolve_width(s, fallback);
}

/* Resolve color override with fallback */
static inline CEL_Color Widget_resolve_color(CEL_Color override, CEL_Color fallback) {
    return _cel_resolve_clay_color(override, fallback);
}

/* ============================================================================
 * Preset Styles
 * ============================================================================ */

/* Compact button: fixed width, centered, border on select (all defaults) */
static const Widget_ButtonStyle Widget_BUTTON_COMPACT = {
    .width  = { .mode = CEL_SIZE_FIXED, .value = 20 },
};

/* Ghost button: no background, text only, no border */
static const Widget_ButtonStyle Widget_BUTTON_GHOST = {
    .bg         = { 0, 0, 0, 1 },  /* alpha=1 to distinguish from "unset" */
    .bg_selected = { 0, 0, 0, 1 },
    .border     = CEL_BORDER_NONE,
};

/* Danger button: red when selected */
static const Widget_ButtonStyle Widget_BUTTON_DANGER = {
    .bg_selected  = { 180, 40, 40, 255 },
    .fg_selected  = { 255, 255, 255, 255 },
    .border_color = { 220, 80, 80, 255 },
};

/* ============================================================================
 * Backward Compatibility (v0.2 -> v0.3)
 * ============================================================================ */
typedef Widget_ButtonStyle TUI_ButtonStyle;
typedef Widget_PanelStyle TUI_PanelStyle;
typedef Widget_CanvasStyle TUI_CanvasStyle;
#define TUI_BUTTON_COMPACT Widget_BUTTON_COMPACT
#define TUI_BUTTON_GHOST   Widget_BUTTON_GHOST
#define TUI_BUTTON_DANGER  Widget_BUTTON_DANGER
#define tui_resolve_sizing Widget_resolve_sizing
#define tui_resolve_width  Widget_resolve_width
#define tui_resolve_color  Widget_resolve_color

#endif /* CELS_WIDGETS_STYLE_H */
