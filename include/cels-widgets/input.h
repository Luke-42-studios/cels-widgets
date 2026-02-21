/*
 * Copyright 2026 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
#include <cels/backend.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Focus State
 * ============================================================================ */

/* Current focus state -- tracks which widget has keyboard focus */
cel_state(W_FocusState, {
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
cel_component(W_NavigationScope, {
    bool wrap;              /* Wrap around at boundaries */
    int direction;          /* 0 = vertical, 1 = horizontal */
    int selected_index;     /* Currently selected child index */
    int child_count;        /* Total children in scope */
});

/* ============================================================================
 * Navigation State
 * ============================================================================ */

/* Navigation scope state -- tracks active navigation group for focus isolation */
cel_state(W_NavigationState, {
    cels_entity_t active_scope;     /* Entity ID of active NavigationGroup (0 = global) */
    int scope_depth;                /* Nesting depth for push/pop */
});

/* Scope management API */
extern void widgets_nav_scope_push(cels_entity_t scope_entity);
extern void widgets_nav_scope_pop(void);
extern cels_entity_t widgets_nav_scope_active(void);

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

/*
 * Text input behavioral system: processes raw_key into buffer edits.
 * Called from the focus system each frame with world, current input, and
 * previous input (for edge detection). Only processes entities that are
 * both focused and selected.
 */
struct ecs_world_t;
extern void text_input_system_run(struct ecs_world_t* world,
                                  const CELS_Input* input,
                                  const CELS_Input* prev_input);

/*
 * Check if any text input entity is currently focused+selected (active).
 * Used by the focus system to suppress q-quit and arrow navigation.
 */
extern bool text_input_is_active(struct ecs_world_t* world);

#ifdef __cplusplus
}
#endif

#endif /* CELS_WIDGETS_INPUT_H */
