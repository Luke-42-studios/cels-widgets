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
#include <cels-widgets/input.h>
#include <cels-widgets/theme.h>
#include <cels-widgets/style.h>
#include <cels-clay/clay_layout.h>
#include <cels-clay/clay_render.h>
#include <clay.h>
#include <flecs.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Theme Singleton
 * ============================================================================ */

static const Widget_Theme* s_active_theme = NULL;
static bool s_theme_dirty = false;

/* Shared ring buffer for border decoration data (Panel, Canvas, InfoBox,
 * Popup, Modal, Window). Each bordered layout call allocates one slot,
 * valid for the current frame. 128 slots handles all bordered widgets.
 * Wraps safely since render happens after all layouts. */
#define W_BORDER_DECOR_MAX 128
static CelClayBorderDecor g_border_decors[W_BORDER_DECOR_MAX];
static int g_border_decor_idx = 0;

static CelClayBorderDecor* _alloc_border_decor(void) {
    CelClayBorderDecor* d = &g_border_decors[g_border_decor_idx % W_BORDER_DECOR_MAX];
    g_border_decor_idx++;
    return d;
}

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
    const W_Text* d = (const W_Text*)ecs_get_id(world, self, W_Text_id);
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
    const W_Hint* d = (const W_Hint*)ecs_get_id(world, self, W_Hint_id);
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
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
        }
    ) {
        CLAY_TEXT(CEL_Clay_Text(d->text, (int)strlen(d->text)),
            CLAY_TEXT_CONFIG({ .textColor = text_fg,
                              .userData = w_pack_text_attr(text_attr) }));
    }
}

void w_canvas_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Canvas* d = (const W_Canvas*)ecs_get_id(world, self, W_Canvas_id);
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

    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : (CEL_Color){0, 0, 0, 1};
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    CEL_Color title_fg = t->primary.color;
    CEL_TextAttr title_attr = t->primary.attr;

    CelClayBorderDecor* decor = _alloc_border_decor();
    *decor = (CelClayBorderDecor){
        .title = d->title,
        .border_color = bdr_color,
        .title_color = title_fg,
        .bg_color = bg_color,
        .border_style = 0,
        .title_text_attr = (uintptr_t)w_pack_text_attr(title_attr)
    };

    CEL_Clay(
        .layout = {
            .sizing = { .width = w_sizing, .height = h_sizing },
            .padding = { .left = 1, .right = 1, .top = 1, .bottom = 1 },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = bg_color,
        .userData = decor
    ) {
        /* Title rendered by renderer on the border line */
    }
}

void w_info_box_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_InfoBox* d = (const W_InfoBox*)ecs_get_id(world, self, W_InfoBox_id);
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_InfoBoxStyle* s = d->style;

    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : (CEL_Color){0, 0, 0, 1};
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    CEL_Color title_fg = t->primary.color;
    CEL_TextAttr title_attr = t->primary.attr;
    CEL_Color content_fg = (s && s->fg.a > 0) ? s->fg : t->content.color;
    CEL_TextAttr content_attr = (s && (s->text_attr.bold || s->text_attr.dim
        || s->text_attr.underline || s->text_attr.reverse || s->text_attr.italic))
        ? s->text_attr : t->content.attr;

    if (d->border) {
        CelClayBorderDecor* decor = _alloc_border_decor();
        *decor = (CelClayBorderDecor){
            .title = d->title,
            .border_color = bdr_color,
            .title_color = title_fg,
            .bg_color = bg_color,
            .border_style = 0,
            .title_text_attr = (uintptr_t)w_pack_text_attr(title_attr)
        };

        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(3) },
                .padding = { .left = 1, .right = 1, .top = 1, .bottom = 1 },
                .childGap = 1
            },
            .backgroundColor = bg_color,
            .userData = decor
        ) {
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
    const W_Badge* d = (const W_Badge*)ecs_get_id(world, self, W_Badge_id);
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
    const W_TextArea* d = (const W_TextArea*)ecs_get_id(world, self, W_TextArea_id);
    if (!d || !d->text) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_TextAreaStyle* s = d->style;

    /* Read W_Scrollable for scroll state (behavioral component, preparatory) */
    const W_Scrollable* scr = (const W_Scrollable*)ecs_get_id(world, self, W_Scrollable_id);
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
        bool needs_clip = (d->max_height > 0);
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = w_sizing, .height = h_sizing },
                .padding = CLAY_PADDING_ALL(1)
            },
            .clip = {
                .vertical = needs_clip,
                .childOffset = {0}
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
    const W_Button* d = (const W_Button*)ecs_get_id(world, self, W_Button_id);
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ButtonStyle* s = d->style;

    /* Read W_InteractState if available (set by composition macro) */
    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_id);
    bool disabled = ist ? ist->disabled : false;
    bool focused = ist ? ist->focused : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_id);
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
    const W_Slider* d = (const W_Slider*)ecs_get_id(world, self, W_Slider_id);
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_SliderStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_id);
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_id);
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
    const W_RangeValueF* rv = (const W_RangeValueF*)ecs_get_id(world, self, W_RangeValueF_id);
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
    const W_Toggle* d = (const W_Toggle*)ecs_get_id(world, self, W_Toggle_id);
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ToggleStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_id);
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_id);
    bool selected = sel ? sel->selected : false;

    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_NONE,
        selected, false, disabled);

    /* ON/OFF colors from style or theme */
    CEL_Color on_color = (s && s->on_color.a > 0) ? s->on_color : t->status_success.color;
    CEL_Color off_color = (s && s->off_color.a > 0) ? s->off_color : t->status_error.color;

    /* Active value highlighted, inactive muted. Reverse when selected. */
    CEL_Color on_fg  = d->value ? on_color : t->content_muted.color;
    CEL_Color off_fg = d->value ? t->content_muted.color : off_color;
    CEL_TextAttr on_attr  = { .reverse = (selected && d->value) };
    CEL_TextAttr off_attr = { .reverse = (selected && !d->value) };

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childGap = 1
        }
    ) {
        char label_buf[32];
        int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
        CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
            CLAY_TEXT_CONFIG({ .textColor = v.fg,
                              .userData = w_pack_text_attr(v.text_attr) }));

        CLAY_TEXT(CLAY_STRING("[ON]"),
            CLAY_TEXT_CONFIG({ .textColor = on_fg,
                              .userData = w_pack_text_attr(on_attr) }));
        CLAY_TEXT(CLAY_STRING("[OFF]"),
            CLAY_TEXT_CONFIG({ .textColor = off_fg,
                              .userData = w_pack_text_attr(off_attr) }));
    }
}

void w_cycle_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Cycle* d = (const W_Cycle*)ecs_get_id(world, self, W_Cycle_id);
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_CycleStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_id);
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_id);
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
    /* Reverse arrows when selected to highlight the interactive controls */
    CEL_TextAttr arrow_attr = { .reverse = selected };

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
            .padding = { .left = 1, .right = 1 },
            .childGap = 1
        }
    ) {
        char label_buf[32];
        int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", d->label);
        CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
            CLAY_TEXT_CONFIG({ .textColor = v.fg,
                              .userData = w_pack_text_attr(v.text_attr) }));

        CLAY_TEXT(CLAY_STRING("[<]"),
            CLAY_TEXT_CONFIG({ .textColor = arrow_color,
                              .userData = w_pack_text_attr(arrow_attr) }));

        const char* val = d->value ? d->value : "";
        char val_buf[24];
        int val_len = snprintf(val_buf, sizeof(val_buf), "%-15s", val);
        CLAY_TEXT(CEL_Clay_Text(val_buf, val_len),
            CLAY_TEXT_CONFIG({ .textColor = t->content.color,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));

        CLAY_TEXT(CLAY_STRING("[>]"),
            CLAY_TEXT_CONFIG({ .textColor = arrow_color,
                              .userData = w_pack_text_attr(arrow_attr) }));
    }
}

/* ============================================================================
 * Progress & Metric Layouts
 * ============================================================================ */

void w_progress_bar_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_ProgressBar* d = (const W_ProgressBar*)ecs_get_id(world, self, W_ProgressBar_id);
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ProgressBarStyle* s = d->style;

    /* Read W_RangeValueF for progress value (behavioral component) */
    const W_RangeValueF* rv = (const W_RangeValueF*)ecs_get_id(world, self, W_RangeValueF_id);
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
    const W_Metric* d = (const W_Metric*)ecs_get_id(world, self, W_Metric_id);
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
    const W_Panel* d = (const W_Panel*)ecs_get_id(world, self, W_Panel_id);
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

    /* Padding: horizontal=1 (2 cells, border overlays outermost → 1 cell gap),
     *          vertical=2 (border on row 0/last, 1 row gap to content).
     * Style override replaces entirely. */
    Clay_Padding pad = { .left = 1, .right = 1, .top = 2, .bottom = 2 };
    if (s && (s->padding.left || s->padding.right || s->padding.top || s->padding.bottom)) {
        pad = (Clay_Padding){ .left = s->padding.left, .right = s->padding.right,
                              .top = s->padding.top, .bottom = s->padding.bottom };
    }

    /* Colors: style override or transparent (no fill by default) */
    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : (CEL_Color){0, 0, 0, 1};
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    CEL_Color title_fg = t->primary.color;
    CEL_TextAttr title_attr = t->primary.attr;

    /* Border mode: DEFAULT/ALWAYS show border, NONE hides it */
    CEL_BorderMode border_mode = s ? s->border : CEL_BORDER_DEFAULT;

    /* Build border decoration for renderer (bypasses Clay's uint16_t border
     * which AR-scales to 2+ cells wide). The renderer draws 1-cell-wide
     * box-drawing characters directly via tui_draw_border. */
    CelClayBorderDecor* decor = NULL;
    if (border_mode != CEL_BORDER_NONE) {
        decor = _alloc_border_decor();
        *decor = (CelClayBorderDecor){
            .title = (d && d->title) ? d->title : NULL,
            .border_color = bdr_color,
            .title_color = title_fg,
            .bg_color = bg_color,
            .border_style = 0, /* rounded (default for Panels) */
            .title_text_attr = (uintptr_t)w_pack_text_attr(title_attr)
        };
    }

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = w_axis, .height = h_axis },
            .padding = pad,
            .childGap = 1
        },
        .backgroundColor = bg_color,
        .userData = decor
    ) {
        /* Title rendered by renderer on the border line (not as Clay content) */
        CEL_Clay_Children();
    }
}

void w_divider_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Divider* d = (const W_Divider*)ecs_get_id(world, self, W_Divider_id);
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
    const W_Table* d = (const W_Table*)ecs_get_id(world, self, W_Table_id);
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
 * Structural Container Layouts
 * ============================================================================ */

void w_collapsible_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Collapsible* d = (const W_Collapsible*)ecs_get_id(world, self, W_Collapsible_id);
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_CollapsibleStyle* s = d->style;

    /* Read W_InteractState and W_Selectable for focus/selection visual feedback */
    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_id);
    bool disabled = ist ? ist->disabled : false;
    bool focused = ist ? ist->focused : false;

    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_id);
    bool selected = sel ? sel->selected : false;

    /* Resolve title row visual from theme + style + state */
    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_NONE,
        selected, focused, disabled);

    /* Indicator and title colors from style or theme */
    CEL_Color indicator_fg = (s && s->indicator_color.a > 0) ? s->indicator_color : t->primary.color;
    CEL_Color title_fg = (s && s->title_color.a > 0) ? s->title_color : t->content_title.color;

    /* Indentation: indent * 2 cells */
    int left_pad = d->indent * 2;

    /* Unicode triangle indicators (UTF-8 encoded) */
    const char* indicator = d->collapsed
        ? "\xe2\x96\xb6 "   /* right-pointing triangle (collapsed) */
        : "\xe2\x96\xbc ";  /* down-pointing triangle (expanded) */

    /* Outer container: TOP_TO_BOTTOM, GROW width, FIT height */
    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }
        }
    ) {
        /* Title row: LEFT_TO_RIGHT, GROW width, FIXED(1) height */
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) },
                .padding = { .left = (uint16_t)left_pad },
                .childGap = 0
            },
            .backgroundColor = v.bg
        ) {
            /* Indicator */
            CLAY_TEXT(CEL_Clay_Text(indicator, (int)strlen(indicator)),
                CLAY_TEXT_CONFIG({ .textColor = indicator_fg,
                                  .userData = w_pack_text_attr((CEL_TextAttr){0}) }));

            /* Title text */
            if (d->title) {
                CLAY_TEXT(CEL_Clay_Text(d->title, (int)strlen(d->title)),
                    CLAY_TEXT_CONFIG({ .textColor = title_fg,
                                      .userData = w_pack_text_attr(v.text_attr) }));
            }
        }

        /* Content section: only emit children when expanded */
        if (!d->collapsed) {
            CEL_Clay(
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                    .childGap = 0
                }
            ) {
                CEL_Clay_Children();
            }
        }
    }
}

/* ============================================================================
 * Radio Layouts
 * ============================================================================ */

void w_radio_button_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_RadioButton* d = (const W_RadioButton*)ecs_get_id(world, self, W_RadioButton_id);
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_RadioButtonStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_id);
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_id);
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
    const W_RadioGroup* d = (const W_RadioGroup*)ecs_get_id(world, self, W_RadioGroup_id);
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
    const W_TabBar* d = (const W_TabBar*)ecs_get_id(world, self, W_TabBar_id);
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_TabBarStyle* s = d->style;

    bool powerline = (s && s->powerline);

    CEL_Color bar_bg = (s && s->bg.a > 0) ? s->bg : t->surface_alt.color;
    CEL_Color active_fg = t->primary_content.color;
    CEL_TextAttr active_attr = t->primary.attr;
    CEL_Color inactive_fg = t->content_muted.color;
    CEL_Color active_tab_bg = (s && s->active_bg.a > 0) ? s->active_bg : t->interactive_active.color;
    CEL_Color inactive_tab_bg = t->surface_alt.color;

    if (powerline) {
        /* ---- Powerline-styled tab rendering ---- */
        /* Select separator glyph (same set as Widget_Powerline) */
        const char* sep = Widget_powerline_glyphs_enabled()
            ? "\xee\x82\xb0"   /* U+E0B0 Nerd Font arrow */
            : ">";              /* ASCII fallback */
        int sep_len = (int)strlen(sep);

        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) }
            }
        ) {
            for (int i = 0; i < d->count; i++) {
                const char* name = (d->labels && d->labels[i]) ? d->labels[i] : "?";
                bool active = (i == d->active);

                CEL_Color tab_bg = active ? active_tab_bg : inactive_tab_bg;
                CEL_Color tab_fg = active ? active_fg : inactive_fg;
                CEL_TextAttr tab_attr = active ? active_attr : (CEL_TextAttr){0};

                char tab_buf[32];
                int tab_len = snprintf(tab_buf, sizeof(tab_buf), " %s ", name);

                /* Tab segment */
                CEL_Clay(
                    .layout = {
                        .sizing = { .height = CLAY_SIZING_FIXED(1) }
                    },
                    .backgroundColor = tab_bg
                ) {
                    CLAY_TEXT(CEL_Clay_Text(tab_buf, tab_len),
                        CLAY_TEXT_CONFIG({ .textColor = tab_fg,
                                          .userData = w_pack_text_attr(tab_attr) }));
                }

                /* Separator between tabs */
                if (i < d->count - 1) {
                    CEL_Color sep_fg_c = tab_bg;  /* Arrow tip = current tab color */
                    bool next_active = ((i + 1) == d->active);
                    CEL_Color sep_bg_c = next_active ? active_tab_bg : inactive_tab_bg;

                    CEL_Clay(
                        .layout = {
                            .sizing = { .height = CLAY_SIZING_FIXED(1) }
                        },
                        .backgroundColor = sep_bg_c
                    ) {
                        CLAY_TEXT(CEL_Clay_Text(sep, sep_len),
                            CLAY_TEXT_CONFIG({ .textColor = sep_fg_c,
                                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
                    }
                }
            }
        }
    } else {
        /* ---- Standard tab rendering (unchanged) ---- */
        CEL_Color active_border = t->primary.color;
        CEL_Color std_active_tab_bg = (s && s->active_bg.a > 0) ? s->active_bg : t->surface_raised.color;

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
                CEL_Color tab_fg = active ? t->primary.color : inactive_fg;

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
                        .backgroundColor = std_active_tab_bg,
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
}

void w_tab_content_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_TabContent* d = (const W_TabContent*)ecs_get_id(world, self, W_TabContent_id);
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
    const W_StatusBar* d = (const W_StatusBar*)ecs_get_id(world, self, W_StatusBar_id);
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
    const W_ListView* d = (const W_ListView*)ecs_get_id(world, self, W_ListView_id);
    (void)d;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ListViewStyle* s = (d ? d->style : NULL);

    /* Read W_Scrollable for scroll state (behavioral component) */
    const W_Scrollable* scr = (const W_Scrollable*)ecs_get_id(world, self, W_Scrollable_id);
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
    const W_ListItem* d = (const W_ListItem*)ecs_get_id(world, self, W_ListItem_id);
    if (!d || !d->label) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ListItemStyle* s = d->style;

    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(world, self, W_InteractState_id);
    bool disabled = ist ? ist->disabled : false;

    /* Read W_Selectable for selection state (behavioral component) */
    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(world, self, W_Selectable_id);
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

/* ============================================================================
 * Navigation Group Layout
 * ============================================================================ */

void w_navigation_group_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_NavigationScope* scope = (const W_NavigationScope*)ecs_get_id(
        world, self, W_NavigationScope_id);
    int direction = scope ? scope->direction : 0;

    CEL_Clay(
        .layout = {
            .layoutDirection = direction == 0
                ? CLAY_TOP_TO_BOTTOM : CLAY_LEFT_TO_RIGHT,
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIT(0)
            },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER },
            .childGap = 1
        }
    ) {
        CEL_Clay_Children();
    }
}

/* ============================================================================
 * Text Input Layout
 * ============================================================================ */

void w_text_input_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_TextInput* d = (const W_TextInput*)ecs_get_id(world, self, W_TextInput_id);
    if (!d) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_TextInputStyle* s = d->style;

    /* Read persistent buffer state */
    const W_TextInputBuffer* buf = (const W_TextInputBuffer*)ecs_get_id(
        world, self, W_TextInputBuffer_id);

    /* Read interaction state */
    const W_InteractState* ist = (const W_InteractState*)ecs_get_id(
        world, self, W_InteractState_id);
    bool disabled = ist ? ist->disabled : false;
    bool focused = ist ? ist->focused : false;

    const W_Selectable* sel = (const W_Selectable*)ecs_get_id(
        world, self, W_Selectable_id);
    bool selected = sel ? sel->selected : false;

    /* Resolve visual state */
    W_ResolvedVisual v = w_resolve_visual(t,
        s ? s->bg : CEL_COLOR_NONE,
        s ? s->fg : CEL_COLOR_NONE,
        s ? s->text_attr : (CEL_TextAttr){0},
        s ? s->border_color : CEL_COLOR_NONE,
        s ? s->border : CEL_BORDER_DEFAULT,
        CEL_BORDER_ON_FOCUS,
        selected, focused, disabled);

    bool is_active = (selected && focused);

    /* Build display text */
    static char display_buf[768]; /* 256 * 3 for password bullets */
    int display_len = 0;
    int cursor_char = buf ? buf->cursor_pos : 0;
    int text_len = buf ? buf->length : 0;
    int text_byte_len = buf ? buf->byte_length : 0;
    bool is_empty = (!buf || !buf->initialized || text_len == 0);
    bool show_placeholder = (is_empty && !is_active && d->placeholder);

    if (show_placeholder) {
        /* Placeholder text */
        int plen = (int)strlen(d->placeholder);
        if (plen > (int)sizeof(display_buf) - 1) plen = (int)sizeof(display_buf) - 1;
        memcpy(display_buf, d->placeholder, (size_t)plen);
        display_buf[plen] = '\0';
        display_len = plen;
    } else if (d->password && buf && buf->initialized && text_len > 0) {
        /* Password mode: bullet dots U+2022 (\xe2\x80\xa2) per character */
        int pos = 0;
        for (int i = 0; i < text_len && pos + 3 < (int)sizeof(display_buf); i++) {
            display_buf[pos++] = '\xe2';
            display_buf[pos++] = '\x80';
            display_buf[pos++] = '\xa2';
        }
        display_buf[pos] = '\0';
        display_len = pos;
    } else if (buf && buf->initialized && text_byte_len > 0) {
        /* Normal text */
        int cplen = text_byte_len;
        if (cplen > (int)sizeof(display_buf) - 1) cplen = (int)sizeof(display_buf) - 1;
        memcpy(display_buf, buf->buffer, (size_t)cplen);
        display_buf[cplen] = '\0';
        display_len = cplen;
    } else {
        display_buf[0] = '\0';
        display_len = 0;
    }

    /* Colors */
    CEL_Color text_fg = v.fg;
    CEL_Color placeholder_fg = (s && s->placeholder_color.a > 0)
        ? s->placeholder_color : t->content_muted.color;
    CEL_Color cursor_fg = (s && s->cursor_color.a > 0)
        ? s->cursor_color : t->primary_content.color;

    /* Determine scroll offset for single-line display.
     * Default visible width estimate: 30 chars if unknown. */
    int visible_width = 30;
    int scroll_x = buf ? buf->scroll_x : 0;

    /* Adjust scroll to keep cursor visible */
    if (cursor_char < scroll_x) {
        scroll_x = cursor_char;
    }
    if (cursor_char >= scroll_x + visible_width) {
        scroll_x = cursor_char - visible_width + 1;
    }
    if (scroll_x < 0) scroll_x = 0;

    /* Border decoration when focused */
    CelClayBorderDecor* decor = NULL;
    if (v.show_border) {
        decor = _alloc_border_decor();
        *decor = (CelClayBorderDecor){
            .title = NULL,
            .border_color = v.border_color,
            .title_color = v.border_color,
            .bg_color = v.bg,
            .border_style = 0,
            .title_text_attr = 0
        };
    }

    /* Outer container */
    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(v.show_border ? 3 : 1) },
            .padding = v.show_border
                ? (Clay_Padding){ .left = 1, .right = 1, .top = 1, .bottom = 1 }
                : (Clay_Padding){ .left = 1, .right = 1 }
        },
        .backgroundColor = v.bg,
        .userData = decor
    ) {
        if (show_placeholder) {
            /* Placeholder: dim text, no cursor */
            CLAY_TEXT(CEL_Clay_Text(display_buf, display_len),
                CLAY_TEXT_CONFIG({ .textColor = placeholder_fg,
                                  .userData = w_pack_text_attr((CEL_TextAttr){ .dim = true }) }));
        } else if (is_active && buf && buf->initialized) {
            /* Active input: split text around cursor for block cursor rendering */

            /* For password mode, cursor position maps to 3-byte bullet offsets */
            int char_to_byte_before;
            int cursor_char_bytes;
            int after_byte_start;
            int after_byte_len;

            if (d->password) {
                char_to_byte_before = cursor_char * 3;
                cursor_char_bytes = (cursor_char < text_len) ? 3 : 0;
                after_byte_start = char_to_byte_before + cursor_char_bytes;
                after_byte_len = display_len - after_byte_start;
            } else {
                /* ASCII-only for now: 1 byte per character */
                char_to_byte_before = cursor_char;
                cursor_char_bytes = (cursor_char < text_len) ? 1 : 0;
                after_byte_start = char_to_byte_before + cursor_char_bytes;
                after_byte_len = display_len - after_byte_start;
            }

            /* Text before cursor */
            if (char_to_byte_before > 0) {
                CLAY_TEXT(CEL_Clay_Text(display_buf, char_to_byte_before),
                    CLAY_TEXT_CONFIG({ .textColor = text_fg,
                                      .userData = w_pack_text_attr(v.text_attr) }));
            }

            /* Cursor character (reverse video for block cursor) */
            if (cursor_char < text_len) {
                /* Character at cursor position */
                CLAY_TEXT(CEL_Clay_Text(display_buf + char_to_byte_before, cursor_char_bytes),
                    CLAY_TEXT_CONFIG({ .textColor = cursor_fg,
                                      .userData = w_pack_text_attr((CEL_TextAttr){ .reverse = true }) }));
            } else {
                /* Cursor at end of text: render a space with reverse */
                CLAY_TEXT(CLAY_STRING(" "),
                    CLAY_TEXT_CONFIG({ .textColor = cursor_fg,
                                      .userData = w_pack_text_attr((CEL_TextAttr){ .reverse = true }) }));
            }

            /* Text after cursor */
            if (after_byte_len > 0) {
                CLAY_TEXT(CEL_Clay_Text(display_buf + after_byte_start, after_byte_len),
                    CLAY_TEXT_CONFIG({ .textColor = text_fg,
                                      .userData = w_pack_text_attr(v.text_attr) }));
            }
        } else {
            /* Inactive with text: show normally */
            if (display_len > 0) {
                CLAY_TEXT(CEL_Clay_Text(display_buf, display_len),
                    CLAY_TEXT_CONFIG({ .textColor = text_fg,
                                      .userData = w_pack_text_attr(v.text_attr) }));
            }
        }
    }
}

/* ============================================================================
 * Overlay Layouts
 * ============================================================================ */

void w_popup_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Popup* d = (const W_Popup*)ecs_get_id(world, self, W_Popup_id);
    if (!d || !d->visible) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_PopupStyle* s = d->style;

    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface_raised.color;
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    CEL_Color title_fg = (s && s->title_color.a > 0) ? s->title_color : t->content_title.color;
    CEL_TextAttr title_attr = t->content_title.attr;

    int w = (d->width > 0) ? d->width : 40;
    float w_px = (float)w / CEL_CELL_ASPECT_RATIO;

    Clay_SizingAxis h_axis = (d->height > 0)
        ? CLAY_SIZING_FIXED((float)d->height)
        : CLAY_SIZING_FIT(0);

    /* Backdrop: full-screen dim overlay behind popup */
    if (d->backdrop) {
        CEL_Color backdrop = (s && s->backdrop_color.a > 0)
            ? s->backdrop_color : (CEL_Color){0, 0, 0, 200};
        CEL_Clay(
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0)
                }
            },
            .backgroundColor = backdrop,
            .floating = {
                .attachTo = CLAY_ATTACH_TO_ROOT,
                .attachPoints = {
                    .element = CLAY_ATTACH_POINT_LEFT_TOP,
                    .parent = CLAY_ATTACH_POINT_LEFT_TOP
                },
                .zIndex = 99,
                .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH
            }
        ) {}
    }

    /* Popup container: centered floating element */
    CelClayBorderDecor* decor = _alloc_border_decor();
    *decor = (CelClayBorderDecor){
        .title = d->title,
        .border_color = bdr_color,
        .title_color = title_fg,
        .bg_color = bg_color,
        .border_style = 0,
        .title_text_attr = (uintptr_t)w_pack_text_attr(title_attr)
    };

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_FIXED(w_px), .height = h_axis },
            .padding = { .left = 1, .right = 1, .top = 2, .bottom = 2 },
            .childGap = 1
        },
        .backgroundColor = bg_color,
        .userData = decor,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                .parent = CLAY_ATTACH_POINT_CENTER_CENTER
            },
            .zIndex = 100,
            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH
        }
    ) {
        CEL_Clay_Children();
    }
}

void w_modal_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Modal* d = (const W_Modal*)ecs_get_id(world, self, W_Modal_id);
    if (!d || !d->visible) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ModalStyle* s = d->style;

    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface_raised.color;
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border_focused.color;
    CEL_Color title_fg = (s && s->title_color.a > 0) ? s->title_color : t->content_title.color;
    CEL_TextAttr title_attr = t->content_title.attr;

    int w = (d->width > 0) ? d->width : 50;
    float w_px = (float)w / CEL_CELL_ASPECT_RATIO;

    Clay_SizingAxis h_axis = (d->height > 0)
        ? CLAY_SIZING_FIXED((float)d->height)
        : CLAY_SIZING_FIT(0);

    /* Backdrop: always shown for modals (dimming is inherent to modal pattern) */
    CEL_Clay(
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0)
            }
        },
        .backgroundColor = (CEL_Color){0, 0, 0, 200},
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_LEFT_TOP,
                .parent = CLAY_ATTACH_POINT_LEFT_TOP
            },
            .zIndex = 199,
            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH
        }
    ) {}

    /* Modal container: centered floating element at higher z-band than popup */
    CelClayBorderDecor* decor = _alloc_border_decor();
    *decor = (CelClayBorderDecor){
        .title = d->title,
        .border_color = bdr_color,
        .title_color = title_fg,
        .bg_color = bg_color,
        .border_style = 0,
        .title_text_attr = (uintptr_t)w_pack_text_attr(title_attr)
    };

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_FIXED(w_px), .height = h_axis },
            .padding = { .left = 1, .right = 1, .top = 2, .bottom = 2 },
            .childGap = 1
        },
        .backgroundColor = bg_color,
        .userData = decor,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                .parent = CLAY_ATTACH_POINT_CENTER_CENTER
            },
            .zIndex = 200,
            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH
        }
    ) {
        CEL_Clay_Children();
    }
}

void w_window_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Window* d = (const W_Window*)ecs_get_id(world, self, W_Window_id);
    if (!d || !d->visible) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_WindowStyle* s = d->style;

    /* Resolve colors from theme + style overrides */
    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface_raised.color;
    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    /* Move mode: override border color to primary for visual feedback */
    const W_Draggable* drag = (const W_Draggable*)ecs_get_id(world, self, W_Draggable_id);
    if (drag && drag->moving) {
        bdr_color = t->primary.color;
    }
    CEL_Color title_fg = (s && s->title_color.a > 0) ? s->title_color : t->content_title.color;
    CEL_Color close_fg = (s && s->close_color.a > 0) ? s->close_color : t->status_error.color;
    CEL_TextAttr title_attr = t->content_title.attr;

    int w = (d->width > 0) ? d->width : 40;
    float w_px = (float)w / CEL_CELL_ASPECT_RATIO;

    Clay_SizingAxis h_axis = (d->height > 0)
        ? CLAY_SIZING_FIXED((float)d->height)
        : CLAY_SIZING_FIT(0);

    /* Position: center if x==0 && y==0, otherwise offset from top-left */
    Clay_FloatingAttachPoints attach;
    Clay_Vector2 offset;
    if (d->x == 0 && d->y == 0) {
        attach = (Clay_FloatingAttachPoints){
            .element = CLAY_ATTACH_POINT_CENTER_CENTER,
            .parent = CLAY_ATTACH_POINT_CENTER_CENTER
        };
        offset = (Clay_Vector2){ 0, 0 };
    } else {
        attach = (Clay_FloatingAttachPoints){
            .element = CLAY_ATTACH_POINT_LEFT_TOP,
            .parent = CLAY_ATTACH_POINT_LEFT_TOP
        };
        offset = (Clay_Vector2){
            .x = (float)d->x / CEL_CELL_ASPECT_RATIO,
            .y = (float)d->y
        };
    }

    int z = 150 + d->z_order;

    /* Window container: floating element with renderer-drawn border */
    CelClayBorderDecor* decor = _alloc_border_decor();
    *decor = (CelClayBorderDecor){
        .title = d->title,
        .right_text = d->on_close ? "[X]" : NULL,
        .border_color = bdr_color,
        .title_color = title_fg,
        .right_color = close_fg,
        .bg_color = bg_color,
        .border_style = 1, /* single (sharper look for windows) */
        .title_text_attr = (uintptr_t)w_pack_text_attr(title_attr)
    };

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_FIXED(w_px), .height = h_axis },
            .padding = { .left = 1, .right = 1, .top = 2, .bottom = 2 },
            .childGap = 1
        },
        .backgroundColor = bg_color,
        .userData = decor,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = attach,
            .offset = offset,
            .zIndex = (int16_t)z,
            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH
        }
    ) {
        CEL_Clay_Children();
    }
}

void w_toast_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Toast* d = (const W_Toast*)ecs_get_id(world, self, W_Toast_id);
    if (!d || d->dismissed) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ToastStyle* s = d->style;

    /* Severity-based background color (style override > defaults) */
    CEL_Color bg_color;
    const char* indicator;
    switch (d->severity) {
        case 1:  /* success */
            bg_color = (s && s->success_color.a > 0) ? s->success_color
                       : (CEL_Color){60, 180, 80, 255};
            indicator = "[+] ";
            break;
        case 2:  /* warning */
            bg_color = (s && s->warning_color.a > 0) ? s->warning_color
                       : (CEL_Color){220, 180, 40, 255};
            indicator = "[!] ";
            break;
        case 3:  /* error */
            bg_color = (s && s->error_color.a > 0) ? s->error_color
                       : (CEL_Color){200, 60, 60, 255};
            indicator = "[x] ";
            break;
        default: /* info */
            bg_color = (s && s->info_color.a > 0) ? s->info_color
                       : t->primary.color;
            indicator = "[i] ";
            break;
    }
    /* Style-level bg/fg overrides on top of severity */
    if (s && s->bg.a > 0) bg_color = s->bg;
    CEL_Color text_fg = (s && s->fg.a > 0) ? s->fg
        : (CEL_Color){255, 255, 255, 255};

    /* Toast width: based on message length (indicator + message + padding), min 20, max 50 */
    int msg_len = d->message ? (int)strlen(d->message) : 0;
    int content_len = 4 + msg_len + 2; /* "[x] " + message + padding */
    if (content_len < 20) content_len = 20;
    if (content_len > 50) content_len = 50;
    float toast_width = (float)content_len / CEL_CELL_ASPECT_RATIO;

    /* Position-based attach points and offsets */
    Clay_FloatingAttachPoints attach;
    Clay_Vector2 offset;
    switch (d->position) {
        case 1:  /* bottom-center */
            attach = (Clay_FloatingAttachPoints){
                .element = CLAY_ATTACH_POINT_CENTER_BOTTOM,
                .parent = CLAY_ATTACH_POINT_CENTER_BOTTOM
            };
            offset = (Clay_Vector2){ .x = 0, .y = -1 };
            break;
        case 2:  /* top-right */
            attach = (Clay_FloatingAttachPoints){
                .element = CLAY_ATTACH_POINT_RIGHT_TOP,
                .parent = CLAY_ATTACH_POINT_RIGHT_TOP
            };
            offset = (Clay_Vector2){ .x = -2, .y = 1 };
            break;
        case 3:  /* top-center */
            attach = (Clay_FloatingAttachPoints){
                .element = CLAY_ATTACH_POINT_CENTER_TOP,
                .parent = CLAY_ATTACH_POINT_CENTER_TOP
            };
            offset = (Clay_Vector2){ .x = 0, .y = 1 };
            break;
        default: /* 0 = bottom-right */
            attach = (Clay_FloatingAttachPoints){
                .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
                .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM
            };
            offset = (Clay_Vector2){ .x = -2, .y = -1 };
            break;
    }

    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
                .width = CLAY_SIZING_FIXED(toast_width),
                .height = CLAY_SIZING_FIT(0)
            },
            .padding = { .left = 1, .right = 1 }
        },
        .backgroundColor = bg_color,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = attach,
            .offset = offset,
            .zIndex = 300,
            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH
        }
    ) {
        /* Severity indicator prefix */
        CLAY_TEXT(CEL_Clay_Text(indicator, (int)strlen(indicator)),
            CLAY_TEXT_CONFIG({ .textColor = text_fg,
                              .userData = w_pack_text_attr((CEL_TextAttr){ .bold = true }) }));

        /* Message text */
        if (d->message) {
            CLAY_TEXT(CEL_Clay_Text(d->message, msg_len),
                CLAY_TEXT_CONFIG({ .textColor = text_fg,
                                  .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
        }
    }
}

/* ============================================================================
 * Split Pane Layout
 * ============================================================================ */

void w_split_pane_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_SplitPane* d = (const W_SplitPane*)ecs_get_id(world, self, W_SplitPane_id);
    const Widget_Theme* t = Widget_get_theme();
    const Widget_SplitStyle* s = (d ? d->style : NULL);

    float ratio = (d && d->ratio > 0.0f) ? d->ratio : 0.5f;
    int direction = d ? d->direction : 0;

    /* Divider color: style override or theme */
    CEL_Color div_color = (s && s->divider_color.a > 0) ? s->divider_color : t->divider.color;

    /* Outer container: direction-appropriate flex */
    Clay_LayoutDirection layout_dir = (direction == 0)
        ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM;

    CEL_Clay(
        .layout = {
            .layoutDirection = layout_dir,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
        }
    ) {
        /* Pane 1: PERCENT(ratio) in split direction, GROW in other */
        if (direction == 0) {
            /* Horizontal: width = PERCENT(ratio), height = GROW */
            CEL_Clay(
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = {
                        .width = CLAY_SIZING_PERCENT(ratio),
                        .height = CLAY_SIZING_GROW(0)
                    }
                }
            ) {
                CEL_Clay_ChildAt(0);
            }
        } else {
            /* Vertical: width = GROW, height = PERCENT(ratio) */
            CEL_Clay(
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_PERCENT(ratio)
                    }
                }
            ) {
                CEL_Clay_ChildAt(0);
            }
        }

        /* Divider: FIXED(1) in split direction, GROW in other */
        if (direction == 0) {
            CEL_Clay(
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(1),
                        .height = CLAY_SIZING_GROW(0)
                    }
                },
                .backgroundColor = div_color
            ) {}
        } else {
            CEL_Clay(
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_FIXED(1)
                    }
                },
                .backgroundColor = div_color
            ) {}
        }

        /* Pane 2: GROW to fill remaining space */
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0)
                }
            }
        ) {
            CEL_Clay_ChildAt(1);
        }
    }
}

/* ============================================================================
 * Scrollable Container Layout
 * ============================================================================ */

/* ============================================================================
 * Data Visualization Helpers
 * ============================================================================ */

static CEL_Color w_color_lerp(CEL_Color a, CEL_Color b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return (CEL_Color){
        .r = (float)(a.r + (b.r - a.r) * t),
        .g = (float)(a.g + (b.g - a.g) * t),
        .b = (float)(a.b + (b.b - a.b) * t),
        .a = 255
    };
}

/* Three-stop gradient: start -> mid -> end based on normalized [0,1] value */
static CEL_Color w_value_gradient(float normalized,
                                   CEL_Color start, CEL_Color mid, CEL_Color end) {
    if (normalized < 0.5f) {
        return w_color_lerp(start, mid, normalized * 2.0f);
    } else {
        return w_color_lerp(mid, end, (normalized - 0.5f) * 2.0f);
    }
}

/* ============================================================================
 * Data Visualization Layouts
 * ============================================================================ */

void w_spark_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Spark* d = (const W_Spark*)ecs_get_id(world, self, W_Spark_id);
    if (!d || d->count <= 0 || !d->values) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_SparkStyle* s = d->style;

    /* Auto-scale: find min/max from data */
    float data_min = d->values[0];
    float data_max = d->values[0];
    for (int i = 1; i < d->count; i++) {
        if (d->values[i] < data_min) data_min = d->values[i];
        if (d->values[i] > data_max) data_max = d->values[i];
    }

    /* Override with manual min/max if explicitly set */
    float range_min = d->has_min ? d->min : data_min;
    float range_max = d->has_max ? d->max : data_max;

    /* Division-by-zero guard: all same value renders at mid-height */
    float range = range_max - range_min;
    if (range <= 0.0f) range = 1.0f;

    /* Unicode block characters U+2581 through U+2588 (8 levels) */
    static const char* spark_blocks[] = {
        "\xe2\x96\x81", "\xe2\x96\x82", "\xe2\x96\x83", "\xe2\x96\x84",
        "\xe2\x96\x85", "\xe2\x96\x86", "\xe2\x96\x87", "\xe2\x96\x88"
    };

    /* Build display string: 3 bytes per block char, max ~170 values for 512 buffer */
    static char spark_buf[512];
    int pos = 0;
    int max_vals = d->count;
    if (max_vals > 170) max_vals = 170; /* Safety: 170 * 3 = 510 < 512 */

    for (int i = 0; i < max_vals; i++) {
        float normalized = (d->values[i] - range_min) / range;
        /* Clamp to [0, 1] */
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;

        /* Map to block index 0-7 */
        int idx = (int)(normalized * 7.0f + 0.5f);
        if (idx > 7) idx = 7;

        /* When all values are the same, range is forced to 1.0 and normalized
         * will be some value. The mid-height block (index 3-4) is appropriate.
         * With range=1.0 and (val-min)/1.0, all same values give normalized=0
         * which maps to idx=0. Force mid-height when range was zero. */
        if (range_max == range_min) idx = 3;

        memcpy(spark_buf + pos, spark_blocks[idx], 3);
        pos += 3;
    }
    spark_buf[pos] = '\0';

    /* Spark color: style override or theme primary */
    CEL_Color spark_fg = (s && s->spark_color.a > 0) ? s->spark_color : t->primary.color;

    CEL_Clay(
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) }
        }
    ) {
        CLAY_TEXT(CEL_Clay_Text(spark_buf, pos),
            CLAY_TEXT_CONFIG({ .textColor = spark_fg,
                              .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
    }
}

void w_bar_chart_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_BarChart* d = (const W_BarChart*)ecs_get_id(world, self, W_BarChart_id);
    if (!d || d->count <= 0 || !d->entries) return;
    const Widget_Theme* t = Widget_get_theme();
    const Widget_BarChartStyle* s = d->style;

    /* Auto-scale max_value from data if 0 */
    float max_val = d->max_value;
    if (max_val <= 0.0f) {
        max_val = d->entries[0].value;
        for (int i = 1; i < d->count; i++) {
            if (d->entries[i].value > max_val)
                max_val = d->entries[i].value;
        }
        if (max_val <= 0.0f) max_val = 1.0f; /* Guard zero max */
    }

    /* Default gradient colors */
    CEL_Color grad_start = (s && s->gradient_start.a > 0)
        ? s->gradient_start : (CEL_Color){80, 200, 100, 255};
    CEL_Color grad_mid = (s && s->gradient_mid.a > 0)
        ? s->gradient_mid : (CEL_Color){220, 200, 60, 255};
    CEL_Color grad_end = (s && s->gradient_end.a > 0)
        ? s->gradient_end : (CEL_Color){220, 80, 80, 255};

    /* Label and value colors */
    CEL_Color label_fg = (s && s->label_color.a > 0)
        ? s->label_color : t->content_muted.color;
    CEL_Color value_fg = (s && s->value_color.a > 0)
        ? s->value_color : t->content.color;
    CEL_Color default_bar_fg = (s && s->bar_color.a > 0)
        ? s->bar_color : t->progress_fill.color;

    int bar_max_width = 30; /* Default bar width in terminal columns */

    /* Outer container: vertical stack */
    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) }
        }
    ) {
        for (int i = 0; i < d->count && i < 32; i++) {
            float fill_ratio = d->entries[i].value / max_val;
            if (fill_ratio < 0.0f) fill_ratio = 0.0f;
            if (fill_ratio > 1.0f) fill_ratio = 1.0f;

            int fill_width = (int)(fill_ratio * bar_max_width + 0.5f);
            if (fill_width > bar_max_width) fill_width = bar_max_width;

            /* Determine bar color */
            CEL_Color bar_fg;
            if (d->entries[i].color.a > 0) {
                /* Per-bar color override */
                bar_fg = d->entries[i].color;
            } else if (d->gradient) {
                /* Gradient: green-yellow-red based on normalized value */
                bar_fg = w_value_gradient(fill_ratio, grad_start, grad_mid, grad_end);
            } else {
                bar_fg = default_bar_fg;
            }

            /* Build bar string: full block chars for fill */
            static char bar_buf[128];
            int bpos = 0;
            for (int j = 0; j < fill_width && bpos + 3 < (int)sizeof(bar_buf); j++) {
                bar_buf[bpos++] = '\xe2';
                bar_buf[bpos++] = '\x96';
                bar_buf[bpos++] = '\x88';
            }
            bar_buf[bpos] = '\0';

            /* Format label and value text */
            static char label_buf[16];
            static char val_buf[12];
            const char* lbl = d->entries[i].label ? d->entries[i].label : "";
            int label_len = snprintf(label_buf, sizeof(label_buf), "%-12s", lbl);
            int val_len = snprintf(val_buf, sizeof(val_buf), " %6.1f", (double)d->entries[i].value);

            /* Row: label | bar fill | value */
            CEL_Clay(
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) }
                }
            ) {
                /* Label */
                CLAY_TEXT(CEL_Clay_Text(label_buf, label_len),
                    CLAY_TEXT_CONFIG({ .textColor = label_fg,
                                      .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
                /* Bar fill */
                if (bpos > 0) {
                    CLAY_TEXT(CEL_Clay_Text(bar_buf, bpos),
                        CLAY_TEXT_CONFIG({ .textColor = bar_fg,
                                          .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
                }
                /* Value */
                CLAY_TEXT(CEL_Clay_Text(val_buf, val_len),
                    CLAY_TEXT_CONFIG({ .textColor = value_fg,
                                      .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
            }
        }
    }
}

/* ============================================================================
 * Scrollable Container Layout
 * ============================================================================ */

void w_scrollable_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_ScrollContainer* d = (const W_ScrollContainer*)ecs_get_id(
        world, self, W_ScrollContainer_id);
    const Widget_Theme* t = Widget_get_theme();
    const Widget_ScrollableStyle* s = (d ? d->style : NULL);

    /* Read scroll state from behavioral component */
    const W_Scrollable* scr = (const W_Scrollable*)ecs_get_id(
        world, self, W_Scrollable_id);
    int offset = scr ? scr->scroll_offset : 0;
    int total = scr ? scr->total_count : 0;
    int visible = scr ? scr->visible_count : 0;
    bool needs_scrollbar = total > visible && visible > 0;

    /* Viewport height from component (developer-provided) */
    int vp_height = (d && d->height > 0) ? d->height : 10;

    /* Colors */
    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface.color;
    CEL_Color track_color = (s && s->track_color.a > 0) ? s->track_color : t->surface_alt.color;
    CEL_Color thumb_color = (s && s->thumb_color.a > 0) ? s->thumb_color : t->content_muted.color;

    /* Outer container: horizontal (content viewport | scrollbar gutter)
     * GROW height so the scrollable fills whatever space its parent provides,
     * rather than requiring an exact pixel match with vp_height. */
    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0)
            }
        }
    ) {
        /* Content viewport — virtual rendering: only lay out visible children.
         * When total > visible, skip Clay element creation for off-screen items.
         * Clip offset is 0 because we only render the visible slice. */
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
                .childOffset = { .x = 0, .y = needs_scrollbar ? 0 : -(float)offset }
            },
            .backgroundColor = bg_color
        ) {
            if (needs_scrollbar) {
                CEL_Clay_ChildrenRange(offset, visible);
            } else {
                CEL_Clay_Children();
            }
        }

        /* Scrollbar gutter (only when content overflows) */
        if (needs_scrollbar) {
            int track_h = vp_height;
            int thumb_h = (visible * track_h) / total;
            if (thumb_h < 1) thumb_h = 1;
            int max_off = total - visible;
            int thumb_y = (max_off > 0) ? (offset * (track_h - thumb_h)) / max_off : 0;
            int track_below = track_h - thumb_y - thumb_h;
            if (track_below < 0) track_below = 0;

            /* 1-terminal-cell wide gutter column (divide by aspect ratio) */
            float cell_w = 1.0f / CEL_CELL_ASPECT_RATIO;
            CEL_Clay(
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(cell_w),
                        .height = CLAY_SIZING_GROW(0)
                    }
                }
            ) {
                /* Track above thumb */
                if (thumb_y > 0) {
                    CEL_Clay(
                        .layout = { .sizing = {
                            .width = CLAY_SIZING_FIXED(cell_w),
                            .height = CLAY_SIZING_FIXED((float)thumb_y)
                        }},
                        .backgroundColor = track_color
                    ) {}
                }
                /* Thumb */
                CEL_Clay(
                    .layout = { .sizing = {
                        .width = CLAY_SIZING_FIXED(cell_w),
                        .height = CLAY_SIZING_FIXED((float)thumb_h)
                    }},
                    .backgroundColor = thumb_color
                ) {}
                /* Track below thumb */
                if (track_below > 0) {
                    CEL_Clay(
                        .layout = { .sizing = {
                            .width = CLAY_SIZING_FIXED(cell_w),
                            .height = CLAY_SIZING_FIXED((float)track_below)
                        }},
                        .backgroundColor = track_color
                    ) {}
                }
            }
        }
    }
}

/* ============================================================================
 * Powerline Layout
 * ============================================================================ */

/* Powerline glyph tables */
typedef struct {
    const char* left_hard;   /* Arrow separator (hard) */
    const char* left_soft;   /* Thin separator (soft) */
    const char* left_round;  /* Round separator */
} PowerlineGlyphs;

static const PowerlineGlyphs PL_ASCII = {
    .left_hard  = ">",
    .left_soft  = "|",
    .left_round = "(",
};

static const PowerlineGlyphs PL_NERD = {
    .left_hard  = "\xee\x82\xb0",  /* U+E0B0 */
    .left_soft  = "\xee\x82\xb1",  /* U+E0B1 */
    .left_round = "\xee\x82\xb4",  /* U+E0B4 */
};

/* ============================================================================
 * Log Viewer Layout
 * ============================================================================ */

void w_log_viewer_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_LogViewer* d = (const W_LogViewer*)ecs_get_id(
        world, self, W_LogViewer_id);
    if (!d || d->entry_count <= 0 || !d->entries) {
        /* Empty state: render placeholder */
        const Widget_Theme* t0 = Widget_get_theme();
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_FIXED((float)(d ? d->visible_height : 10)) },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = t0->surface.color
        ) {
            const char* msg = (d && d->entry_count <= 0) ? "No log entries" : "No log entries";
            CLAY_TEXT(CEL_Clay_Text(msg, (int)strlen(msg)),
                CLAY_TEXT_CONFIG({ .textColor = t0->content_muted.color,
                                  .userData = w_pack_text_attr((CEL_TextAttr){ .dim = true }) }));
        }
        return;
    }

    const Widget_Theme* t = Widget_get_theme();
    const Widget_LogViewerStyle* s = d->style;

    /* visible_height > 0: FIXED height with own border (standalone usage).
     * visible_height == 0: GROW to fill parent, no border (inside Panel). */
    bool grow_mode = (d->visible_height <= 0);
    int vp_height = grow_mode ? 0 : d->visible_height;

    /* Border check: skip own border when style says NONE or in grow mode */
    CEL_BorderMode border_mode = (s && s->border) ? s->border : CEL_BORDER_DEFAULT;
    bool has_border = !grow_mode && (border_mode != CEL_BORDER_NONE);

    /* Content rows: subtract border padding (1 top + 1 bottom) when bordered.
     * In grow mode, show all filtered entries (no fixed viewport). */
    int content_rows;
    if (grow_mode) {
        content_rows = d->entry_count; /* will be clamped to filtered_count */
    } else {
        content_rows = has_border ? (vp_height - 2) : vp_height;
        if (content_rows < 1) content_rows = 1;
    }

    /* Get mutable state components */
    W_LogViewerState* state = (W_LogViewerState*)ecs_get_mut_id(
        world, self, W_LogViewerState_id);
    W_Scrollable* scroll = (W_Scrollable*)ecs_get_mut_id(
        world, self, W_Scrollable_id);
    if (!state || !scroll) return;

    /* One-time init of LogViewerState */
    if (!state->initialized) {
        state->initialized = true;
        state->auto_scroll = true;
        state->prev_entry_count = d->entry_count;
    }

    /* ---- Severity filtering ---- */
    int filtered_indices[1024];
    int filtered_count = 0;
    for (int i = 0; i < d->entry_count && filtered_count < 1024; i++) {
        int level_bit = 1 << d->entries[i].level;
        if (d->severity_filter & level_bit) {
            filtered_indices[filtered_count++] = i;
        }
    }

    /* Update W_Scrollable total_count to filtered size */
    scroll->total_count = filtered_count;
    scroll->visible_count = content_rows;

    /* ---- Auto-scroll logic ---- */
    bool new_entries = (d->entry_count > state->prev_entry_count);
    state->prev_entry_count = d->entry_count;

    int max_offset = filtered_count - content_rows;
    if (max_offset < 0) max_offset = 0;

    if (state->auto_scroll && new_entries) {
        scroll->scroll_offset = max_offset;
    }

    /* Detect manual scroll-up: user scrolled away from bottom */
    if (scroll->scroll_offset < max_offset) {
        state->auto_scroll = false;
    }
    /* Detect scroll-to-bottom: re-enable auto-scroll */
    if (scroll->scroll_offset >= max_offset && max_offset > 0) {
        state->auto_scroll = true;
    }

    int offset = scroll->scroll_offset;
    if (offset < 0) offset = 0;
    if (offset > max_offset) offset = max_offset;

    bool needs_scrollbar = filtered_count > content_rows && content_rows > 0;

    /* ---- Colors ---- */
    CEL_Color bg_color = (s && s->bg.a > 0) ? s->bg : t->surface.color;
    CEL_Color debug_fg = (s && s->debug_color.a > 0) ? s->debug_color : t->content_muted.color;
    CEL_Color info_fg  = (s && s->info_color.a > 0)  ? s->info_color  : t->content.color;
    CEL_Color warn_fg  = (s && s->warn_color.a > 0)  ? s->warn_color  : t->status_warning.color;
    CEL_Color error_fg = (s && s->error_color.a > 0) ? s->error_color : t->status_error.color;
    CEL_Color ts_fg    = (s && s->timestamp_color.a > 0) ? s->timestamp_color : t->content_muted.color;

    CEL_Color bdr_color = (s && s->border_color.a > 0) ? s->border_color : t->border.color;
    CEL_Color track_color = t->surface_alt.color;
    CEL_Color thumb_color = t->content_muted.color;

    /* Border decoration for log viewport frame (skipped in grow/borderless mode) */
    CelClayBorderDecor* decor = NULL;
    if (has_border) {
        decor = _alloc_border_decor();
        *decor = (CelClayBorderDecor){
            .border_color = bdr_color,
            .bg_color = bg_color,
            .border_style = 0,
        };
    }

    /* Sizing and padding depend on mode */
    Clay_SizingAxis h_sizing = grow_mode
        ? CLAY_SIZING_GROW(0)
        : CLAY_SIZING_FIXED((float)vp_height);
    Clay_Padding lv_pad = has_border
        ? (Clay_Padding){ .left = 1, .right = 1, .top = 1, .bottom = 1 }
        : (Clay_Padding){ 0 };

    /* Handle "all filtered out" case */
    if (filtered_count == 0) {
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = CLAY_SIZING_GROW(0),
                            .height = h_sizing },
                .padding = lv_pad,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = bg_color,
            .userData = decor
        ) {
            const char* msg = "No matching entries";
            CLAY_TEXT(CEL_Clay_Text(msg, (int)strlen(msg)),
                CLAY_TEXT_CONFIG({ .textColor = t->content_muted.color,
                                  .userData = w_pack_text_attr((CEL_TextAttr){ .dim = true }) }));
        }
        return;
    }

    /* ---- Outer container: horizontal (content | scrollbar) ---- */
    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0),
                        .height = h_sizing },
            .padding = lv_pad
        },
        .backgroundColor = bg_color,
        .userData = decor
    ) {
        /* Content viewport: vertical stack of visible entries */
        CEL_Clay(
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_GROW(0) }
            }
        ) {
            int end = offset + content_rows;
            if (end > filtered_count) end = filtered_count;

            for (int vi = offset; vi < end; vi++) {
                const W_LogEntry* entry = &d->entries[filtered_indices[vi]];
                int level = entry->level;
                if (level < 0) level = 0;
                if (level > 3) level = 3;

                /* Determine severity color and attributes */
                CEL_Color line_fg;
                CEL_TextAttr line_attr = {0};
                const char* level_tag;

                switch (level) {
                    case 0:  /* DEBUG */
                        line_fg = debug_fg;
                        line_attr.dim = true;
                        level_tag = "[D]";
                        break;
                    case 1:  /* INFO */
                        line_fg = info_fg;
                        level_tag = "[I]";
                        break;
                    case 2:  /* WARN */
                        line_fg = warn_fg;
                        line_attr.bold = true;
                        level_tag = "[W]";
                        break;
                    case 3:  /* ERROR */
                        line_fg = error_fg;
                        line_attr.bold = true;
                        level_tag = "[E]";
                        break;
                    default:
                        line_fg = info_fg;
                        level_tag = "[?]";
                        break;
                }

                /* Row: LEFT_TO_RIGHT, GROW width, FIXED(1) height */
                CEL_Clay(
                    .layout = {
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .sizing = { .width = CLAY_SIZING_GROW(0),
                                    .height = CLAY_SIZING_FIXED(1) }
                    }
                ) {
                    /* Timestamp (optional) */
                    if (entry->timestamp) {
                        static char ts_buf[16];
                        int ts_len = snprintf(ts_buf, sizeof(ts_buf), "%-12s", entry->timestamp);
                        if (ts_len >= (int)sizeof(ts_buf)) ts_len = (int)sizeof(ts_buf) - 1;
                        CLAY_TEXT(CEL_Clay_Text(ts_buf, ts_len),
                            CLAY_TEXT_CONFIG({ .textColor = ts_fg,
                                              .userData = w_pack_text_attr((CEL_TextAttr){ .dim = true }) }));
                    }

                    /* Severity indicator */
                    static char tag_buf[8];
                    int tag_len = snprintf(tag_buf, sizeof(tag_buf), "%s ", level_tag);
                    CLAY_TEXT(CEL_Clay_Text(tag_buf, tag_len),
                        CLAY_TEXT_CONFIG({ .textColor = line_fg,
                                          .userData = w_pack_text_attr(line_attr) }));

                    /* Message text */
                    const char* msg = entry->message ? entry->message : "";
                    CLAY_TEXT(CEL_Clay_Text(msg, (int)strlen(msg)),
                        CLAY_TEXT_CONFIG({ .textColor = line_fg,
                                          .userData = w_pack_text_attr(line_attr) }));
                }
            }
        }

        /* ---- Scrollbar gutter (right side) ---- */
        if (needs_scrollbar) {
            int track_h = content_rows;
            int thumb_h = (content_rows * track_h) / filtered_count;
            if (thumb_h < 1) thumb_h = 1;
            int thumb_y = (max_offset > 0) ? (offset * (track_h - thumb_h)) / max_offset : 0;
            int track_below = track_h - thumb_y - thumb_h;
            if (track_below < 0) track_below = 0;

            float cell_w = 1.0f / CEL_CELL_ASPECT_RATIO;
            CEL_Clay(
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(cell_w),
                        .height = CLAY_SIZING_GROW(0)
                    }
                }
            ) {
                /* Track above thumb */
                if (thumb_y > 0) {
                    CEL_Clay(
                        .layout = { .sizing = {
                            .width = CLAY_SIZING_FIXED(cell_w),
                            .height = CLAY_SIZING_FIXED((float)thumb_y)
                        }},
                        .backgroundColor = track_color
                    ) {}
                }
                /* Thumb */
                CEL_Clay(
                    .layout = { .sizing = {
                        .width = CLAY_SIZING_FIXED(cell_w),
                        .height = CLAY_SIZING_FIXED((float)thumb_h)
                    }},
                    .backgroundColor = thumb_color
                ) {}
                /* Track below thumb */
                if (track_below > 0) {
                    CEL_Clay(
                        .layout = { .sizing = {
                            .width = CLAY_SIZING_FIXED(cell_w),
                            .height = CLAY_SIZING_FIXED((float)track_below)
                        }},
                        .backgroundColor = track_color
                    ) {}
                }
            }
        }
    }

    /* Write back modified state */
    ecs_set_id(world, self, W_LogViewerState_id,
               sizeof(W_LogViewerState), state);
    ecs_set_id(world, self, W_Scrollable_id,
               sizeof(W_Scrollable), scroll);
}

void w_powerline_layout(struct ecs_world_t* world, cels_entity_t self) {
    const W_Powerline* d = (const W_Powerline*)ecs_get_id(world, self, W_Powerline_id);
    if (!d || d->segment_count <= 0 || !d->segments) return;

    const PowerlineGlyphs* gl = Widget_powerline_glyphs_enabled() ? &PL_NERD : &PL_ASCII;

    /* Select separator based on style */
    const char* sep;
    switch (d->separator_style) {
        case 1:  sep = gl->left_round; break;
        case 2:  sep = gl->left_soft;  break;
        default: sep = gl->left_hard;  break;
    }
    int sep_len = (int)strlen(sep);

    /* Outer horizontal container */
    CEL_Clay(
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1) }
        }
    ) {
        for (int i = 0; i < d->segment_count; i++) {
            const W_PowerlineSegment* seg = &d->segments[i];
            const char* text = seg->text ? seg->text : "";

            /* Segment text with padding */
            static char seg_buf[128];
            int seg_len = snprintf(seg_buf, sizeof(seg_buf), " %s ", text);
            if (seg_len < 0) seg_len = 0;
            if (seg_len >= (int)sizeof(seg_buf)) seg_len = (int)sizeof(seg_buf) - 1;

            CEL_Clay(
                .layout = {
                    .sizing = { .height = CLAY_SIZING_FIXED(1) }
                },
                .backgroundColor = seg->bg
            ) {
                CLAY_TEXT(CEL_Clay_Text(seg_buf, seg_len),
                    CLAY_TEXT_CONFIG({ .textColor = seg->fg,
                                      .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
            }

            /* Separator between segments (not after last) */
            if (i < d->segment_count - 1) {
                CEL_Color sep_fg = seg->bg;                    /* Arrow tip = current segment color */
                CEL_Color sep_bg = d->segments[i + 1].bg;     /* Background = next segment color */

                CEL_Clay(
                    .layout = {
                        .sizing = { .height = CLAY_SIZING_FIXED(1) }
                    },
                    .backgroundColor = sep_bg
                ) {
                    CLAY_TEXT(CEL_Clay_Text(sep, sep_len),
                        CLAY_TEXT_CONFIG({ .textColor = sep_fg,
                                          .userData = w_pack_text_attr((CEL_TextAttr){0}) }));
                }
            }
        }
    }
}
