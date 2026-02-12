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
 * Split Pane Navigation (Ctrl+Arrow)
 *
 * When Ctrl+Arrow is detected, find a W_SplitPane ancestor of the currently
 * focused NavigationGroup, determine which pane the focus is in, and switch
 * focus to the first selectable child in the other pane.
 * ============================================================================ */

/* Ctrl+Arrow key codes (must match tui_input.c definitions) */
#define CELS_KEY_CTRL_UP    600
#define CELS_KEY_CTRL_DOWN  601
#define CELS_KEY_CTRL_RIGHT 602
#define CELS_KEY_CTRL_LEFT  603

/* Check if entity is a descendant of ancestor via parent chain traversal */
static bool is_descendant_of(ecs_world_t* world, ecs_entity_t entity, ecs_entity_t ancestor) {
    ecs_entity_t current = entity;
    int depth = 0;
    while (current != 0 && depth < 32) {
        if (current == ancestor) return true;
        current = ecs_get_parent(world, current);
        depth++;
    }
    return false;
}

/* Find the first W_NavigationScope entity under a given parent (pane child) */
static ecs_entity_t find_nav_scope_under(ecs_world_t* world, ecs_entity_t parent) {
    /* Check direct children first */
    ecs_iter_t it = ecs_children(world, parent);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            if (ecs_has_id(world, it.entities[i], W_NavigationScopeID)) {
                return it.entities[i];
            }
        }
    }
    /* Check grandchildren (one level deep) */
    ecs_iter_t it2 = ecs_children(world, parent);
    while (ecs_children_next(&it2)) {
        for (int i = 0; i < it2.count; i++) {
            ecs_iter_t it3 = ecs_children(world, it2.entities[i]);
            while (ecs_children_next(&it3)) {
                for (int j = 0; j < it3.count; j++) {
                    if (ecs_has_id(world, it3.entities[j], W_NavigationScopeID)) {
                        return it3.entities[j];
                    }
                }
            }
        }
    }
    return 0;
}

static void process_split_pane_navigation(ecs_world_t* world, const CELS_Input* input) {
    /* Only act on Ctrl+Arrow edge (not held) */
    if (!input->has_raw_key) return;
    if (input->raw_key < CELS_KEY_CTRL_UP || input->raw_key > CELS_KEY_CTRL_LEFT) return;
    if (s_prev_input.has_raw_key && s_prev_input.raw_key == input->raw_key) return;

    W_SplitPane_ensure();
    W_NavigationScope_ensure();

    /* Query all W_SplitPane entities */
    ecs_query_t* q = ecs_query(world, {
        .terms = {{ .id = W_SplitPaneID }}
    });
    if (!q) return;

    ecs_iter_t qit = ecs_query_iter(world, q);
    while (ecs_query_next(&qit)) {
        for (int e = 0; e < qit.count; e++) {
            ecs_entity_t split_entity = qit.entities[e];
            const W_SplitPane* split = (const W_SplitPane*)ecs_get_id(
                world, split_entity, W_SplitPaneID);
            if (!split) continue;

            /* Filter Ctrl+Arrow by split direction:
             * Horizontal split (left|right): Ctrl+Left/Right switch panes
             * Vertical split (top|bottom): Ctrl+Up/Down switch panes */
            bool relevant = false;
            if (split->direction == 0) {
                relevant = (input->raw_key == CELS_KEY_CTRL_LEFT ||
                            input->raw_key == CELS_KEY_CTRL_RIGHT);
            } else {
                relevant = (input->raw_key == CELS_KEY_CTRL_UP ||
                            input->raw_key == CELS_KEY_CTRL_DOWN);
            }
            if (!relevant) continue;

            /* Get the two pane children (child 0 and child 1) */
            ecs_entity_t pane_children[2] = {0, 0};
            int pane_idx = 0;
            ecs_iter_t cit = ecs_children(world, split_entity);
            while (ecs_children_next(&cit) && pane_idx < 2) {
                for (int c = 0; c < cit.count && pane_idx < 2; c++) {
                    pane_children[pane_idx++] = cit.entities[c];
                }
            }
            if (pane_children[0] == 0 || pane_children[1] == 0) continue;

            /* Find which pane has the currently active NavigationScope */
            W_NavigationState_ensure();
            ecs_entity_t active_scope = W_NavigationState.active_scope;

            int current_pane = -1;
            if (active_scope != 0) {
                if (is_descendant_of(world, active_scope, pane_children[0]) ||
                    active_scope == pane_children[0]) {
                    current_pane = 0;
                } else if (is_descendant_of(world, active_scope, pane_children[1]) ||
                           active_scope == pane_children[1]) {
                    current_pane = 1;
                }
            }

            /* If no active scope found in either pane, try to find any
             * NavigationScope in pane 0 or pane 1 that has selected children */
            if (current_pane == -1) {
                ecs_entity_t nav0 = find_nav_scope_under(world, pane_children[0]);
                if (nav0 != 0) {
                    current_pane = 0;
                } else {
                    current_pane = 0; /* Default to pane 0 */
                }
            }

            /* Switch to the other pane */
            int target_pane = (current_pane == 0) ? 1 : 0;
            ecs_entity_t target_nav = find_nav_scope_under(world, pane_children[target_pane]);
            if (target_nav == 0) continue;

            /* Activate the target NavigationScope */
            W_NavigationState.active_scope = target_nav;

            /* Set selected_index to 0 in the target scope and ensure
             * at least the first child is selected */
            W_NavigationScope* target_scope = (W_NavigationScope*)ecs_get_mut_id(
                world, target_nav, W_NavigationScopeID);
            if (target_scope) {
                target_scope->selected_index = 0;
            }
        }
    }

    ecs_query_fini(q);
}

/* ============================================================================
 * Scrollable Container Navigation
 *
 * For each W_ScrollContainer entity:
 *   1. Auto-scroll: keep NavigationGroup's selected child visible
 *   2. Keyboard scroll: PgUp/PgDn/Home/End direct scroll control
 * ============================================================================ */

static void process_scrollable_navigation(ecs_world_t* world, const CELS_Input* input) {
    W_ScrollContainer_ensure();
    W_Scrollable_ensure();
    W_NavigationScope_ensure();

    ecs_query_t* q = ecs_query(world, {
        .terms = {{ .id = W_ScrollContainerID }}
    });
    if (!q) return;

    /* Edge-detect PgUp/PgDn/Home/End */
    bool pgup_edge  = (input->key_page_up   && !s_prev_input.key_page_up);
    bool pgdn_edge  = (input->key_page_down && !s_prev_input.key_page_down);
    bool home_edge  = (input->key_home      && !s_prev_input.key_home);
    bool end_edge   = (input->key_end       && !s_prev_input.key_end);

    ecs_iter_t qit = ecs_query_iter(world, q);
    while (ecs_query_next(&qit)) {
        for (int e = 0; e < qit.count; e++) {
            ecs_entity_t sc_entity = qit.entities[e];

            W_Scrollable* scr = (W_Scrollable*)ecs_get_mut_id(
                world, sc_entity, W_ScrollableID);
            if (!scr || scr->visible_count <= 0) continue;

            const W_ScrollContainer* sc = (const W_ScrollContainer*)ecs_get_id(
                world, sc_entity, W_ScrollContainerID);
            (void)sc;

            int visible = scr->visible_count;
            int total = scr->total_count;

            /* --- Auto-scroll to selected child --- */
            /* Find NavigationScope under this scrollable (child or grandchild) */
            ecs_entity_t nav = find_nav_scope_under(world, sc_entity);
            if (nav != 0) {
                const W_NavigationScope* scope = (const W_NavigationScope*)ecs_get_id(
                    world, nav, W_NavigationScopeID);
                if (scope) {
                    int sel = scope->selected_index;
                    /* Scroll up to show selected */
                    if (sel < scr->scroll_offset) {
                        scr->scroll_offset = sel;
                    }
                    /* Scroll down to show selected */
                    if (sel >= scr->scroll_offset + visible) {
                        scr->scroll_offset = sel - visible + 1;
                    }
                }
            }

            /* --- Keyboard scroll (edge-detected) --- */
            if (pgup_edge) {
                scr->scroll_offset -= visible;
            }
            if (pgdn_edge) {
                scr->scroll_offset += visible;
            }
            if (home_edge) {
                scr->scroll_offset = 0;
            }
            if (end_edge && total > visible) {
                scr->scroll_offset = total - visible;
            }

            /* Write back (ScrollClampSystem enforces bounds at PostUpdate) */
            ecs_set_id(world, sc_entity, W_ScrollableID,
                       sizeof(W_Scrollable), scr);
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
        process_split_pane_navigation(world, input);
        process_scrollable_navigation(world, input);
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
    W_SplitPane_ensure();
    W_ScrollContainer_ensure();

    cels_entity_t components[] = { W_FocusableID };
    cels_system_declare("W_FocusSystem", CELS_Phase_OnUpdate,
                        focus_system_run, components, 1);
}
