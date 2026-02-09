/*
 * CELS Widgets - Backend-agnostic widget component definitions
 *
 * Defines reusable UI widget components using CEL_Define. These components
 * are pure data -- backends (cels-ncurses, cels-sdl3, etc.) provide the
 * renderers via the feature/provider system.
 *
 * All widget components use the W_ prefix to distinguish them from
 * backend-specific types.
 *
 * Usage:
 *   #include <cels-widgets/widgets.h>
 *
 *   CEL_Composition(MyButton) {
 *       CEL_Has(W_Button, .label = "Play", .selected = true);
 *   }
 *
 * Module initialization:
 *   #include <cels-widgets/widgets.h>
 *   CEL_Build(App) {
 *       Widgets_init();
 *       // ... backend setup ...
 *   }
 */

#ifndef CELS_WIDGETS_H
#define CELS_WIDGETS_H

#include <cels/cels.h>

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
});

/* Info box: bordered box with title and content */
CEL_Define(W_InfoBox, {
    const char* title;      /* Box title */
    const char* content;    /* Content text */
    bool border;            /* Draw border (default true) */
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
});

/* Slider: labeled value slider with min/max */
CEL_Define(W_Slider, {
    const char* label;      /* Slider label text */
    float value;            /* Current value (0.0 - 1.0) */
    float min;              /* Minimum value (default 0) */
    float max;              /* Maximum value (default 1) */
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

#ifdef __cplusplus
}
#endif

#endif /* CELS_WIDGETS_H */
