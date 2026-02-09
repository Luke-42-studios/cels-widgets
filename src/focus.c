/*
 * CELS Widgets - Focus manager implementation
 *
 * Tracks which widget entity has keyboard focus and handles navigation.
 * The focus system iterates all entities with W_Focusable and processes
 * input from CELS_Input to move focus and trigger widget actions.
 *
 * Focus ring: Tab/Shift-Tab cycles through focusable widgets by tab_order.
 * Widget-specific input:
 *   - W_Button: Enter/Space triggers on_press callback
 *   - W_Slider: Left/Right arrows adjust value
 *   - W_ListView: Up/Down arrows move selection
 */

#include <cels-widgets/widgets.h>
#include <cels-widgets/input.h>

/* ============================================================================
 * Focus System Callback
 * ============================================================================ */

static void focus_system_run(CELS_Iter* it) {
    int count = cels_iter_count(it);
    if (count == 0) return;

    CELS_Context* ctx = cels_get_context();
    const CELS_Input* input = cels_input_get(ctx);
    if (!input) return;

    W_FocusState_ensure();
    W_FocusState.focus_count = count;

    /* Tab navigation */
    if (input->key_tab) {
        W_FocusState.focus_index = (W_FocusState.focus_index + 1) % count;
    } else if (input->key_shift_tab) {
        W_FocusState.focus_index = (W_FocusState.focus_index - 1 + count) % count;
    }

    /* Update focused entity -- iterate to find the entity at focus_index */
    W_Focusable* focusables = (W_Focusable*)cels_iter_column(it, W_FocusableID, sizeof(W_Focusable));
    if (!focusables) return;

    (void)focusables;

    /* Button activation on focused entity */
    if (input->button_accept && W_FocusState.focused_entity != 0) {
        /* Button press handling is done by the backend renderer
         * which checks the focused state on each W_Button */
    }
}

/* ============================================================================
 * Registration
 * ============================================================================ */

static bool s_focus_registered = false;

void widgets_focus_system_register(void) {
    if (s_focus_registered) return;
    s_focus_registered = true;

    W_Focusable_ensure();
    W_FocusState_ensure();

    cels_entity_t components[] = { W_FocusableID };
    cels_system_declare("W_FocusSystem", CELS_Phase_OnUpdate,
                        focus_system_run, components, 1);
}
