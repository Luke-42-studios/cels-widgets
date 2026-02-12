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
    const Widget_TextStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Hint: dim hint text line */
CEL_Define(W_Hint, {
    const char* text;       /* Hint text content */
    const Widget_HintStyle* style; /* Visual overrides (NULL = defaults) */
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
    const Widget_InfoBoxStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Badge: styled tag/label with color */
CEL_Define(W_Badge, {
    const char* text;       /* Badge text */
    unsigned char r, g, b;  /* Badge color (0-255) */
    const Widget_BadgeStyle* style; /* Visual overrides (NULL = defaults) */
});

/* TextArea: multi-line scrollable text
 * Note: W_Scrollable component attached for scroll state */
CEL_Define(W_TextArea, {
    const char* text;       /* Multi-line text content */
    int max_width;          /* Max width (0 = grow) */
    int max_height;         /* Max height (0 = grow) */
    bool scrollable;        /* Enable scroll container */
    const Widget_TextAreaStyle* style; /* Visual overrides (NULL = defaults) */
});

/* ============================================================================
 * Interactive Components
 * ============================================================================ */

/* Button: selectable button with label (selection via W_Selectable component) */
CEL_Define(W_Button, {
    const char* label;      /* Button text */
    void (*on_press)(void); /* Callback when activated (Enter/Space) */
    const Widget_ButtonStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Slider: labeled value slider (range via W_RangeValueF component) */
CEL_Define(W_Slider, {
    const char* label;      /* Slider label text */
    const Widget_SliderStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Toggle: labeled ON/OFF toggle (selection via W_Selectable component) */
CEL_Define(W_Toggle, {
    const char* label;      /* Toggle label text */
    bool value;             /* Current on/off state */
    const Widget_ToggleStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Cycle: labeled cycle-through-options control (selection via W_Selectable component) */
CEL_Define(W_Cycle, {
    const char* label;      /* Cycle label text */
    const char* value;      /* Current displayed value string */
    const Widget_CycleStyle* style; /* Visual overrides (NULL = defaults) */
});

/* ============================================================================
 * Progress & Metric Components
 * ============================================================================ */

/* ProgressBar: horizontal progress indicator (value via W_RangeValueF component) */
CEL_Define(W_ProgressBar, {
    const char* label;      /* Progress label text */
    bool color_by_value;    /* Color changes based on value (red->yellow->green) */
    const Widget_ProgressBarStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Metric: label + value display for dashboards */
CEL_Define(W_Metric, {
    const char* label;      /* Metric label */
    const char* value;      /* Formatted value string */
    int status;             /* 0=normal, 1=success, 2=warning, 3=error */
    const Widget_MetricStyle* style; /* Visual overrides (NULL = defaults) */
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
    const Widget_DividerStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Table: key-value table display */
CEL_Define(W_Table, {
    int row_count;          /* Number of rows */
    const char** keys;      /* Array of key strings */
    const char** values;    /* Array of value strings */
    const Widget_TableStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Collapsible: expandable/collapsible content section with title */
CEL_Define(W_Collapsible, {
    const char* title;      /* Section title text */
    bool collapsed;         /* true = children hidden, false = expanded */
    int indent;             /* Nesting depth for indentation (0 = top level) */
    const Widget_CollapsibleStyle* style; /* Visual overrides (NULL = defaults) */
});

/* SplitPane: two-region split container with configurable ratio */
CEL_Define(W_SplitPane, {
    float ratio;            /* Split ratio for first pane (0.0-1.0, default 0.5) */
    int direction;          /* 0 = horizontal (left|right), 1 = vertical (top|bottom) */
    const Widget_SplitStyle* style; /* Visual overrides (NULL = defaults) */
});

/* ScrollContainer: generic scrollable viewport with scrollbar gutter */
CEL_Define(W_ScrollContainer, {
    int height;             /* Viewport height in rows (required, determines visible_count) */
    const Widget_ScrollableStyle* style; /* Visual overrides (NULL = defaults) */
});

/* ============================================================================
 * Radio Components
 * ============================================================================ */

/* Radio button: individual radio option in a group (selection via W_Selectable component) */
CEL_Define(W_RadioButton, {
    const char* label;      /* Radio button label */
    int group_id;           /* Group identifier (links related radios) */
    const Widget_RadioButtonStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Radio group: container for radio button state */
CEL_Define(W_RadioGroup, {
    int group_id;           /* Group identifier */
    int selected_index;     /* Currently selected option index */
    int count;              /* Total number of options */
    const Widget_RadioGroupStyle* style; /* Visual overrides (NULL = defaults) */
});

/* ============================================================================
 * Navigation Components
 * ============================================================================ */

/* Tab bar: horizontal tab strip with numbered labels */
CEL_Define(W_TabBar, {
    int active;             /* Index of the currently active tab */
    int count;              /* Total number of tabs */
    const char** labels;    /* Array of tab label strings (count elements) */
    const Widget_TabBarStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Tab content: placeholder content area for a tab */
CEL_Define(W_TabContent, {
    const char* text;       /* Main placeholder text (centered) */
    const char* hint;       /* Secondary hint text (centered, below main) */
    const Widget_TabContentStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Status bar: bottom status line with left and right sections */
CEL_Define(W_StatusBar, {
    const char* left;       /* Left-aligned text */
    const char* right;      /* Right-aligned text */
    const Widget_StatusBarStyle* style; /* Visual overrides (NULL = defaults) */
});

/* ============================================================================
 * List Components
 * ============================================================================ */

/* List view: scrollable list container (scroll via W_Scrollable component) */
CEL_Define(W_ListView, {
    int item_count;         /* Total number of items */
    int selected_index;     /* Currently selected item index */
    const Widget_ListViewStyle* style; /* Visual overrides (NULL = defaults) */
});

/* List item: individual item in a list view (selection via W_Selectable component) */
CEL_Define(W_ListItem, {
    const char* label;      /* Item label text */
    void* data;             /* User data pointer */
    const Widget_ListItemStyle* style; /* Visual overrides (NULL = defaults) */
});

/* ============================================================================
 * Focus Tag
 * ============================================================================ */

/* Focusable: tag component marking a widget as keyboard-focusable */
CEL_Define(W_Focusable, {
    int tab_order;          /* Tab navigation order (0 = auto) */
});

/* ============================================================================
 * Interaction State
 * ============================================================================ */

/* Interaction state: unified visual state for all interactive widgets.
 * w_resolve_visual() reads these flags to determine theme-based colors.
 * Composition macros set these from props (Plan 12-03). */
CEL_Define(W_InteractState, {
    bool focused;       /* Has keyboard focus */
    bool selected;      /* Currently selected/highlighted */
    bool disabled;      /* Interaction disabled */
});

/* ============================================================================
 * Behavioral Components
 * ============================================================================
 *
 * Behavioral components represent reusable interaction patterns that any
 * widget can opt into. Built-in systems (Plan 03) enforce invariants like
 * range clamping and scroll bounds automatically.
 *
 * Use CEL_UseInstead(OldName, NewName, ...) to swap a default behavioral
 * component for a custom one -- built-in systems skip entities that don't
 * have the default component.
 */

/* Selectable: marks a widget as part of a selection group.
 * Built-in systems will manage selection state within NavigationScopes. */
CEL_Define(W_Selectable, {
    bool selected;          /* True if currently selected in its group */
});

/* RangeValueF: bounded float range for sliders, progress bars, etc.
 * Built-in RangeClampF system enforces min <= value <= max each frame. */
CEL_Define(W_RangeValueF, {
    float value;            /* Current value */
    float min;              /* Minimum bound */
    float max;              /* Maximum bound */
    float step;             /* Increment step (0 = continuous) */
});

/* RangeValueI: bounded integer range for step counters, page selectors, etc.
 * Forward-looking infrastructure -- no built-in widget attaches this in Phase 13.
 * Built-in RangeClampI system enforces min <= value <= max each frame. */
CEL_Define(W_RangeValueI, {
    int value;              /* Current value */
    int min;                /* Minimum bound */
    int max;                /* Maximum bound */
    int step;               /* Increment step (0 = 1) */
});

/* Scrollable: scroll state for list views, text areas, etc.
 * Built-in ScrollClamp system enforces scroll_offset bounds each frame. */
CEL_Define(W_Scrollable, {
    int scroll_offset;      /* First visible item/line index */
    int total_count;        /* Total number of items/lines */
    int visible_count;      /* Number of visible items/lines */
});

/* ============================================================================
 * Overlay Components
 * ============================================================================ */

/* OverlayState: marks an entity as an overlay with z-ordering and visibility.
 * Used by Widget_Popup, Widget_Modal, Widget_Window, Widget_Toast. */
CEL_Define(W_OverlayState, {
    bool visible;       /* Whether the overlay is currently shown */
    int z_index;        /* z-ordering value (higher = on top) */
    bool modal;         /* true = focus-isolating overlay */
});

/* Toast: auto-dismissing notification overlay */
CEL_Define(W_Toast, {
    const char* message;    /* Toast message text */
    float duration;         /* Auto-dismiss after N seconds (default 3.0) */
    float elapsed;          /* Time since toast appeared */
    int severity;           /* 0=info, 1=success, 2=warning, 3=error */
    int position;           /* 0=bottom-right, 1=bottom-center, 2=top-right, 3=top-center */
    bool dismissed;         /* true = should not render */
    const Widget_ToastStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Popup: centered floating overlay container */
CEL_Define(W_Popup, {
    const char* title;      /* Optional popup title */
    bool visible;           /* Display state */
    bool backdrop;          /* Show dimming backdrop behind popup */
    int width;              /* Popup width (0 = 40 default) */
    int height;             /* Popup height (0 = fit) */
    const Widget_PopupStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Modal: popup with focus scope isolation + Escape dismiss */
CEL_Define(W_Modal, {
    const char* title;      /* Modal title */
    bool visible;           /* Display state */
    int width;              /* Modal width (0 = 50 default) */
    int height;             /* Modal height (0 = fit) */
    void (*on_dismiss)(void); /* Callback when Escape pressed */
    const Widget_ModalStyle* style; /* Visual overrides (NULL = defaults) */
});

/* Window: positioned floating panel with title bar and close */
CEL_Define(W_Window, {
    const char* title;      /* Window title (shown in title bar) */
    bool visible;           /* Display state */
    int x;                  /* Position X (0 = centered) */
    int y;                  /* Position Y (0 = centered) */
    int width;              /* Window width */
    int height;             /* Window height */
    int z_order;            /* Dynamic z-index for multi-window management */
    void (*on_close)(void); /* Callback when close triggered */
    const Widget_WindowStyle* style; /* Visual overrides (NULL = defaults) */
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
