/*
 * CELS Widgets - Style System
 *
 * Provides per-instance visual customization for widgets. Each widget
 * accepts an optional `const Widget_*Style*` pointer via its `.style` prop.
 * NULL = use widget defaults (zero breakage for existing code).
 *
 * Style structs use CELS-native types (CEL_Color, CEL_Sizing, CEL_Padding,
 * CEL_BorderMode, CEL_BorderStyle, CEL_TextAttr) from cels-layout. All
 * fields are optional: zero-initialized values fall through to theme
 * defaults via the alpha sentinel (color.a > 0 = set).
 *
 * W_STYLE_COMMON_FIELDS stamps 6 visual fields onto every style struct
 * so widget-specific styles share a uniform base without nesting:
 *
 *   static const Widget_ButtonStyle menu_btn = {
 *       .bg     = CEL_RGB(40, 44, 60),
 *       .border = CEL_BORDER_ON_SELECT,
 *       .width  = CEL_FIXED(30),
 *   };
 *   Widget_Button(.label = "Play", .selected = true, .style = &menu_btn) {}
 *
 * w_resolve_visual() centralizes theme + style + state resolution:
 *   disabled > selected > focused > normal, with style overrides on top.
 *
 * These types are TUI-backend-specific. When SDL3 or other backends
 * mature, they will define their own style types with backend-appropriate
 * properties (pixel-based sizing, etc.).
 */

#ifndef CELS_WIDGETS_STYLE_H
#define CELS_WIDGETS_STYLE_H

#include <cels-layout/layout.h>
#include <cels-widgets/theme.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * W_STYLE_COMMON_FIELDS - Stamped onto every per-widget style struct
 *
 * Maintains common visual fields in one place (RESEARCH pitfall 6).
 * All fields optional: {0} = inherit from theme via w_resolve_visual().
 * ============================================================================ */

#define W_STYLE_COMMON_FIELDS \
    CEL_Color        bg;           /* {0,0,0,0} = theme default */ \
    CEL_Color        fg;           /* {0,0,0,0} = theme default */ \
    CEL_TextAttr     text_attr;    /* {0} = theme default */       \
    CEL_Color        border_color; /* {0,0,0,0} = theme default */ \
    CEL_BorderMode   border;       /* DEFAULT = widget default */  \
    CEL_BorderStyle  border_style; /* DEFAULT = theme default */

/* ============================================================================
 * W_ResolvedVisual - Output of w_resolve_visual()
 *
 * Final computed visual properties after theme + style + state resolution.
 * Layout functions use this to set Clay element colors and text attributes.
 * ============================================================================ */

typedef struct W_ResolvedVisual {
    CEL_Color    bg;
    CEL_Color    fg;
    CEL_Color    border_color;
    CEL_TextAttr text_attr;
    bool         show_border;
} W_ResolvedVisual;

/* ============================================================================
 * w_resolve_visual() - Centralized state-to-visual mapper
 *
 * Priority chain: disabled > selected > focused > normal
 * Theme provides base visuals per state; style struct overrides on top.
 * Alpha sentinel: style color with a > 0 overrides theme default.
 * ============================================================================ */

static inline W_ResolvedVisual w_resolve_visual(
    const Widget_Theme* theme,
    CEL_Color style_bg,
    CEL_Color style_fg,
    CEL_TextAttr style_attr,
    CEL_Color style_border_color,
    CEL_BorderMode border_mode,
    CEL_BorderMode default_mode,
    bool selected, bool focused, bool disabled)
{
    W_ResolvedVisual v = {0};

    /* Base colors from theme based on highest-priority state */
    if (disabled) {
        v.bg = theme->interactive_disabled.color;
        v.fg = theme->content_muted.color;
        v.text_attr = theme->interactive_disabled.attr;
    } else if (selected) {
        v.bg = theme->interactive_active.color;
        v.fg = theme->primary_content.color;
        v.text_attr = theme->interactive_active.attr;
    } else if (focused) {
        v.bg = theme->interactive.color;
        v.fg = theme->content.color;
        v.text_attr = theme->interactive.attr;
    } else {
        v.bg = theme->interactive.color;
        v.fg = theme->content.color;
        v.text_attr = (CEL_TextAttr){0};
    }

    /* Style overrides (alpha sentinel: a > 0 means set) */
    if (style_bg.a > 0) v.bg = style_bg;
    if (style_fg.a > 0) v.fg = style_fg;
    if (style_border_color.a > 0) {
        v.border_color = style_border_color;
    } else {
        v.border_color = (focused || selected)
            ? theme->border_focused.color
            : theme->border.color;
    }

    /* Text attr override: if any field is set in style, use it */
    if (style_attr.bold || style_attr.dim || style_attr.underline
        || style_attr.reverse || style_attr.italic) {
        v.text_attr = style_attr;
    }

    /* Border visibility from mode */
    CEL_BorderMode mode = (border_mode != CEL_BORDER_DEFAULT)
        ? border_mode : default_mode;
    switch (mode) {
        case CEL_BORDER_ALWAYS:    v.show_border = true; break;
        case CEL_BORDER_ON_SELECT: v.show_border = selected; break;
        case CEL_BORDER_ON_FOCUS:  v.show_border = focused; break;
        case CEL_BORDER_NONE:      v.show_border = false; break;
        default:                   v.show_border = selected; break;
    }

    return v;
}

/* ============================================================================
 * Text Attribute Pack/Unpack (RESEARCH Pattern 5)
 *
 * Encode/decode CEL_TextAttr into a void* (no allocation, fits in pointer).
 * Used to pass text attributes through Clay's userData void* field.
 * ============================================================================ */

static inline void* w_pack_text_attr(CEL_TextAttr attr) {
    uintptr_t packed = 0;
    if (attr.bold)      packed |= 0x01;
    if (attr.dim)       packed |= 0x02;
    if (attr.underline) packed |= 0x04;
    if (attr.reverse)   packed |= 0x08;
    if (attr.italic)    packed |= 0x10;
    return (void*)packed;
}

static inline CEL_TextAttr w_unpack_text_attr(void* userData) {
    uintptr_t packed = (uintptr_t)userData;
    return (CEL_TextAttr){
        .bold      = (packed & 0x01) != 0,
        .dim       = (packed & 0x02) != 0,
        .underline = (packed & 0x04) != 0,
        .reverse   = (packed & 0x08) != 0,
        .italic    = (packed & 0x10) != 0,
    };
}

/* ============================================================================
 * Per-Widget Style Structs (22 widgets)
 *
 * All use W_STYLE_COMMON_FIELDS for flat field access (no .base. prefix).
 * Widget-specific fields follow the common block.
 * ============================================================================ */

/* Button style */
typedef struct Widget_ButtonStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Color       bg_selected;  /* {0} = theme interactive_active */
    CEL_Color       fg_selected;  /* {0} = theme primary_content */
    CEL_Sizing      width;        /* Default: GROW */
    CEL_Sizing      height;       /* Default: FIXED(1) */
    CEL_Padding     padding;      /* Default: {1, 1, 0, 0} */
    int             align;        /* 0=default(center), 1=left, 2=center, 3=right */
} Widget_ButtonStyle;

/* Panel style */
typedef struct Widget_PanelStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Sizing      width;        /* Default: GROW */
    CEL_Sizing      height;       /* Default: GROW */
    CEL_Padding     padding;      /* Default: ALL(1) */
} Widget_PanelStyle;

/* Canvas style */
typedef struct Widget_CanvasStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Sizing      width;        /* Default: GROW */
    CEL_Sizing      height;       /* Default: FIXED(3) */
} Widget_CanvasStyle;

/* Slider style */
typedef struct Widget_SliderStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Color       fill_color;   /* {0} = theme progress_fill */
    CEL_Color       track_color;  /* {0} = theme progress_track */
} Widget_SliderStyle;

/* Toggle style */
typedef struct Widget_ToggleStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Color       on_color;     /* {0} = theme status_success */
    CEL_Color       off_color;    /* {0} = theme content_muted */
} Widget_ToggleStyle;

/* Cycle style */
typedef struct Widget_CycleStyle {
    W_STYLE_COMMON_FIELDS
} Widget_CycleStyle;

/* ProgressBar style */
typedef struct Widget_ProgressBarStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Color       fill_color;   /* {0} = theme progress_fill */
    CEL_Color       track_color;  /* {0} = theme progress_track */
} Widget_ProgressBarStyle;

/* Metric style */
typedef struct Widget_MetricStyle {
    W_STYLE_COMMON_FIELDS
} Widget_MetricStyle;

/* Text style */
typedef struct Widget_TextStyle {
    W_STYLE_COMMON_FIELDS
} Widget_TextStyle;

/* Hint style */
typedef struct Widget_HintStyle {
    W_STYLE_COMMON_FIELDS
} Widget_HintStyle;

/* InfoBox style */
typedef struct Widget_InfoBoxStyle {
    W_STYLE_COMMON_FIELDS
} Widget_InfoBoxStyle;

/* Badge style */
typedef struct Widget_BadgeStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Color       badge_color;  /* {0} = theme accent */
} Widget_BadgeStyle;

/* TextArea style */
typedef struct Widget_TextAreaStyle {
    W_STYLE_COMMON_FIELDS
} Widget_TextAreaStyle;

/* Divider style */
typedef struct Widget_DividerStyle {
    W_STYLE_COMMON_FIELDS
} Widget_DividerStyle;

/* Table style */
typedef struct Widget_TableStyle {
    W_STYLE_COMMON_FIELDS
} Widget_TableStyle;

/* Collapsible style */
typedef struct Widget_CollapsibleStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Color indicator_color; /* {0} = theme primary */
    CEL_Color title_color;     /* {0} = theme content_title */
} Widget_CollapsibleStyle;

/* RadioButton style */
typedef struct Widget_RadioButtonStyle {
    W_STYLE_COMMON_FIELDS
} Widget_RadioButtonStyle;

/* RadioGroup style */
typedef struct Widget_RadioGroupStyle {
    W_STYLE_COMMON_FIELDS
} Widget_RadioGroupStyle;

/* TabBar style */
typedef struct Widget_TabBarStyle {
    W_STYLE_COMMON_FIELDS
    CEL_Color       active_bg;    /* {0} = theme interactive_active */
} Widget_TabBarStyle;

/* TabContent style */
typedef struct Widget_TabContentStyle {
    W_STYLE_COMMON_FIELDS
} Widget_TabContentStyle;

/* StatusBar style */
typedef struct Widget_StatusBarStyle {
    W_STYLE_COMMON_FIELDS
} Widget_StatusBarStyle;

/* ListView style */
typedef struct Widget_ListViewStyle {
    W_STYLE_COMMON_FIELDS
} Widget_ListViewStyle;

/* ListItem style */
typedef struct Widget_ListItemStyle {
    W_STYLE_COMMON_FIELDS
} Widget_ListItemStyle;

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
    .bg         = { 0, 0, 0, 1 },  /* alpha=1 sentinel: transparent black */
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
