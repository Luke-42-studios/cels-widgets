/*
 * CELS Widgets - Focus management and input handling
 *
 * Provides a focus manager that tracks which widget entity has keyboard
 * focus and handles tab navigation between focusable widgets.
 *
 * Widgets declare focusability by adding the W_Focusable component.
 * The focus system runs at CELS_Phase_OnUpdate and:
 *   - Moves focus on Tab / Shift+Tab
 *   - Triggers W_Button.on_press on Enter/Space when focused
 *   - Adjusts W_Slider value on Left/Right arrows when focused
 *   - Moves W_ListView selection on Up/Down arrows when focused
 *
 * Usage:
 *   #include <cels-widgets/input.h>
 *
 *   CEL_Build(App) {
 *       Widgets_init();  // registers focus system automatically
 *   }
 */

#ifndef CELS_WIDGETS_INPUT_H
#define CELS_WIDGETS_INPUT_H

#include <cels/cels.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Focus State
 * ============================================================================ */

/* Current focus state -- tracks which widget has keyboard focus */
CEL_State(W_FocusState, {
    cels_entity_t focused_entity;   /* Entity with current focus (0 = none) */
    int focus_count;                /* Total focusable widget count */
    int focus_index;                /* Current index in focus ring */
});

/* ============================================================================
 * Navigation Scope
 * ============================================================================ */

/* NavigationScope: navigation group scope state for automatic focus/selection
 * management. A NavigationScope entity owns a group of child widgets and
 * drives selection cycling via input. Direction: 0=vertical, 1=horizontal. */
CEL_Define(W_NavigationScope, {
    bool wrap;              /* Wrap around at boundaries */
    int direction;          /* 0 = vertical, 1 = horizontal */
    int selected_index;     /* Currently selected child index */
    int child_count;        /* Total children in scope */
});

/* ============================================================================
 * Focus System Registration
 * ============================================================================ */

/*
 * Register the focus system. Called automatically by Widgets_init().
 * Can also be called manually during CEL_Build for explicit control.
 */
extern void widgets_focus_system_register(void);

/*
 * Register behavioral systems (RangeClamp, ScrollClamp, NavigationScope).
 * Called by Widgets_init() after behavioral components are registered.
 * Forward declaration -- implementation in Plan 03.
 */
extern void widgets_behavioral_systems_register(void);

#ifdef __cplusplus
}
#endif

#endif /* CELS_WIDGETS_INPUT_H */
