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
 * Patterns:
 *   - CEL_Clay(...) { } for element containers
 *   - CLAY_TEXT(str, config) for text
 *   - CEL_Clay_Text(buf, len) for dynamic strings
 *   - CEL_Clay_Children() for child entity insertion
 *   - Widget_get_theme() for consistent theming
 */

#include <cels-widgets/widgets.h>
#include <cels-widgets/layouts.h>
#include <cels-widgets/theme.h>
#include <cels-clay/clay_layout.h>
#include <clay.h>
#include <flecs.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Theme Singleton
 * ============================================================================ */

static const Widget_Theme* s_active_theme = NULL;

const Widget_Theme* Widget_get_theme(void) {
    return s_active_theme ? s_active_theme : &Widget_THEME_DEFAULT;
}

void Widget_set_theme(const Widget_Theme* theme) {
    s_active_theme = theme;
}

/* ============================================================================
 * Helper: status color from theme
 * ============================================================================ */

static Clay_Color status_color(const Widget_Theme* t, int status) {
    switch (status) {
        case 1: return t->success;
        case 2: return t->warning;
        case 3: return t->error;
        default: return t->fg;
    }
}

/* ============================================================================
 * Text & Display Layouts
 * ============================================================================ */

void w_text_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Text* d = (const W_Text*)ecs_get_id(world, self, W_Text_ensure());
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();

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
            CLAY_TEXT_CONFIG({ .textColor = t->fg }));
    }
}

void w_hint_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Hint* d = (const W_Hint*)ecs_get_id(world, self, W_Hint_ensure());
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();

    CEL_Clay(
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .top = 1 }
        }
    ) {
        CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
            CLAY_TEXT_CONFIG({ .textColor = t->secondary }));
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

    Clay_Color bg = s ? Widget_resolve_color(s->bg, t->panel_bg) : t->panel_bg;
    Clay_Color border_color = s ? Widget_resolve_color(s->border_color, t->panel_border) : t->panel_border;

    CEL_Clay(
        .layout = {
            .sizing = { .width = w_sizing, .height = h_sizing },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
        },
        .border = {
            .color = border_color,
            .width = CLAY_BORDER_OUTSIDE(1)
        },
        .backgroundColor = bg,
        .cornerRadius = { .topLeft = 1, .topRight = 1 }
    ) {
        if (d->title) {
            CLAY_TEXT(CEL_Clay_Text(d->title, (int)strlen(d->title)),
                CLAY_TEXT_CONFIG({ .textColor = t->focus_border }));
        }
    }
}

void w_info_box_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_InfoBox* d = (const W_InfoBox*)ecs_get_id(world, self, W_InfoBox_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();

    if (d->border) {
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(3) },
                .padding = CLAY_PADDING_ALL(1),
                .childGap = 1
            },
            .border = {
                .color = t->panel_border,
                .width = CLAY_BORDER_OUTSIDE(1)
            },
            .backgroundColor = t->panel_bg,
            .cornerRadius = { .topLeft = 1, .topRight = 1 }
        ) {
            if (d->title) {
                CLAY_TEXT(CEL_Clay_Text(d->title, (int)strlen(d->title)),
                    CLAY_TEXT_CONFIG({ .textColor = t->focus_border }));
            }
            if (d->content) {
                CLAY_TEXT(CEL_Clay_Text(d->content, (int)strlen(d->content)),
                    CLAY_TEXT_CONFIG({ .textColor = t->fg }));
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
                    CLAY_TEXT_CONFIG({ .textColor = t->focus_border }));
            }
            if (d->content) {
                CLAY_TEXT(CEL_Clay_Text(d->content, (int)strlen(d->content)),
                    CLAY_TEXT_CONFIG({ .textColor = t->fg }));
            }
        }
    }
}

void w_badge_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Badge* d = (const W_Badge*)ecs_get_id(world, self, W_Badge_ensure());
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();

    Clay_Color badge_color = (d->r || d->g || d->b)
        ? (Clay_Color){ d->r, d->g, d->b, 255 }
        : t->badge_bg;

    CEL_Clay(
        .layout = {
            .sizing = { .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 }
        },
        .backgroundColor = badge_color
    ) {
        CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
            CLAY_TEXT_CONFIG({ .textColor = t->fg }));
    }
}

void w_text_area_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_TextArea* d = (const W_TextArea*)ecs_get_id(world, self, W_TextArea_ensure());
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();

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
                CLAY_TEXT_CONFIG({ .textColor = t->fg }));
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
                CLAY_TEXT_CONFIG({ .textColor = t->fg }));
        }
    }
}

/* ============================================================================
 * Interactive Layouts
 * ============================================================================ */

void w_button_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Button* d = (const W_Button*)ecs_get_id(world, self, W_Button_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ButtonStyle* s = d->style;

    /* Sizing: style override or defaults (GROW x FIXED(1))
     * Width uses Widget_resolve_width (divides by AR so CEL_FIXED(30) = 30 terminal cols) */
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

    /* Colors: style override or theme defaults */
    Clay_Color bg_normal   = s ? Widget_resolve_color(s->bg, t->button_bg) : t->button_bg;
    Clay_Color bg_selected = s ? Widget_resolve_color(s->bg_selected, t->button_selected_bg) : t->button_selected_bg;
    Clay_Color fg_normal   = s ? Widget_resolve_color(s->fg, t->button_fg) : t->button_fg;
    Clay_Color fg_selected = s ? Widget_resolve_color(s->fg_selected, t->focus_border) : t->focus_border;

    Clay_Color bg = d->selected ? bg_selected : bg_normal;
    Clay_Color text_color = d->selected ? fg_selected : fg_normal;

    /* Border: style override or default (on select) */
    Clay_Color border_color = s ? Widget_resolve_color(s->border_color, t->focus_border) : t->focus_border;
    Clay_BorderWidth border_w = {0};
    CEL_BorderMode border_mode = s ? s->border : CEL_BORDER_DEFAULT;
    switch (border_mode) {
        case CEL_BORDER_ALWAYS:
            border_w = (Clay_BorderWidth){1, 1, 1, 1, 0};
            break;
        case CEL_BORDER_ON_SELECT:
            if (d->selected) border_w = (Clay_BorderWidth){1, 1, 1, 1, 0};
            break;
        case CEL_BORDER_ON_FOCUS:
            if (d->focused) border_w = (Clay_BorderWidth){1, 1, 1, 1, 0};
            break;
        case CEL_BORDER_NONE:
            break;
        default: /* CEL_BORDER_DEFAULT -- border on select */
            if (d->selected) border_w = (Clay_BorderWidth){1, 1, 1, 1, 0};
            break;
    }

    CEL_Clay(
        .layout = {
            .sizing = { .width = w_axis, .height = h_axis },
            .padding = pad,
            .childAlignment = align
        },
        .backgroundColor = bg,
        .border = {
            .color = border_color,
            .width = border_w
        }
    ) {
        if (d->selected) {
            CLAY_TEXT(CLAY_STRING("> "),
                CLAY_TEXT_CONFIG({ .textColor = text_color }));
        }
        CLAY_TEXT(CEL_Clay_Text(d->label, (int)strlen(d->label)),
            CLAY_TEXT_CONFIG({ .textColor = text_color }));
    }
}

void w_slider_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Slider* d = (const W_Slider*)ecs_get_id(world, self, W_Slider_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();

    Clay_Color bg = d->selected ? t->button_selected_bg : t->button_bg;
    Clay_Color label_color = d->selected ? t->warning : t->fg;

    float range = (d->max > d->min) ? (d->max - d->min) : 1.0f;
    float norm = (d->value - d->min) / range;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

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
        .backgroundColor = bg
    ) {
        /* Label */
        char label_buf[32];
        int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
        CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
            CLAY_TEXT_CONFIG({ .textColor = label_color }));

        /* Bar */
        CLAY_TEXT(CEL_Clay_Text(bar_buf, (int)strlen(bar_buf)),
            CLAY_TEXT_CONFIG({ .textColor = t->focus_border }));
    }
}

void w_toggle_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Toggle* d = (const W_Toggle*)ecs_get_id(world, self, W_Toggle_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();

    Clay_Color bg = d->selected ? t->button_selected_bg : t->button_bg;
    Clay_Color label_color = d->selected ? t->warning : t->fg;

    const char* state_str = d->value ? "ON" : "OFF";
    Clay_Color state_color = d->value ? t->success : t->error;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childGap = 1
        },
        .backgroundColor = bg
    ) {
        char label_buf[32];
        int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
        CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
            CLAY_TEXT_CONFIG({ .textColor = label_color }));

        char toggle_buf[16];
        int toggle_len = snprintf(toggle_buf, sizeof(toggle_buf), "[%s]", state_str);
        CLAY_TEXT(CEL_Clay_Text(toggle_buf, toggle_len),
            CLAY_TEXT_CONFIG({ .textColor = state_color }));
    }
}

void w_cycle_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Cycle* d = (const W_Cycle*)ecs_get_id(world, self, W_Cycle_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();

    Clay_Color bg = d->selected ? t->button_selected_bg : t->button_bg;
    Clay_Color label_color = d->selected ? t->warning : t->fg;
    Clay_Color arrow_color = d->selected ? t->focus_border : t->secondary;

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childGap = 1
        },
        .backgroundColor = bg
    ) {
        char label_buf[32];
        int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
        CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
            CLAY_TEXT_CONFIG({ .textColor = label_color }));

        CLAY_TEXT(CLAY_STRING("[<]"),
            CLAY_TEXT_CONFIG({ .textColor = arrow_color }));

        const char* val = d->value ? d->value : "";
        char val_buf[24];
        int val_len = snprintf(val_buf, sizeof(val_buf), "%-15s", val);
        CLAY_TEXT(CEL_Clay_Text(val_buf, val_len),
            CLAY_TEXT_CONFIG({ .textColor = t->fg }));

        CLAY_TEXT(CLAY_STRING("[>]"),
            CLAY_TEXT_CONFIG({ .textColor = arrow_color }));
    }
}

/* ============================================================================
 * Progress & Metric Layouts
 * ============================================================================ */

void w_progress_bar_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_ProgressBar* d = (const W_ProgressBar*)ecs_get_id(world, self, W_ProgressBar_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();

    float v = d->value;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    Clay_Color fill_color = t->progress_fill.color;
    if (d->color_by_value) {
        if (v < 0.33f)      fill_color = t->error;
        else if (v < 0.66f) fill_color = t->warning;
        else                 fill_color = t->success;
    }

    int bar_width = 20;
    int filled = (int)(v * bar_width);
    char bar_buf[32];
    bar_buf[0] = '[';
    for (int j = 0; j < bar_width; j++) {
        bar_buf[j + 1] = (j < filled) ? '#' : ' ';
    }
    bar_buf[bar_width + 1] = ']';
    bar_buf[bar_width + 2] = '\0';

    char pct_buf[8];
    int pct_len = snprintf(pct_buf, sizeof(pct_buf), "%3d%%", (int)(v * 100));

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
                CLAY_TEXT_CONFIG({ .textColor = t->fg }));
        }
        CLAY_TEXT(CEL_Clay_Text(bar_buf, (int)strlen(bar_buf)),
            CLAY_TEXT_CONFIG({ .textColor = fill_color }));
        CLAY_TEXT(CEL_Clay_Text(pct_buf, pct_len),
            CLAY_TEXT_CONFIG({ .textColor = t->secondary }));
    }
}

void w_metric_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Metric* d = (const W_Metric*)ecs_get_id(world, self, W_Metric_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();

    Clay_Color val_color = status_color(t, d->status);

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
                CLAY_TEXT_CONFIG({ .textColor = t->secondary }));
        }
        if (d->value) {
            CLAY_TEXT(CEL_Clay_Text(d->value, (int)strlen(d->value)),
                CLAY_TEXT_CONFIG({ .textColor = val_color }));
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
    Clay_Color bg = s ? Widget_resolve_color(s->bg, t->panel_bg) : t->panel_bg;
    Clay_Color border_color = s ? Widget_resolve_color(s->border_color, t->panel_border) : t->panel_border;

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
        .backgroundColor = bg,
        .border = {
            .color = border_color,
            .width = border_w
        },
        .cornerRadius = { .topLeft = 1, .topRight = 1 }
    ) {
        if (d && d->title) {
            CLAY_TEXT(CEL_Clay_Text(d->title, (int)strlen(d->title)),
                CLAY_TEXT_CONFIG({ .textColor = t->focus_border }));
        }
        CEL_Clay_Children();
    }
}

void w_divider_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Divider* d = (const W_Divider*)ecs_get_id(world, self, W_Divider_ensure());
    const Widget_Theme* t = Widget_get_theme();

    bool vertical = d ? d->vertical : false;

    if (vertical) {
        CEL_Clay(
            .layout = {
                .sizing = { .width = CLAY_SIZING_FIXED(1), .height = CLAY_SIZING_GROW(0) }
            },
            .backgroundColor = t->divider.color
        ) {}
    } else {
        CEL_Clay(
            .layout = {
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) }
            },
            .backgroundColor = t->divider.color
        ) {}
    }
}

void w_table_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Table* d = (const W_Table*)ecs_get_id(world, self, W_Table_ensure());
    if (!d || d->row_count <= 0) return;
    const Widget_Theme* t = Widget_get_theme();

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
                    CLAY_TEXT_CONFIG({ .textColor = t->secondary }));
                CLAY_TEXT(CEL_Clay_Text(val, (int)strlen(val)),
                    CLAY_TEXT_CONFIG({ .textColor = t->fg }));
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

    const char* marker = d->selected ? "(*)" : "( )";
    Clay_Color text_color = d->selected ? t->warning : t->secondary;

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%s %s", marker, d->label);

    CEL_Clay(
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1 }
        }
    ) {
        CLAY_TEXT(CEL_Clay_Text(buf, len),
            CLAY_TEXT_CONFIG({ .textColor = text_color }));
    }
}

void w_radio_group_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_RadioGroup* d = (const W_RadioGroup*)ecs_get_id(world, self, W_RadioGroup_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();

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
            CLAY_TEXT_CONFIG({ .textColor = t->focus_border }));
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

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(2) },
            .childGap = 0,
            .childAlignment = { .y = CLAY_ALIGN_Y_BOTTOM }
        },
        .backgroundColor = t->status_bar_bg
    ) {
        for (int i = 0; i < d->count; i++) {
            const char* name = (d->labels && d->labels[i]) ? d->labels[i] : "?";
            bool active = (i == d->active);
            Clay_Color tab_fg = active ? t->focus_border : t->secondary;

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
                    .backgroundColor = t->tab_active_bg,
                    .border = {
                        .color = t->focus_border,
                        .width = { .top = 1, .left = 1, .right = 1 }
                    },
                    .cornerRadius = { .topLeft = 1, .topRight = 1 }
                ) {
                    CLAY_TEXT(CEL_Clay_Text(tab_buf, tab_len),
                        CLAY_TEXT_CONFIG({ .textColor = tab_fg }));
                }
            } else {
                /* Inactive tabs: 1 row, aligned to bottom */
                CEL_Clay(
                    .layout = {
                        .sizing = { .height = CLAY_SIZING_FIXED(1) },
                        .padding = { .left = 1, .right = 1 }
                    },
                    .backgroundColor = t->status_bar_bg
                ) {
                    CLAY_TEXT(CEL_Clay_Text(tab_buf, tab_len),
                        CLAY_TEXT_CONFIG({ .textColor = tab_fg }));
                }
            }
        }
    }
}

void w_tab_content_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_TabContent* d = (const W_TabContent*)ecs_get_id(world, self, W_TabContent_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();

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
                CLAY_TEXT_CONFIG({ .textColor = t->secondary }));
        }
        if (d->hint) {
            CLAY_TEXT(CEL_Clay_Text(d->hint, (int)strlen(d->hint)),
                CLAY_TEXT_CONFIG({ .textColor = t->secondary }));
        }
        CEL_Clay_Children();
    }
}

void w_status_bar_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_StatusBar* d = (const W_StatusBar*)ecs_get_id(world, self, W_StatusBar_ensure());
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = t->status_bar_bg
    ) {
        if (d->left) {
            CLAY_TEXT(CEL_Clay_Text(d->left, (int)strlen(d->left)),
                CLAY_TEXT_CONFIG({ .textColor = t->fg }));
        }
        /* Spacer pushes right text to far end */
        CEL_Clay(
            .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } }
        ) {}
        if (d->right) {
            CLAY_TEXT(CEL_Clay_Text(d->right, (int)strlen(d->right)),
                CLAY_TEXT_CONFIG({ .textColor = t->secondary }));
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
        .backgroundColor = t->bg
    ) {
        CEL_Clay_Children();
    }
}

void w_list_item_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_ListItem* d = (const W_ListItem*)ecs_get_id(world, self, W_ListItem_ensure());
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();

    Clay_Color bg = d->selected ? t->button_selected_bg : t->bg;
    Clay_Color text_color = d->selected ? t->warning : t->fg;

    CEL_Clay(
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 2 }
        },
        .backgroundColor = bg
    ) {
        if (d->selected) {
            CLAY_TEXT(CLAY_STRING("> "),
                CLAY_TEXT_CONFIG({ .textColor = text_color }));
        }
        CLAY_TEXT(CEL_Clay_Text(d->label, (int)strlen(d->label)),
            CLAY_TEXT_CONFIG({ .textColor = text_color }));
    }
}
