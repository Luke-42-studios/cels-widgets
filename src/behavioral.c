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
 */

#include <cels-widgets/widgets.h>
#include <cels-widgets/input.h>

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

    cels_entity_t range_f_comps[] = { W_RangeValueFID };
    cels_system_declare("W_RangeClampF", CELS_Phase_PostUpdate,
                        range_clamp_f_run, range_f_comps, 1);

    cels_entity_t range_i_comps[] = { W_RangeValueIID };
    cels_system_declare("W_RangeClampI", CELS_Phase_PostUpdate,
                        range_clamp_i_run, range_i_comps, 1);

    cels_entity_t scroll_comps[] = { W_ScrollableID };
    cels_system_declare("W_ScrollClamp", CELS_Phase_PostUpdate,
                        scroll_clamp_run, scroll_comps, 1);

    cels_entity_t toast_comps[] = { W_ToastID };
    cels_system_declare("W_ToastTimer", CELS_Phase_PostUpdate,
                        toast_timer_run, toast_comps, 1);
}
