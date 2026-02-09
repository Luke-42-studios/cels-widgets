/*
 * CELS Widgets - Clay Layout Implementations
 *
 * Each layout function reads the widget's component data via ecs_get_id()
 * and emits Clay elements via CEL_Clay() scopes. Container widgets use
 * CEL_Clay_Children() to insert child compositions into the layout tree.
 *
 * All rendering is handled by the generic Clay renderer (e.g.,
 * clay_ncurses_renderer). No per-widget renderer code is needed.
 *
 * Visual resolution:
 *   - Interactive widgets use w_resolve_visual() for state-to-color mapping
 *   - Display widgets read semantic theme tokens directly
 *   - All CLAY_TEXT calls include .userData = w_pack_text_attr() for
 *     text attribute propagation to the renderer
 *
 * Patterns:
 *   - CEL_Clay(...) { } for element containers
 *   - CLAY_TEXT(str, config) for text
 *   - CEL_Clay_Text(buf, len) for dynamic strings
 *   - CEL_Clay_Children() for child entity insertion
 *   - Widget_get_theme() for consistent theming
 *   - w_resolve_visual() for interactive widget state resolution
 */

#include <cels-widgets/widgets.h>
#include <cels-widgets/layouts.h>
#include <cels-widgets/theme.h>
#include <cels-widgets/style.h>
#include <cels-clay/clay_layout.h>
#include <clay.h>
#include <flecs.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Theme Singleton
 * ============================================================================ */

static const Widget_Theme* s_active_theme = NULL;
static bool s_theme_dirty = false;

const Widget_Theme* Widget_get_theme(void) {
    return s_active_theme ? s_active_theme : &Widget_THEME_DEFAULT;
}

void Widget_set_theme(const Widget_Theme* theme) {
    s_active_theme = theme;
    s_theme_dirty = true;
}

bool Widget_theme_changed(void) {
    bool dirty = s_theme_dirty;
    s_theme_dirty = false;
    return dirty;
}

/* ============================================================================
 * Helper: status color from theme (semantic tokens)
 * ============================================================================ */

static CEL_Color status_color(const Widget_Theme* t, int status) {
    switch (status) {
        case 1: return t->status_success.color;
        case 2: return t->status_warning.color;
        case 3: return t->status_error.color;
        default: return t->content.color;
    }
}

/* ============================================================================
 * Text & Display Layouts
 *
 * Display widgets use direct theme token access (no w_resolve_visual)
 * since they have no interactive state. Style overrides use alpha sentinel.
 * ============================================================================ */

void w_text_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Text* d = (const W_Text*)ecs_get_id(world, self, W_Text_ensure());
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_TextStyle* s = d->style;

    CEL_Color text_fg = (s && s->fg.a > 0) ? s->fg : t->content.color;
    CEL_TextAttr text_attr = (s && (s->text_attr.bold || s->text_attr.dim
        || s->text_attr.underline || s->text_attr.reverse || s->text_attr.italic))
        ? s->text_attr : t->content.attr;

    Clay_ChildAlignment align = {0};
    if (d->align == 1) align.x = CLAY_ALIGN_X_CENTER;
    else if (d->align == 2) align.x = CLAY_ALIGN_X_RIGHT;

    CEL_Clay(
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .childAlignment = align
        }
    ) {
        CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
            CLAY_TEXT_CONFIG({ .textColor = text_fg,
                              .userData = w_pack_text_attr(text_attr) }));
    }
}

void w_hint_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Hint* d = (const W_Hint*)ecs_get_id(world, self, W_Hint_ensure());
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_HintStyle* s = d->style;

    CEL_Color text_fg = (s && s->fg.a > 0) ? s->fg : t->content_muted.color;
    CEL_TextAttr text_attr = (s && (s->text_attr.bold || s->text_attr.dim
        || s->text_attr.underline || s->text_attr.reverse || s->text_attr.italic))
        ? s->text_attr : t->content_muted.attr;

    CEL_Clay(
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .top = 1 }
        }
    ) {
        CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
            CLAY_TEXT_CONFIG({ .textColor = text_fg,
                              .userData = w_pack_text_attr(text_attr) }));
    }
}

void w_canvas_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Canvas* d = (const W_Canvas*)ecs_get_id(world, self, W_Canvas_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_CanvasStyle* s = d->style;

    /* Width: style override > prop width > GROW (divide by AR for terminal cols) */
    Clay_SizingAxis w_sizing;
    if (s) {
        Clay_SizingAxis prop_default = (d->width > 0)
            ? CLAY_SIZING_FIXED((float)d->width / CEL_CELL_ASPECT_RATIO) : CLAY_SIZING_GROW(0);
        w_sizing = Widget_resolve_width(s->width, prop_default);
    } else {
        w_sizing = (d->width > 0) ? CLAY_SIZING_FIXED((float)d->width / CEL_CELL_ASPECT_RATIO) : CLAY_SIZING_GROW(0);
    }

    /* Height: style override or FIXED(3) */
    Clay_SizingAxis h_sizing = s
        ? Widget_resolve_sizing(s->height, CLAY_SIZING_FIXED(3))
        : CLAY_SIZING_FIXED(3);

    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface_raised.color;
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    CEL_Color title_fg = t->primary.color;
    CEL_TextAttr title_attr = t->primary.attr;

    CEL_Clay(
        .layout = {
            .sizing = { .width = w_sizing, .height = h_sizing },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
        },
        .border = {
            .color = bdr_color,
            .width = CLAY_BORDER_OUTSIDE(1)
        },
        .backgroundColor = bg_color,
        .cornerRadius = { .topLeft = 1, .topRight = 1 }
    ) {
        if (d->title) {
            CLAY_TEXT(CEL_Clay_Text(d->title, (int)strlen(d->title)),
                CLAY_TEXT_CONFIG({ .textColor = title_fg,
                                  .userData = w_pack_text_attr(title_attr) }));
        }
    }
}

void w_info_box_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_InfoBox* d = (const W_InfoBox*)ecs_get_id(world, self, W_InfoBox_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_InfoBoxStyle* s = d->style;

    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface_raised.color;
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    CEL_Color title_fg = t->primary.color;
    CEL_TextAttr title_attr = t->primary.attr;
    CEL_Color content_fg = (s && s->fg.a > 0) ? s->fg : t->content.color;
    CEL_TextAttr content_attr = (s && (s->text_attr.bold || s->text_attr.dim
        || s->text_attr.underline || s->text_attr.reverse || s->text_attr.italic))
        ? s->text_attr : t->content.attr;

    if (d->border) {
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(3) },
                .padding = CLAY_PADDING_ALL(1),
                .childGap = 1
            },
            .border = {
                .color = bdr_color,
                .width = CLAY_BORDER_OUTSIDE(1)
            },
            .backgroundColor = bg_color,
            .cornerRadius = { .topLeft = 1, .topRight = 1 }
        ) {
            if (d->title) {
                CLAY_TEXT(CEL_Clay_Text(d->title, (int)strlen(d->title)),
                    CLAY_TEXT_CONFIG({ .textColor = title_fg,
                                      .userData = w_pack_text_attr(title_attr) }));
            }
            if (d->content) {
                CLAY_TEXT(CEL_Clay_Text(d->content, (int)strlen(d->content)),
                    CLAY_TEXT_CONFIG({ .textColor = content_fg,
                                      .userData = w_pack_text_attr(content_attr) }));
            }
        }
    } else {
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = CLAY_SIZING_GROW(0) },
                .childGap = 1
            }
        ) {
            if (d->title) {
                CLAY_TEXT(CEL_Clay_Text(d->title, (int)strlen(d->title)),
                    CLAY_TEXT_CONFIG({ .textColor = title_fg,
                                      .userData = w_pack_text_attr(title_attr) }));
            }
            if (d->content) {
                CLAY_TEXT(CEL_Clay_Text(d->content, (int)strlen(d->content)),
                    CLAY_TEXT_CONFIG({ .textColor = content_fg,
                                      .userData = w_pack_text_attr(content_attr) }));
            }
        }
    }
}

void w_badge_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Badge* d = (const W_Badge*)ecs_get_id(world, self, W_Badge_ensure());
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_BadgeStyle* s = d->style;

    /* Badge bg: style badge_color > props r/g/b > theme accent */
    CEL_Color badge_bg;
    if (s && s->badge_color.a > 0) {
        badge_bg = s->badge_color;
    } else if (d->r || d->g || d->b) {
        badge_bg = (CEL_Color){ d->r, d->g, d->b, 255 };
    } else {
        badge_bg = t->accent.color;
    }

    CEL_Color text_fg = (s && s->fg.a > 0) ? s->fg : t->primary_content.color;
    CEL_TextAttr text_attr = (s && (s->text_attr.bold || s->text_attr.dim
        || s->text_attr.underline || s->text_attr.reverse || s->text_attr.italic))
        ? s->text_attr : (CEL_TextAttr){0};

    CEL_Clay(
        .layout = {
            .sizing = { .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 }
        },
        .backgroundColor = badge_bg
    ) {
        CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
            CLAY_TEXT_CONFIG({ .textColor = text_fg,
                              .userData = w_pack_text_attr(text_attr) }));
    }
}

void w_text_area_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_TextArea* d = (const W_TextArea*)ecs_get_id(world, self, W_TextArea_ensure());
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_TextAreaStyle* s = d->style;

    /* Read W_Scrollable for scroll state (behavioral component, preparatory) */
    const W_Scrollable* scr = (const W_Scrollable*)ecs_get_id(world, self, W_Scrollable_ensure());
    (void)scr; /* Available for future scroll offset control */

    CEL_Color text_fg = (s && s->fg.a > 0) ? s->fg : t->content.color;
    CEL_TextAttr text_attr = (s && (s->text_attr.bold || s->text_attr.dim
        || s->text_attr.underline || s->text_attr.reverse || s->text_attr.italic))
        ? s->text_attr : t->content.attr;

    Clay_SizingAxis w_sizing = (d->max_width > 0)
        ? CLAY_SIZING_FIXED((float)d->max_width)
        : CLAY_SIZING_GROW(0);
    Clay_SizingAxis h_sizing = (d->max_height > 0)
        ? CLAY_SIZING_FIXED((float)d->max_height)
        : CLAY_SIZING_GROW(0);

    if (d->scrollable) {
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = w_sizing, .height = h_sizing },
                .padding = CLAY_PADDING_ALL(1)
            },
            .clip = {
                .vertical = true,
                .childOffset = Clay_GetScrollOffset()
            }
        ) {
            CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
                CLAY_TEXT_CONFIG({ .textColor = text_fg,
                                  .userData = w_pack_text_attr(text_attr) }));
        }
    } else {
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = w_sizing, .height = h_sizing },
                .padding = CLAY_PADDING_ALL(1)
            }
        ) {
            CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
                CLAY_TEXT_CONFIG({ .textColor = text_fg,
                                  .userData = w_pack_text_attr(text_attr) }));
        }
    }
}

/* ============================================================================
 * Interactive Layouts
 *
 * Interactive widgets use w_resolve_visual() for centralized state-to-visual
 * mapping. Priority: disabled > selected > focused > normal.
 * ============================================================================ */

void w_button_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Button* d = (const W_Button*)ecs_get_id(world, self, W_Button_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ButtonStyle* s = d->style;

    /* Read W_InteractState if available (set by composition macro) */
    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_ensure());
    bool disabled = ist ? ist->disabled : false;
    bool focused = ist ? ist->focused : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_ensure());
    bool selected = sel ? sel->selected : false;

    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_ON_SELECT,
        selected, focused, disabled);

    /* Selected-state specific overrides */
    CEL_Color final_bg = v.bg;
    CEL_Color final_fg = v.fg;
    if (selected && s && s->bg_selected.a > 0) final_bg = s->bg_selected;
    if (selected && s && s->fg_selected.a > 0) final_fg = s->fg_selected;

    /* Sizing: style override or defaults (GROW x FIXED(1)) */
    Clay_SizingAxis w_axis = s
        ? Widget_resolve_width(s->width, CLAY_SIZING_GROW(0))
        : CLAY_SIZING_GROW(0);
    Clay_SizingAxis h_axis = s
        ? Widget_resolve_sizing(s->height, CLAY_SIZING_FIXED(1))
        : CLAY_SIZING_FIXED(1);

    /* Padding: style override or default {1, 1, 0, 0} */
    Clay_Padding pad = { .left = 1, .right = 1 };
    if (s && (s->padding.left || s->padding.right || s->padding.top || s->padding.bottom)) {
        pad = (Clay_Padding){ .left = s->padding.left, .right = s->padding.right,
                              .top = s->padding.top, .bottom = s->padding.bottom };
    }

    /* Alignment: 0=default(center), 1=left, 2=center, 3=right */
    Clay_ChildAlignment align = { .x = CLAY_ALIGN_X_CENTER };
    if (s && s->align > 0) {
        if (s->align == 1)      align.x = CLAY_ALIGN_X_LEFT;
        else if (s->align == 3) align.x = CLAY_ALIGN_X_RIGHT;
    }

    CEL_Clay(
        .layout = {
            .sizing = { .width = w_axis, .height = h_axis },
            .padding = pad,
            .childAlignment = align
        },
        .backgroundColor = final_bg,
        .border = {
            .color = v.border_color,
            .width = { v.show_border?1:0, v.show_border?1:0, v.show_border?1:0, v.show_border?1:0, 0 }
        }
    ) {
        if (selected) {
            CLAY_TEXT(CLAY_STRING("> "),
                CLAY_TEXT_CONFIG({ .textColor = final_fg,
                                  .userData = w_pack_text_attr(v.text_attr) }));
        }
        CLAY_TEXT(CEL_Clay_Text(d->label, (int)strlen(d->label)),
            CLAY_TEXT_CONFIG({ .textColor = final_fg,
                              .userData = w_pack_text_attr(v.text_attr) }));
    }
}

void w_slider_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Slider* d = (const W_Slider*)ecs_get_id(world, self, W_Slider_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_SliderStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_ensure());
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_ensure());
    bool selected = sel ? sel->selected : false;

    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_NONE,
        selected, false, disabled);

    /* Bar fill color: style override or theme primary */
    CEL_Color bar_color = (s && s->fill_color.a > 0) ? s->fill_color : t->primary.color;

    /* Read W_RangeValueF for range data (behavioral component) */
    const W_RangeValueF* rv = (const W_RangeValueF*)ecs_get_id(world, self, W_RangeValueF_ensure());
    float val = rv ? rv->value : 0.0f;
    float rmin = rv ? rv->min : 0.0f;
    float rmax = rv ? rv->max : 1.0f;

    float range = (rmax > rmin) ? (rmax - rmin) : 1.0f;
    float norm = (val - rmin) / range;

    char bar_buf[32];
    int bar_width = 20;
    int filled = (int)(norm * bar_width);
    bar_buf[0] = '[';
    for (int j = 0; j < bar_width; j++) {
        bar_buf[j + 1] = (j < filled) ? '=' : ' ';
    }
    bar_buf[bar_width + 1] = ']';
    bar_buf[bar_width + 2] = '\0';

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childGap = 1
        },
        .backgroundColor = v.bg
    ) {
        /* Label */
        char label_buf[32];
        int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
        CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
            CLAY_TEXT_CONFIG({ .textColor = v.fg,
                              .userData = w_pack_text_attr(v.text_attr) }));

        /* Bar */
        CLAY_TEXT(CEL_Clay_Text(bar_buf, (int)strlen(bar_buf)),
            CLAY_TEXT_CONFIG({ .textColor = bar_color,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
    }
}

void w_toggle_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Toggle* d = (const W_Toggle*)ecs_get_id(world, self, W_Toggle_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ToggleStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_ensure());
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_ensure());
    bool selected = sel ? sel->selected : false;

    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_NONE,
        selected, false, disabled);

    const char* state_str = d->value ? "ON" : "OFF";
    /* State indicator: style overrides or theme status colors */
    CEL_Color state_color;
    if (d->value) {
        state_color = (s && s->on_color.a > 0) ? s->on_color : t->status_success.color;
    } else {
        state_color = (s && s->off_color.a > 0) ? s->off_color : t->status_error.color;
    }

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childGap = 1
        },
        .backgroundColor = v.bg
    ) {
        char label_buf[32];
        int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
        CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
            CLAY_TEXT_CONFIG({ .textColor = v.fg,
                              .userData = w_pack_text_attr(v.text_attr) }));

        char toggle_buf[16];
        int toggle_len = snprintf(toggle_buf, sizeof(toggle_buf), "[%s]", state_str);
        CLAY_TEXT(CEL_Clay_Text(toggle_buf, toggle_len),
            CLAY_TEXT_CONFIG({ .textColor = state_color,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
    }
}

void w_cycle_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Cycle* d = (const W_Cycle*)ecs_get_id(world, self, W_Cycle_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_CycleStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_ensure());
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_ensure());
    bool selected = sel ? sel->selected : false;

    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_NONE,
        selected, false, disabled);

    /* Arrow color: selected = border_focused, normal = content_muted */
    CEL_Color arrow_color = selected ? t->border_focused.color : t->content_muted.color;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childGap = 1
        },
        .backgroundColor = v.bg
    ) {
        char label_buf[32];
        int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
        CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
            CLAY_TEXT_CONFIG({ .textColor = v.fg,
                              .userData = w_pack_text_attr(v.text_attr) }));

        CLAY_TEXT(CLAY_STRING("[<]"),
            CLAY_TEXT_CONFIG({ .textColor = arrow_color,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));

        const char* val = d->value ? d->value : "";
        char val_buf[24];
        int val_len = snprintf(val_buf, sizeof(val_buf), "%-15s", val);
        CLAY_TEXT(CEL_Clay_Text(val_buf, val_len),
            CLAY_TEXT_CONFIG({ .textColor = t->content.color,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));

        CLAY_TEXT(CLAY_STRING("[>]"),
            CLAY_TEXT_CONFIG({ .textColor = arrow_color,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
    }
}

/* ============================================================================
 * Progress & Metric Layouts
 * ============================================================================ */

void w_progress_bar_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_ProgressBar* d = (const W_ProgressBar*)ecs_get_id(world, self, W_ProgressBar_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ProgressBarStyle* s = d->style;

    /* Read W_RangeValueF for progress value (behavioral component) */
    const W_RangeValueF* rv = (const W_RangeValueF*)ecs_get_id(world, self, W_RangeValueF_ensure());
    float val = rv ? rv->value : 0.0f;

    CEL_Color fill_color = (s && s->fill_color.a > 0) ? s->fill_color : t->progress_fill.color;
    if (d->color_by_value) {
        if (val < 0.33f)      fill_color = t->status_error.color;
        else if (val < 0.66f) fill_color = t->status_warning.color;
        else                  fill_color = t->status_success.color;
    }

    CEL_Color label_fg = (s && s->fg.a > 0) ? s->fg : t->content.color;
    CEL_TextAttr label_attr = (s && (s->text_attr.bold || s->text_attr.dim
        || s->text_attr.underline || s->text_attr.reverse || s->text_attr.italic))
        ? s->text_attr : t->content.attr;
    CEL_Color pct_fg = t->content_muted.color;

    int bar_width = 20;
    int filled = (int)(val * bar_width);
    char bar_buf[32];
    bar_buf[0] = '[';
    for (int j = 0; j < bar_width; j++) {
        bar_buf[j + 1] = (j < filled) ? '#' : ' ';
    }
    bar_buf[bar_width + 1] = ']';
    bar_buf[bar_width + 2] = '\0';

    char pct_buf[8];
    int pct_len = snprintf(pct_buf, sizeof(pct_buf), "%3d%%", (int)(val * 100));

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .childGap = 1
        }
    ) {
        if (d->label) {
            char label_buf[32];
            int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
            CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
                CLAY_TEXT_CONFIG({ .textColor = label_fg,
                                  .userData = w_pack_text_attr(label_attr) }));
        }
        CLAY_TEXT(CEL_Clay_Text(bar_buf, (int)strlen(bar_buf)),
            CLAY_TEXT_CONFIG({ .textColor = fill_color,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
        CLAY_TEXT(CEL_Clay_Text(pct_buf, pct_len),
            CLAY_TEXT_CONFIG({ .textColor = pct_fg,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
    }
}

void w_metric_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Metric* d = (const W_Metric*)ecs_get_id(world, self, W_Metric_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_MetricStyle* s = d->style;

    CEL_Color val_color = status_color(t, d->status);
    CEL_Color label_fg = (s && s->fg.a > 0) ? s->fg : t->content_muted.color;
    CEL_TextAttr label_attr = (s && (s->text_attr.bold || s->text_attr.dim
        || s->text_attr.underline || s->text_attr.reverse || s->text_attr.italic))
        ? s->text_attr : t->content_muted.attr;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .childGap = 1
        }
    ) {
        if (d->label) {
            char label_buf[32];
            int label_len = snprintf(label_buf, sizeof(label_buf), "%-16s", d->label);
            CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
                CLAY_TEXT_CONFIG({ .textColor = label_fg,
                                  .userData = w_pack_text_attr(label_attr) }));
        }
        if (d->value) {
            CLAY_TEXT(CEL_Clay_Text(d->value, (int)strlen(d->value)),
                CLAY_TEXT_CONFIG({ .textColor = val_color,
                                  .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
        }
    }
}

/* ============================================================================
 * Container Layouts
 * ============================================================================ */

void w_panel_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Panel* d = (const W_Panel*)ecs_get_id(world, self, W_Panel_ensure());
    (void)d; /* d may be NULL if no props were set -- still render container */
    const Widget_Theme* t = Widget_get_theme();
    const Widget_PanelStyle* s = (d ? d->style : NULL);

    /* Sizing: style override or GROW both axes */
    Clay_SizingAxis w_axis = s
        ? Widget_resolve_width(s->width, CLAY_SIZING_GROW(0))
        : CLAY_SIZING_GROW(0);
    Clay_SizingAxis h_axis = s
        ? Widget_resolve_sizing(s->height, CLAY_SIZING_GROW(0))
        : CLAY_SIZING_GROW(0);

    /* Padding: style override or ALL(1) */
    Clay_Padding pad = CLAY_PADDING_ALL(1);
    if (s && (s->padding.left || s->padding.right || s->padding.top || s->padding.bottom)) {
        pad = (Clay_Padding){ .left = s->padding.left, .right = s->padding.right,
                              .top = s->padding.top, .bottom = s->padding.bottom };
    }

    /* Colors: style override or theme */
    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface_raised.color;
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    CEL_Color title_fg = t->primary.color;
    CEL_TextAttr title_attr = t->primary.attr;

    /* Border: style override or always-on */
    Clay_BorderWidth border_w = CLAY_BORDER_OUTSIDE(1);
    CEL_BorderMode border_mode = s ? s->border : CEL_BORDER_DEFAULT;
    if (border_mode == CEL_BORDER_NONE) {
        border_w = (Clay_BorderWidth){0};
    }
    /* CEL_BORDER_DEFAULT and CEL_BORDER_ALWAYS both show border (panel default) */

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = w_axis, .height = h_axis },
            .padding = pad,
            .childGap = 1
        },
        .backgroundColor = bg_color,
        .border = {
            .color = bdr_color,
            .width = border_w
        },
        .cornerRadius = { .topLeft = 1, .topRight = 1 }
    ) {
        if (d && d->title) {
            CLAY_TEXT(CEL_Clay_Text(d->title, (int)strlen(d->title)),
                CLAY_TEXT_CONFIG({ .textColor = title_fg,
                                  .userData = w_pack_text_attr(title_attr) }));
        }
        CEL_Clay_Children();
    }
}

void w_divider_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Divider* d = (const W_Divider*)ecs_get_id(world, self, W_Divider_ensure());
    const Widget_Theme* t = Widget_get_theme();
    const Widget_DividerStyle* s = (d ? d->style : NULL);

    CEL_Color div_color = (s && s->bg.a > 0) ? s->bg : t->divider.color;
    bool vertical = d ? d->vertical : false;

    if (vertical) {
        CEL_Clay(
            .layout = {
                .sizing = { .width = CLAY_SIZING_FIXED(1), .height = CLAY_SIZING_GROW(0) }
            },
            .backgroundColor = div_color
        ) {}
    } else {
        CEL_Clay(
            .layout = {
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) }
            },
            .backgroundColor = div_color
        ) {}
    }
}

void w_table_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Table* d = (const W_Table*)ecs_get_id(world, self, W_Table_ensure());
    if (!d || d->row_count <= 0) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_TableStyle* s = d->style;

    CEL_Color key_fg = (s && s->fg.a > 0) ? s->fg : t->content_muted.color;
    CEL_TextAttr key_attr = t->content_muted.attr;
    CEL_Color val_fg = t->content.color;
    CEL_TextAttr val_attr = t->content.attr;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .childGap = 0
        }
    ) {
        for (int i = 0; i < d->row_count; i++) {
            const char* key = (d->keys && d->keys[i]) ? d->keys[i] : "";
            const char* val = (d->values && d->values[i]) ? d->values[i] : "";

            CEL_Clay(
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
                    .childGap = 1
                }
            ) {
                char key_buf[32];
                int key_len = snprintf(key_buf, sizeof(key_buf), "%-16s", key);
                CLAY_TEXT(CEL_Clay_Text(key_buf, key_len),
                    CLAY_TEXT_CONFIG({ .textColor = key_fg,
                                      .userData = w_pack_text_attr(key_attr) }));
                CLAY_TEXT(CEL_Clay_Text(val, (int)strlen(val)),
                    CLAY_TEXT_CONFIG({ .textColor = val_fg,
                                      .userData = w_pack_text_attr(val_attr) }));
            }
        }
    }
}

/* ============================================================================
 * Radio Layouts
 * ============================================================================ */

void w_radio_button_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_RadioButton* d = (const W_RadioButton*)ecs_get_id(world, self, W_RadioButton_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_RadioButtonStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_ensure());
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_ensure());
    bool selected = sel ? sel->selected : false;

    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_NONE,
        selected, false, disabled);

    const char* marker = selected ? "(*)" : "( )";
    /* Use resolved fg for selected, content_muted for unselected */
    CEL_Color text_color = selected ? v.fg : t->content_muted.color;
    CEL_TextAttr text_attr = v.text_attr;

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%s %s", marker, d->label);

    CEL_Clay(
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1 }
        }
    ) {
        CLAY_TEXT(CEL_Clay_Text(buf, len),
            CLAY_TEXT_CONFIG({ .textColor = text_color,
                              .userData = w_pack_text_attr(text_attr) }));
    }
}

void w_radio_group_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_RadioGroup* d = (const W_RadioGroup*)ecs_get_id(world, self, W_RadioGroup_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_RadioGroupStyle* s = d->style;

    CEL_Color header_fg = (s && s->fg.a > 0) ? s->fg : t->primary.color;
    CEL_TextAttr header_attr = t->primary.attr;

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "Radio Group %d (%d/%d)",
                       d->group_id, d->selected_index + 1, d->count);

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .childGap = 0
        }
    ) {
        CLAY_TEXT(CEL_Clay_Text(buf, len),
            CLAY_TEXT_CONFIG({ .textColor = header_fg,
                              .userData = w_pack_text_attr(header_attr) }));
        CEL_Clay_Children();
    }
}

/* ============================================================================
 * Navigation Layouts
 * ============================================================================ */

void w_tab_bar_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_TabBar* d = (const W_TabBar*)ecs_get_id(world, self, W_TabBar_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_TabBarStyle* s = d->style;

    CEL_Color bar_bg = (s && s->bg.a > 0) ? s->bg : t->surface_alt.color;
    CEL_Color active_fg = t->primary.color;
    CEL_TextAttr active_attr = t->primary.attr;
    CEL_Color inactive_fg = t->content_muted.color;
    CEL_Color active_tab_bg = (s && s->active_bg.a > 0) ? s->active_bg : t->surface_raised.color;
    CEL_Color active_border = t->primary.color;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(2) },
            .childGap = 0,
            .childAlignment = { .y = CLAY_ALIGN_Y_BOTTOM }
        },
        .backgroundColor = bar_bg
    ) {
        for (int i = 0; i < d->count; i++) {
            const char* name = (d->labels && d->labels[i]) ? d->labels[i] : "?";
            bool active = (i == d->active);
            CEL_Color tab_fg = active ? active_fg : inactive_fg;

            char tab_buf[32];
            int tab_len = snprintf(tab_buf, sizeof(tab_buf), " %d:%s ", i + 1, name);

            if (active) {
                /* Active tab: 2 rows tall with rounded top corners */
                CEL_Clay(
                    .layout = {
                        .sizing = { .height = CLAY_SIZING_FIXED(2) },
                        .padding = { .left = 1, .right = 1 },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = active_tab_bg,
                    .border = {
                        .color = active_border,
                        .width = { .top = 1, .left = 1, .right = 1 }
                    },
                    .cornerRadius = { .topLeft = 1, .topRight = 1 }
                ) {
                    CLAY_TEXT(CEL_Clay_Text(tab_buf, tab_len),
                        CLAY_TEXT_CONFIG({ .textColor = tab_fg,
                                          .userData = w_pack_text_attr(active_attr) }));
                }
            } else {
                /* Inactive tabs: 1 row, aligned to bottom */
                CEL_Clay(
                    .layout = {
                        .sizing = { .height = CLAY_SIZING_FIXED(1) },
                        .padding = { .left = 1, .right = 1 }
                    },
                    .backgroundColor = bar_bg
                ) {
                    CLAY_TEXT(CEL_Clay_Text(tab_buf, tab_len),
                        CLAY_TEXT_CONFIG({ .textColor = tab_fg,
                                          .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
                }
            }
        }
    }
}

void w_tab_content_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_TabContent* d = (const W_TabContent*)ecs_get_id(world, self, W_TabContent_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_TabContentStyle* s = d->style;

    CEL_Color text_fg = (s && s->fg.a > 0) ? s->fg : t->content_muted.color;
    CEL_TextAttr text_attr = t->content_muted.attr;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0)
            },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .childGap = 1
        }
    ) {
        if (d->text) {
            CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
                CLAY_TEXT_CONFIG({ .textColor = text_fg,
                                  .userData = w_pack_text_attr(text_attr) }));
        }
        if (d->hint) {
            CLAY_TEXT(CEL_Clay_Text(d->hint, (int)strlen(d->hint)),
                CLAY_TEXT_CONFIG({ .textColor = text_fg,
                                  .userData = w_pack_text_attr(text_attr) }));
        }
        CEL_Clay_Children();
    }
}

void w_status_bar_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_StatusBar* d = (const W_StatusBar*)ecs_get_id(world, self, W_StatusBar_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_StatusBarStyle* s = d->style;

    CEL_Color bar_bg = (s && s->bg.a > 0) ? s->bg : t->surface_alt.color;
    CEL_Color left_fg = (s && s->fg.a > 0) ? s->fg : t->content.color;
    CEL_Color right_fg = t->content_muted.color;
    CEL_TextAttr left_attr = t->content.attr;
    CEL_TextAttr right_attr = t->content_muted.attr;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = bar_bg
    ) {
        if (d->left) {
            CLAY_TEXT(CEL_Clay_Text(d->left, (int)strlen(d->left)),
                CLAY_TEXT_CONFIG({ .textColor = left_fg,
                                  .userData = w_pack_text_attr(left_attr) }));
        }
        /* Spacer pushes right text to far end */
        CEL_Clay(
            .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
        ) {}
        if (d->right) {
            CLAY_TEXT(CEL_Clay_Text(d->right, (int)strlen(d->right)),
                CLAY_TEXT_CONFIG({ .textColor = right_fg,
                                  .userData = w_pack_text_attr(right_attr) }));
        }
    }
}

/* ============================================================================
 * List Layouts
 * ============================================================================ */

void w_list_view_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_ListView* d = (const W_ListView*)ecs_get_id(world, self, W_ListView_ensure());
    (void)d;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ListViewStyle* s = (d ? d->style : NULL);

    /* Read W_Scrollable for scroll state (behavioral component) */
    const W_Scrollable* scr = (const W_Scrollable*)ecs_get_id(world, self, W_Scrollable_ensure());
    (void)scr; /* Available for future scroll offset inspection */

    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface.color;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0)
            }
        },
        .clip = {
            .vertical = true,
            .childOffset = Clay_GetScrollOffset()
        },
        .backgroundColor = bg_color
    ) {
        CEL_Clay_Children();
    }
}

void w_list_item_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_ListItem* d = (const W_ListItem*)ecs_get_id(world, self, W_ListItem_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ListItemStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_ensure());
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_ensure());
    bool selected = sel ? sel->selected : false;

    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_NONE,
        selected, false, disabled);

    CEL_Clay(
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 2 }
        },
        .backgroundColor = v.bg
    ) {
        if (selected) {
            CLAY_TEXT(CLAY_STRING("> "),
                CLAY_TEXT_CONFIG({ .textColor = v.fg,
                                  .userData = w_pack_text_attr(v.text_attr) }));
        }
        CLAY_TEXT(CEL_Clay_Text(d->label, (int)strlen(d->label)),
            CLAY_TEXT_CONFIG({ .textColor = v.fg,
                              .userData = w_pack_text_attr(v.text_attr) }));
    }
}
