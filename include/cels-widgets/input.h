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
 * Focus System Registration
 * ============================================================================ */

/*
 * Register the focus system. Called automatically by Widgets_init().
 * Can also be called manually during CEL_Build for explicit control.
 */
extern void widgets_focus_system_register(void);

#ifdef __cplusplus
}
#endif

#endif /* CELS_WIDGETS_INPUT_H */
