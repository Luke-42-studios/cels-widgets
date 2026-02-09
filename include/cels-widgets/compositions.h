/*
 * CELS Widgets - Convenience Composition Macros
 *
 * Each widget has a composition that wires up ClayUI + component data
 * automatically. Apps just write declarative widget trees:
 *
 *   Widget_Panel(.title = "Settings") {
 *       Widget_Button(.label = "Play", .selected = true) {}
 *       Widget_Slider(.label = "Volume", .value = 0.7f) {}
 *       Widget_Divider() {}
 *       Widget_Text(.text = "Hello world") {}
 *   }
 *
 * Each composition:
 *   1. Attaches ClayUI with the widget's layout function
 *   2. Attaches the widget's data component with props
 *   3. Container widgets (Panel, ListView) render CEL_Clay_Children()
 *      so child compositions appear inside their layout
 *
 * Requires: #include <cels-widgets/widgets.h> before this header
 * Requires: cels-clay module linked (provides ClayUI, CEL_Clay macros)
 */

#ifndef CELS_WIDGETS_COMPOSITIONS_H
#define CELS_WIDGETS_COMPOSITIONS_H

#include <cels-widgets/widgets.h>
#include <cels-widgets/layouts.h>
#include <cels-layout/layout.h>
#include <cels-clay/clay_layout.h>

/* ============================================================================
 * Text & Display Compositions
 * ============================================================================ */

CEL_Composition(WText, const char* text; int align;) {
    CEL_Has(ClayUI, .layout_fn = w_text_layout);
    CEL_Has(W_Text, .text = props.text, .align = props.align);
}
#define Widget_Text(...) CEL_Init(WText, __VA_ARGS__)

CEL_Composition(WHint, const char* text;) {
    CEL_Has(ClayUI, .layout_fn = w_hint_layout);
    CEL_Has(W_Hint, .text = props.text);
}
#define Widget_Hint(...) CEL_Init(WHint, __VA_ARGS__)

CEL_Composition(WCanvas, const char* title; int width; const Widget_CanvasStyle* style;) {
    CEL_Has(ClayUI, .layout_fn = w_canvas_layout);
    CEL_Has(W_Canvas, .title = props.title, .width = props.width, .style = props.style);
}
#define Widget_Canvas(...) CEL_Init(WCanvas, __VA_ARGS__)

CEL_Composition(WInfoBox, const char* title; const char* content; bool border;) {
    CEL_Has(ClayUI, .layout_fn = w_info_box_layout);
    CEL_Has(W_InfoBox, .title = props.title, .content = props.content, .border = props.border);
}
#define Widget_InfoBox(...) CEL_Init(WInfoBox, __VA_ARGS__)

CEL_Composition(WBadge, const char* text; unsigned char r; unsigned char g; unsigned char b;) {
    CEL_Has(ClayUI, .layout_fn = w_badge_layout);
    CEL_Has(W_Badge, .text = props.text, .r = props.r, .g = props.g, .b = props.b);
}
#define Widget_Badge(...) CEL_Init(WBadge, __VA_ARGS__)

CEL_Composition(WTextArea, const char* text; int max_width; int max_height; bool scrollable;) {
    CEL_Has(ClayUI, .layout_fn = w_text_area_layout);
    CEL_Has(W_TextArea, .text = props.text, .max_width = props.max_width,
            .max_height = props.max_height, .scrollable = props.scrollable);
}
#define Widget_TextArea(...) CEL_Init(WTextArea, __VA_ARGS__)

/* ============================================================================
 * Interactive Compositions
 * ============================================================================ */

CEL_Composition(WButton, const char* label; bool selected; bool focused; void (*on_press)(void); const Widget_ButtonStyle* style;) {
    CEL_Has(ClayUI, .layout_fn = w_button_layout);
    CEL_Has(W_Button, .label = props.label, .selected = props.selected,
            .focused = props.focused, .on_press = props.on_press, .style = props.style);
}
#define Widget_Button(...) CEL_Init(WButton, __VA_ARGS__)

CEL_Composition(WSlider, const char* label; float value; float min; float max; bool selected;) {
    CEL_Has(ClayUI, .layout_fn = w_slider_layout);
    CEL_Has(W_Slider, .label = props.label, .value = props.value,
            .min = props.min, .max = props.max, .selected = props.selected);
}
#define Widget_Slider(...) CEL_Init(WSlider, __VA_ARGS__)

CEL_Composition(WToggle, const char* label; bool value; bool selected;) {
    CEL_Has(ClayUI, .layout_fn = w_toggle_layout);
    CEL_Has(W_Toggle, .label = props.label, .value = props.value, .selected = props.selected);
}
#define Widget_Toggle(...) CEL_Init(WToggle, __VA_ARGS__)

CEL_Composition(WCycle, const char* label; const char* value; bool selected;) {
    CEL_Has(ClayUI, .layout_fn = w_cycle_layout);
    CEL_Has(W_Cycle, .label = props.label, .value = props.value, .selected = props.selected);
}
#define Widget_Cycle(...) CEL_Init(WCycle, __VA_ARGS__)

/* ============================================================================
 * Progress & Metric Compositions
 * ============================================================================ */

CEL_Composition(WProgressBar, const char* label; float value; bool color_by_value;) {
    CEL_Has(ClayUI, .layout_fn = w_progress_bar_layout);
    CEL_Has(W_ProgressBar, .label = props.label, .value = props.value,
            .color_by_value = props.color_by_value);
}
#define Widget_ProgressBar(...) CEL_Init(WProgressBar, __VA_ARGS__)

CEL_Composition(WMetric, const char* label; const char* value; int status;) {
    CEL_Has(ClayUI, .layout_fn = w_metric_layout);
    CEL_Has(W_Metric, .label = props.label, .value = props.value, .status = props.status);
}
#define Widget_Metric(...) CEL_Init(WMetric, __VA_ARGS__)

/* ============================================================================
 * Container Compositions
 * ============================================================================ */

CEL_Composition(WPanel, const char* title; int border_style; const Widget_PanelStyle* style;) {
    CEL_Has(ClayUI, .layout_fn = w_panel_layout);
    CEL_Has(W_Panel, .title = props.title, .border_style = props.border_style, .style = props.style);
}
#define Widget_Panel(...) CEL_Init(WPanel, __VA_ARGS__)

CEL_Composition(WDivider, bool vertical;) {
    CEL_Has(ClayUI, .layout_fn = w_divider_layout);
    CEL_Has(W_Divider, .vertical = props.vertical);
}
#define Widget_Divider(...) CEL_Init(WDivider, __VA_ARGS__)

CEL_Composition(WTable, int row_count; const char** keys; const char** values;) {
    CEL_Has(ClayUI, .layout_fn = w_table_layout);
    CEL_Has(W_Table, .row_count = props.row_count, .keys = props.keys, .values = props.values);
}
#define Widget_Table(...) CEL_Init(WTable, __VA_ARGS__)

/* ============================================================================
 * Radio Compositions
 * ============================================================================ */

CEL_Composition(WRadioButton, const char* label; bool selected; int group_id;) {
    CEL_Has(ClayUI, .layout_fn = w_radio_button_layout);
    CEL_Has(W_RadioButton, .label = props.label, .selected = props.selected,
            .group_id = props.group_id);
}
#define Widget_RadioButton(...) CEL_Init(WRadioButton, __VA_ARGS__)

CEL_Composition(WRadioGroup, int group_id; int selected_index; int count;) {
    CEL_Has(ClayUI, .layout_fn = w_radio_group_layout);
    CEL_Has(W_RadioGroup, .group_id = props.group_id,
            .selected_index = props.selected_index, .count = props.count);
}
#define Widget_RadioGroup(...) CEL_Init(WRadioGroup, __VA_ARGS__)

/* ============================================================================
 * Navigation Compositions
 * ============================================================================ */

CEL_Composition(WTabBar, int active; int count; const char** labels;) {
    CEL_Has(ClayUI, .layout_fn = w_tab_bar_layout);
    CEL_Has(W_TabBar, .active = props.active, .count = props.count, .labels = props.labels);
}
#define Widget_TabBar(...) CEL_Init(WTabBar, __VA_ARGS__)

CEL_Composition(WTabContent, const char* text; const char* hint;) {
    CEL_Has(ClayUI, .layout_fn = w_tab_content_layout);
    CEL_Has(W_TabContent, .text = props.text, .hint = props.hint);
}
#define Widget_TabContent(...) CEL_Init(WTabContent, __VA_ARGS__)

CEL_Composition(WStatusBar, const char* left; const char* right;) {
    CEL_Has(ClayUI, .layout_fn = w_status_bar_layout);
    CEL_Has(W_StatusBar, .left = props.left, .right = props.right);
}
#define Widget_StatusBar(...) CEL_Init(WStatusBar, __VA_ARGS__)

/* ============================================================================
 * List Compositions
 * ============================================================================ */

CEL_Composition(WListView, int item_count; int selected_index; int scroll_offset; int visible_count;) {
    CEL_Has(ClayUI, .layout_fn = w_list_view_layout);
    CEL_Has(W_ListView, .item_count = props.item_count,
            .selected_index = props.selected_index,
            .scroll_offset = props.scroll_offset,
            .visible_count = props.visible_count);
}
#define Widget_ListView(...) CEL_Init(WListView, __VA_ARGS__)

CEL_Composition(WListItem, const char* label; bool selected; void* data;) {
    CEL_Has(ClayUI, .layout_fn = w_list_item_layout);
    CEL_Has(W_ListItem, .label = props.label, .selected = props.selected, .data = props.data);
}
#define Widget_ListItem(...) CEL_Init(WListItem, __VA_ARGS__)

/* ============================================================================
 * Backward Compatibility (v0.2 -> v0.3)
 * ============================================================================ */
#define WText(...)        Widget_Text(__VA_ARGS__)
#define WHint(...)        Widget_Hint(__VA_ARGS__)
#define WCanvas(...)      Widget_Canvas(__VA_ARGS__)
#define WInfoBox(...)     Widget_InfoBox(__VA_ARGS__)
#define WBadge(...)       Widget_Badge(__VA_ARGS__)
#define WTextArea(...)    Widget_TextArea(__VA_ARGS__)
#define WButton(...)      Widget_Button(__VA_ARGS__)
#define WSlider(...)      Widget_Slider(__VA_ARGS__)
#define WToggle(...)      Widget_Toggle(__VA_ARGS__)
#define WCycle(...)       Widget_Cycle(__VA_ARGS__)
#define WProgressBar(...) Widget_ProgressBar(__VA_ARGS__)
#define WMetric(...)      Widget_Metric(__VA_ARGS__)
#define WPanel(...)       Widget_Panel(__VA_ARGS__)
#define WDivider(...)     Widget_Divider(__VA_ARGS__)
#define WTable(...)       Widget_Table(__VA_ARGS__)
#define WRadioButton(...) Widget_RadioButton(__VA_ARGS__)
#define WRadioGroup(...)  Widget_RadioGroup(__VA_ARGS__)
#define WTabBar(...)      Widget_TabBar(__VA_ARGS__)
#define WTabContent(...)  Widget_TabContent(__VA_ARGS__)
#define WStatusBar(...)   Widget_StatusBar(__VA_ARGS__)
#define WListView(...)    Widget_ListView(__VA_ARGS__)
#define WListItem(...)    Widget_ListItem(__VA_ARGS__)

#endif /* CELS_WIDGETS_COMPOSITIONS_H */
