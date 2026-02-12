/*
 * CELS Widgets - Clay Layout Function Declarations
 *
 * Each widget has a corresponding Clay layout function that reads the
 * widget's component data and emits Clay elements. These functions are
 * called per-frame by the Clay layout system when attached to entities
 * via CEL_Has(ClayUI, .layout_fn = w_button_layout).
 *
 * Layout functions are pure declarations: read component data, emit
 * CLAY() calls, no side effects. The clay_ncurses_renderer handles
 * all rendering generically from Clay render commands.
 *
 * Usage:
 *   #include <cels-widgets/layouts.h>
 *   CEL_Has(ClayUI, .layout_fn = w_button_layout);
 */

#ifndef CELS_WIDGETS_LAYOUTS_H
#define CELS_WIDGETS_LAYOUTS_H

#include <cels/cels.h>

struct ecs_world_t;

/* Text & Display */
extern void w_text_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_hint_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_canvas_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_info_box_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_badge_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_text_area_layout(struct ecs_world_t* world, cels_entity_t self);

/* Interactive */
extern void w_button_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_slider_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_toggle_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_cycle_layout(struct ecs_world_t* world, cels_entity_t self);

/* Progress & Metric */
extern void w_progress_bar_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_metric_layout(struct ecs_world_t* world, cels_entity_t self);

/* Containers */
extern void w_panel_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_divider_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_table_layout(struct ecs_world_t* world, cels_entity_t self);

/* Structural containers */
extern void w_collapsible_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_split_pane_layout(struct ecs_world_t* world, cels_entity_t self);

/* Radio */
extern void w_radio_button_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_radio_group_layout(struct ecs_world_t* world, cels_entity_t self);

/* Navigation */
extern void w_tab_bar_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_tab_content_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_status_bar_layout(struct ecs_world_t* world, cels_entity_t self);

/* List */
extern void w_list_view_layout(struct ecs_world_t* world, cels_entity_t self);
extern void w_list_item_layout(struct ecs_world_t* world, cels_entity_t self);

/* Navigation Group */
extern void w_navigation_group_layout(struct ecs_world_t* world, cels_entity_t self);

#endif /* CELS_WIDGETS_LAYOUTS_H */
