/*
 * CELS Widgets - Behavioral Systems
 *
 * Generic ECS systems that operate on behavioral components.
 * Registered automatically by Widgets_init() via widgets_behavioral_systems_register().
 *
 * Systems:
 *   W_RangeClampF   - Clamps W_RangeValueF.value to [min, max] at PostUpdate
 *   W_RangeClampI   - Clamps W_RangeValueI.value to [min, max] at PostUpdate
 *   W_ScrollClamp   - Clamps W_Scrollable.scroll_offset to valid range at PostUpdate
 *   W_ToastTimer    - Auto-dismiss timer for W_Toast notifications at PostUpdate
 *   TextInputSystem - Processes raw_key into W_TextInputBuffer edits (insert/delete/cursor)
 */

#include <cels-widgets/widgets.h>
#include <cels-widgets/input.h>
#include <flecs.h>
#include <string.h>

/* ============================================================================
 * System Callbacks
 * ============================================================================ */

static void range_clamp_f_run(CELS_Iter* it) {
    int count = cels_iter_count(it);
    W_RangeValueF* ranges = (W_RangeValueF*)cels_iter_column(it, W_RangeValueFID, sizeof(W_RangeValueF));
    if (!ranges) return;
    for (int i = 0; i < count; i++) {
        if (ranges[i].value < ranges[i].min) ranges[i].value = ranges[i].min;
        if (ranges[i].value > ranges[i].max) ranges[i].value = ranges[i].max;
    }
}

static void range_clamp_i_run(CELS_Iter* it) {
    int count = cels_iter_count(it);
    W_RangeValueI* ranges = (W_RangeValueI*)cels_iter_column(it, W_RangeValueIID, sizeof(W_RangeValueI));
    if (!ranges) return;
    for (int i = 0; i < count; i++) {
        if (ranges[i].value < ranges[i].min) ranges[i].value = ranges[i].min;
        if (ranges[i].value > ranges[i].max) ranges[i].value = ranges[i].max;
    }
}

static void scroll_clamp_run(CELS_Iter* it) {
    int count = cels_iter_count(it);
    W_Scrollable* scrolls = (W_Scrollable*)cels_iter_column(it, W_ScrollableID, sizeof(W_Scrollable));
    if (!scrolls) return;
    for (int i = 0; i < count; i++) {
        int max_offset = scrolls[i].total_count - scrolls[i].visible_count;
        if (max_offset < 0) max_offset = 0;
        if (scrolls[i].scroll_offset < 0) scrolls[i].scroll_offset = 0;
        if (scrolls[i].scroll_offset > max_offset) scrolls[i].scroll_offset = max_offset;
    }
}

/* W_ToastTimer: auto-dismiss timer for toast notifications */
static void toast_timer_run(CELS_Iter* it) {
    int count = cels_iter_count(it);
    W_Toast* toasts = (W_Toast*)cels_iter_column(it, W_ToastID, sizeof(W_Toast));
    float dt = cels_iter_delta_time(it);
    if (!toasts) return;
    for (int i = 0; i < count; i++) {
        if (toasts[i].dismissed) continue;
        toasts[i].elapsed += dt;
        if (toasts[i].elapsed >= toasts[i].duration) {
            toasts[i].dismissed = true;
        }
    }
}

/* ============================================================================
 * TextInputSystem: processes raw_key into W_TextInputBuffer edits
 *
 * Called from the focus system each frame. Queries all entities with BOTH
 * W_TextInput and W_TextInputBuffer. Only processes the entity if it is
 * focused AND selected (text input must be activated to accept typing).
 *
 * Input processing:
 *   - Printable chars (32-126): insert at cursor position
 *   - Backspace: delete char before cursor
 *   - Delete: delete char at cursor
 *   - Left/Right arrows: move cursor (edge-detected)
 *   - Home/End: jump cursor to start/end (edge-detected)
 *   - Enter: call on_submit callback (single-line)
 * ============================================================================ */

void text_input_system_run(ecs_world_t* world, const CELS_Input* input,
                           const CELS_Input* prev_input) {
    W_TextInput_ensure();
    W_TextInputBuffer_ensure();
    W_Selectable_ensure();
    W_InteractState_ensure();

    /* Query all entities with both W_TextInput and W_TextInputBuffer */
    ecs_query_t* q = ecs_query(world, {
        .terms = {{ .id = W_TextInputID }, { .id = W_TextInputBufferID }}
    });
    if (!q) return;

    ecs_iter_t qit = ecs_query_iter(world, q);
    while (ecs_query_next(&qit)) {
        for (int e = 0; e < qit.count; e++) {
            ecs_entity_t entity = qit.entities[e];

            W_TextInputBuffer* buf = (W_TextInputBuffer*)ecs_get_mut_id(
                world, entity, W_TextInputBufferID);
            if (!buf) continue;

            const W_TextInput* cfg = (const W_TextInput*)ecs_get_id(
                world, entity, W_TextInputID);
            if (!cfg) continue;

            /* One-time init */
            if (!buf->initialized) {
                buf->initialized = true;
                buf->cursor_pos = 0;
                buf->length = 0;
                buf->byte_length = 0;
                buf->sel_start = -1;
                buf->sel_end = -1;
                buf->scroll_x = 0;
                memset(buf->buffer, 0, sizeof(buf->buffer));
            }

            /* Skip inactive: must be focused AND selected */
            const W_Selectable* sel = (const W_Selectable*)ecs_get_id(
                world, entity, W_SelectableID);
            const W_InteractState* ist = (const W_InteractState*)ecs_get_id(
                world, entity, W_InteractStateID);
            bool selected = sel ? sel->selected : false;
            bool focused = ist ? ist->focused : false;
            if (!selected || !focused) continue;

            int max_len = (cfg->max_length > 0) ? cfg->max_length : 255;
            if (max_len > 255) max_len = 255;
            bool modified = false;

            /* Character insertion: printable ASCII */
            if (input->has_raw_key && input->raw_key >= 32 && input->raw_key <= 126) {
                if (buf->length < max_len) {
                    char ch = (char)input->raw_key;
                    int byte_pos = buf->cursor_pos; /* ASCII: 1 byte per char */

                    /* Shift right to make room */
                    if (byte_pos < buf->byte_length) {
                        memmove(buf->buffer + byte_pos + 1,
                                buf->buffer + byte_pos,
                                (size_t)(buf->byte_length - byte_pos));
                    }
                    buf->buffer[byte_pos] = ch;
                    buf->cursor_pos++;
                    buf->length++;
                    buf->byte_length++;
                    buf->buffer[buf->byte_length] = '\0';
                    modified = true;
                }
            }

            /* Backspace: delete char before cursor */
            if (input->key_backspace && buf->cursor_pos > 0) {
                int byte_pos = buf->cursor_pos - 1; /* ASCII: 1 byte per char */

                /* Shift left to fill gap */
                if (byte_pos < buf->byte_length - 1) {
                    memmove(buf->buffer + byte_pos,
                            buf->buffer + byte_pos + 1,
                            (size_t)(buf->byte_length - byte_pos - 1));
                }
                buf->cursor_pos--;
                buf->length--;
                buf->byte_length--;
                buf->buffer[buf->byte_length] = '\0';
                modified = true;
            }

            /* Delete: delete char at cursor */
            if (input->key_delete && buf->cursor_pos < buf->length) {
                int byte_pos = buf->cursor_pos; /* ASCII: 1 byte per char */

                /* Shift left to fill gap */
                if (byte_pos < buf->byte_length - 1) {
                    memmove(buf->buffer + byte_pos,
                            buf->buffer + byte_pos + 1,
                            (size_t)(buf->byte_length - byte_pos - 1));
                }
                buf->length--;
                buf->byte_length--;
                buf->buffer[buf->byte_length] = '\0';
                modified = true;
            }

            /* Arrow keys: edge-detected cursor movement */
            bool left_edge = (input->axis_left[0] < -0.5f &&
                              (prev_input ? prev_input->axis_left[0] >= -0.5f : true));
            bool right_edge = (input->axis_left[0] > 0.5f &&
                               (prev_input ? prev_input->axis_left[0] <= 0.5f : true));

            if (left_edge && buf->cursor_pos > 0) {
                buf->cursor_pos--;
            }
            if (right_edge && buf->cursor_pos < buf->length) {
                buf->cursor_pos++;
            }

            /* Home/End: edge-detected */
            bool home_edge = (input->key_home &&
                              (prev_input ? !prev_input->key_home : true));
            bool end_edge = (input->key_end &&
                             (prev_input ? !prev_input->key_end : true));

            if (home_edge) {
                buf->cursor_pos = 0;
            }
            if (end_edge) {
                buf->cursor_pos = buf->length;
            }

            /* Enter/Submit: single-line only */
            bool accept_edge = (input->button_accept &&
                                (prev_input ? !prev_input->button_accept : true));
            if (accept_edge && !cfg->multiline) {
                if (cfg->on_submit) {
                    cfg->on_submit(buf->buffer);
                }
                /* Do NOT insert the character */
            }

            /* Call on_change callback if text was modified */
            if (modified && cfg->on_change) {
                cfg->on_change(buf->buffer);
            }

            /* Update horizontal scroll */
            int visible_width = 30;
            if (buf->cursor_pos < buf->scroll_x) {
                buf->scroll_x = buf->cursor_pos;
            }
            if (buf->cursor_pos >= buf->scroll_x + visible_width) {
                buf->scroll_x = buf->cursor_pos - visible_width + 1;
            }
            if (buf->scroll_x < 0) buf->scroll_x = 0;

            /* Write back modified buffer */
            ecs_set_id(world, entity, W_TextInputBufferID,
                       sizeof(W_TextInputBuffer), buf);
        }
    }

    ecs_query_fini(q);
}

/* ============================================================================
 * Text Input Active Detection
 *
 * Check if any text input entity is currently focused+selected.
 * Used by the focus system to suppress q-quit and arrow navigation.
 * ============================================================================ */

bool text_input_is_active(ecs_world_t* world) {
    W_TextInputBuffer_ensure();
    W_Selectable_ensure();
    W_InteractState_ensure();

    ecs_query_t* q = ecs_query(world, {
        .terms = {{ .id = W_TextInputBufferID }}
    });
    if (!q) return false;

    bool active = false;
    ecs_iter_t qit = ecs_query_iter(world, q);
    while (ecs_query_next(&qit) && !active) {
        for (int e = 0; e < qit.count && !active; e++) {
            ecs_entity_t entity = qit.entities[e];
            const W_Selectable* sel = (const W_Selectable*)ecs_get_id(
                world, entity, W_SelectableID);
            const W_InteractState* ist = (const W_InteractState*)ecs_get_id(
                world, entity, W_InteractStateID);
            if ((sel && sel->selected) && (ist && ist->focused)) {
                active = true;
            }
        }
    }
    ecs_query_fini(q);
    return active;
}

/* ============================================================================
 * Registration
 * ============================================================================ */

static bool s_behavioral_registered = false;

void widgets_behavioral_systems_register(void) {
    if (s_behavioral_registered) return;
    s_behavioral_registered = true;

    W_RangeValueF_ensure();
    W_RangeValueI_ensure();
    W_Scrollable_ensure();
    W_Toast_ensure();
    W_TextInput_ensure();
    W_TextInputBuffer_ensure();

    cels_entity_t range_f_comps[] = { W_RangeValueFID };
    cels_system_declare("W_RangeClampF", CELS_Phase_OnUpdate,
                        range_clamp_f_run, range_f_comps, 1);

    cels_entity_t range_i_comps[] = { W_RangeValueIID };
    cels_system_declare("W_RangeClampI", CELS_Phase_OnUpdate,
                        range_clamp_i_run, range_i_comps, 1);

    cels_entity_t scroll_comps[] = { W_ScrollableID };
    cels_system_declare("W_ScrollClamp", CELS_Phase_OnUpdate,
                        scroll_clamp_run, scroll_comps, 1);

    cels_entity_t toast_comps[] = { W_ToastID };
    cels_system_declare("W_ToastTimer", CELS_Phase_OnUpdate,
                        toast_timer_run, toast_comps, 1);
}
