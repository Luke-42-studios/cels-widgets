/*
 * CELS Widgets - Widget component definitions with Clay layout integration
 *
 * Defines reusable UI widget components using CEL_Define. Each widget has:
 *   1. Component definition (pure data, this file)
 *   2. Clay layout function (layouts.h / layouts.c)
 *   3. Composition macro (compositions.h)
 *
 * The Clay layout system renders all widgets generically via the
 * clay_ncurses_renderer -- no per-widget ncurses renderer code needed.
 *
 * All widget components use the W_ prefix.
 *
 * Usage:
 *   #include <cels-widgets/widgets.h>
 *   #include <cels-widgets/compositions.h>
 *
 *   CEL_Root(App, ...) {
 *       Layout_Surface(.width = w, .height = h) {
 *           Widget_Panel(.title = "My Panel") {
 *               Widget_Button(.label = "Play", .selected = true) {}
 *               Widget_Slider(.label = "Volume", .value = 0.7f) {}
 *           }
 *       }
 *   }
 *
 * Module initialization:
 *   #include <cels-widgets/widgets.h>
 *   CEL_Build(App) {
 *       Widget_init();
 *   }
 */

#ifndef CELS_WIDGETS_H
#define CELS_WIDGETS_H

#include <cels/cels.h>
#include <cels-widgets/style.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Text & Display Components
 * ============================================================================ */

/* Text: simple text display with alignment */
CEL_Define(W_Text, {
    const char* text;       /* Text content */
    int align;              /* 0 = left, 1 = center, 2 = right */
});

/* Hint: dim hint text line */
CEL_Define(W_Hint, {
    const char* text;       /* Hint text content */
});

/* Canvas: header box with centered title */
CEL_Define(W_Canvas, {
    const char* title;      /* Centered title text */
    int width;              /* Box width (0 = backend default) */
    const Widget_CanvasStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Info box: bordered box with title and content */
CEL_Define(W_InfoBox, {
    const char* title;      /* Box title */
    const char* content;    /* Content text */
    bool border;            /* Draw border (default true) */
});

/* Badge: styled tag/label with color */
CEL_Define(W_Badge, {
    const char* text;       /* Badge text */
    unsigned char r, g, b;  /* Badge color (0-255) */
});

/* TextArea: multi-line scrollable text */
CEL_Define(W_TextArea, {
    const char* text;       /* Multi-line text content */
    int max_width;          /* Max width (0 = grow) */
    int max_height;         /* Max height (0 = grow) */
    bool scrollable;        /* Enable scroll container */
});

/* ============================================================================
 * Interactive Components
 * ============================================================================ */

/* Button: selectable button with label and selection state */
CEL_Define(W_Button, {
    const char* label;      /* Button text */
    bool selected;          /* True if currently highlighted */
    bool focused;           /* True if has keyboard focus */
    void (*on_press)(void); /* Callback when activated (Enter/Space) */
    const Widget_ButtonStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Slider: labeled value slider with min/max */
CEL_Define(W_Slider, {
    const char* label;      /* Slider label text */
    float value;            /* Current value (0.0 - 1.0) */
    float min;              /* Minimum value (default 0) */
    float max;              /* Maximum value (default 1) */
    bool selected;          /* True if currently highlighted */
});

/* Toggle: labeled ON/OFF toggle */
CEL_Define(W_Toggle, {
    const char* label;      /* Toggle label text */
    bool value;             /* Current on/off state */
    bool selected;          /* True if currently highlighted */
});

/* Cycle: labeled cycle-through-options control */
CEL_Define(W_Cycle, {
    const char* label;      /* Cycle label text */
    const char* value;      /* Current displayed value string */
    bool selected;          /* True if currently highlighted */
});

/* ============================================================================
 * Progress & Metric Components
 * ============================================================================ */

/* ProgressBar: horizontal progress indicator */
CEL_Define(W_ProgressBar, {
    const char* label;      /* Progress label text */
    float value;            /* Progress value (0.0 - 1.0) */
    bool color_by_value;    /* Color changes based on value (red->yellow->green) */
});

/* Metric: label + value display for dashboards */
CEL_Define(W_Metric, {
    const char* label;      /* Metric label */
    const char* value;      /* Formatted value string */
    int status;             /* 0=normal, 1=success, 2=warning, 3=error */
});

/* ============================================================================
 * Container Components
 * ============================================================================ */

/* Panel: bordered container with optional title */
CEL_Define(W_Panel, {
    const char* title;      /* Panel title (NULL = no title) */
    int border_style;       /* 0=single, 1=double, 2=rounded */
    const Widget_PanelStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Divider: horizontal or vertical separator */
CEL_Define(W_Divider, {
    bool vertical;          /* true = vertical, false = horizontal */
});

/* Table: key-value table display */
CEL_Define(W_Table, {
    int row_count;          /* Number of rows */
    const char** keys;      /* Array of key strings */
    const char** values;    /* Array of value strings */
});

/* ============================================================================
 * Radio Components
 * ============================================================================ */

/* Radio button: individual radio option in a group */
CEL_Define(W_RadioButton, {
    const char* label;      /* Radio button label */
    bool selected;          /* True if this option is selected */
    int group_id;           /* Group identifier (links related radios) */
});

/* Radio group: container for radio button state */
CEL_Define(W_RadioGroup, {
    int group_id;           /* Group identifier */
    int selected_index;     /* Currently selected option index */
    int count;              /* Total number of options */
});

/* ============================================================================
 * Navigation Components
 * ============================================================================ */

/* Tab bar: horizontal tab strip with numbered labels */
CEL_Define(W_TabBar, {
    int active;             /* Index of the currently active tab */
    int count;              /* Total number of tabs */
    const char** labels;    /* Array of tab label strings (count elements) */
});

/* Tab content: placeholder content area for a tab */
CEL_Define(W_TabContent, {
    const char* text;       /* Main placeholder text (centered) */
    const char* hint;       /* Secondary hint text (centered, below main) */
});

/* Status bar: bottom status line with left and right sections */
CEL_Define(W_StatusBar, {
    const char* left;       /* Left-aligned text */
    const char* right;      /* Right-aligned text */
});

/* ============================================================================
 * List Components
 * ============================================================================ */

/* List view: scrollable list container */
CEL_Define(W_ListView, {
    int item_count;         /* Total number of items */
    int selected_index;     /* Currently selected item index */
    int scroll_offset;      /* First visible item index */
    int visible_count;      /* Number of visible items (0 = auto) */
});

/* List item: individual item in a list view */
CEL_Define(W_ListItem, {
    const char* label;      /* Item label text */
    bool selected;          /* True if currently selected */
    void* data;             /* User data pointer */
});

/* ============================================================================
 * Focus Tag
 * ============================================================================ */

/* Focusable: tag component marking a widget as keyboard-focusable */
CEL_Define(W_Focusable, {
    int tab_order;          /* Tab navigation order (0 = auto) */
});

/* ============================================================================
 * Module API
 * ============================================================================ */

/* Module entity ID and init function */
extern cels_entity_t Widgets;
extern void Widgets_init(void);

/* Canonical module-namespaced alias */
#define Widget_init Widgets_init

#ifdef __cplusplus
}
#endif

#endif /* CELS_WIDGETS_H */
