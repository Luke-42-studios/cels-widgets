/*
 * CELS Widgets - Focus manager implementation
 *
 * Tracks which widget entity has keyboard focus and handles navigation.
 * The focus system iterates all entities with W_Focusable and processes
 * input from CELS_Input to move focus and trigger widget actions.
 *
 * Focus ring: Tab/Shift-Tab cycles through focusable widgets.
 *
 * NavigationGroup processing:
 *   - Iterates all entities with W_NavigationScope
 *   - Enumerates children with W_Selectable via ecs_children()
 *   - Arrow keys cycle selected_index within the scope
 *   - Sets W_Selectable.selected + W_InteractState.selected on children
 *   - Enter/Space triggers W_Button.on_press on the selected child
 *
 * Runs at CELS_Phase_OnUpdate (after composition, before render).
 */

#include <cels-widgets/widgets.h>
#include <cels-widgets/input.h>
#include <flecs.h>
#include <string.h>

/* ============================================================================
 * Previous Input State (edge detection for NavigationGroup)
 * ============================================================================ */

static CELS_Input s_prev_input = {0};

/* ============================================================================
 * Navigation Scope Management
 * ============================================================================ */

void widgets_nav_scope_push(cels_entity_t scope_entity) {
    W_NavigationState_ensure();
    W_NavigationState.active_scope = scope_entity;
    W_NavigationState.scope_depth++;
}

void widgets_nav_scope_pop(void) {
    W_NavigationState_ensure();
    if (W_NavigationState.scope_depth > 0) {
        W_NavigationState.scope_depth--;
    }
    if (W_NavigationState.scope_depth == 0) {
        W_NavigationState.active_scope = 0;
    }
}

cels_entity_t widgets_nav_scope_active(void) {
    W_NavigationState_ensure();
    return W_NavigationState.active_scope;
}

/* ============================================================================
 * NavigationGroup Child Processing
 *
 * For each W_NavigationScope entity, enumerate children that have
 * W_Selectable, handle arrow key input for cycling, and set selection
 * state on children.
 * ============================================================================ */

/* Max children per NavigationGroup -- reasonable upper bound for TUI menus */
#define MAX_NAV_CHILDREN 64

static void process_navigation_groups(ecs_world_t* world, const CELS_Input* input) {
    W_NavigationScope_ensure();
    W_Selectable_ensure();
    W_InteractState_ensure();
    W_Button_ensure();
    W_Collapsible_ensure();

    /* Query all entities with W_NavigationScope */
    ecs_query_t* q = ecs_query(world, {
        .terms = {{ .id = W_NavigationScopeID }}
    });
    if (!q) return;

    ecs_iter_t qit = ecs_query_iter(world, q);
    while (ecs_query_next(&qit)) {
        for (int e = 0; e < qit.count; e++) {
            ecs_entity_t nav_entity = qit.entities[e];

            W_NavigationScope* scope = (W_NavigationScope*)ecs_get_mut_id(
                world, nav_entity, W_NavigationScopeID);
            if (!scope) continue;

            /* Collect children that have W_Selectable */
            ecs_entity_t children[MAX_NAV_CHILDREN];
            int child_count = 0;

            ecs_iter_t cit = ecs_children(world, nav_entity);
            while (ecs_children_next(&cit)) {
                for (int c = 0; c < cit.count; c++) {
                    ecs_entity_t child = cit.entities[c];
                    /* Only include children that have W_Selectable */
                    if (ecs_has_id(world, child, W_SelectableID) &&
                        child_count < MAX_NAV_CHILDREN) {
                        children[child_count++] = child;
                    }
                }
            }

            scope->child_count = child_count;
            if (child_count == 0) continue;

            /* Clamp selected_index to valid range */
            if (scope->selected_index >= child_count) {
                scope->selected_index = child_count - 1;
            }
            if (scope->selected_index < 0) {
                scope->selected_index = 0;
            }

            /* Arrow key navigation (edge-detected) */
            bool nav_prev = false;
            bool nav_next = false;

            if (scope->direction == 0) {
                /* Vertical: Up/Down */
                nav_prev = (input->axis_left[1] < -0.5f &&
                            s_prev_input.axis_left[1] >= -0.5f);
                nav_next = (input->axis_left[1] > 0.5f &&
                            s_prev_input.axis_left[1] <= 0.5f);
            } else {
                /* Horizontal: Left/Right */
                nav_prev = (input->axis_left[0] < -0.5f &&
                            s_prev_input.axis_left[0] >= -0.5f);
                nav_next = (input->axis_left[0] > 0.5f &&
                            s_prev_input.axis_left[0] <= 0.5f);
            }

            if (nav_prev) {
                scope->selected_index--;
                if (scope->selected_index < 0) {
                    scope->selected_index = scope->wrap
                        ? (child_count - 1) : 0;
                }
            }
            if (nav_next) {
                scope->selected_index++;
                if (scope->selected_index >= child_count) {
                    scope->selected_index = scope->wrap
                        ? 0 : (child_count - 1);
                }
            }

            /* Apply selection state to children */
            for (int i = 0; i < child_count; i++) {
                ecs_entity_t child = children[i];
                bool is_selected = (i == scope->selected_index);

                /* Set W_Selectable.selected */
                W_Selectable sel_val = { .selected = is_selected };
                ecs_set_id(world, child, W_SelectableID,
                           sizeof(W_Selectable), &sel_val);

                /* Set W_InteractState.selected (preserve other fields) */
                const W_InteractState* ist = (const W_InteractState*)ecs_get_id(
                    world, child, W_InteractStateID);
                if (ist) {
                    W_InteractState new_ist = *ist;
                    new_ist.selected = is_selected;
                    ecs_set_id(world, child, W_InteractStateID,
                               sizeof(W_InteractState), &new_ist);
                }
            }

            /* Button activation: Enter/Space on selected child */
            bool accept_pressed = (input->button_accept &&
                                   !s_prev_input.button_accept);
            if (accept_pressed && scope->selected_index >= 0 &&
                scope->selected_index < child_count) {
                ecs_entity_t selected_child = children[scope->selected_index];
                const W_Button* btn = (const W_Button*)ecs_get_id(
                    world, selected_child, W_ButtonID);
                if (btn && btn->on_press) {
                    btn->on_press();
                }

                /* Collapsible toggle: Enter/Space toggles collapsed state */
                if (ecs_has_id(world, selected_child, W_CollapsibleID)) {
                    W_Collapsible* col = (W_Collapsible*)ecs_get_mut_id(
                        world, selected_child, W_CollapsibleID);
                    if (col) {
                        col->collapsed = !col->collapsed;
                        ecs_set_id(world, selected_child, W_CollapsibleID,
                                   sizeof(W_Collapsible), col);
                    }
                }
            }
        }
    }

    ecs_query_fini(q);
}

/* ============================================================================
 * Focus System Callback
 * ============================================================================ */

static void focus_system_run(CELS_Iter* it) {
    int count = cels_iter_count(it);

    CELS_Context* ctx = cels_get_context();
    const CELS_Input* input = cels_input_get(ctx);
    if (!input) return;

    W_FocusState_ensure();
    W_FocusState.focus_count = count;

    /* Tab navigation for focus ring (only when focusable entities exist) */
    if (count > 0) {
        if (input->key_tab) {
            W_FocusState.focus_index = (W_FocusState.focus_index + 1) % count;
        } else if (input->key_shift_tab) {
            W_FocusState.focus_index = (W_FocusState.focus_index - 1 + count) % count;
        }
    }

    /* Process NavigationGroup entities for arrow key selection cycling */
    ecs_world_t* world = cels_get_world(ctx);
    if (world) {
        process_navigation_groups(world, input);
    }

    /* Store input for edge detection on next frame */
    memcpy((void*)&s_prev_input, input, sizeof(CELS_Input));
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
    W_NavigationState_ensure();
    W_NavigationScope_ensure();
    W_Selectable_ensure();
    W_InteractState_ensure();
    W_Collapsible_ensure();

    cels_entity_t components[] = { W_FocusableID };
    cels_system_declare("W_FocusSystem", CELS_Phase_OnUpdate,
                        focus_system_run, components, 1);
}
