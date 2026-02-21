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
 *   3. Interactive widgets also set W_InteractState for style resolution
 *   4. Container widgets (Panel, ListView) render CEL_Clay_Children()
 *      so child compositions appear inside their layout
 *
 * All widgets accept an optional `.style` pointer for visual overrides.
 * Interactive widgets also accept `.disabled` for W_InteractState.
 * New props zero-initialize: existing code is unaffected.
 *
 * Requires: #include <cels-widgets/widgets.h> before this header
 * Requires: cels-clay module linked (provides ClayUI, CEL_Clay macros)
 */

#ifndef CELS_WIDGETS_COMPOSITIONS_H
#define CELS_WIDGETS_COMPOSITIONS_H

#include <cels-widgets/widgets.h>
#include <cels-widgets/input.h>
#include <cels-widgets/layouts.h>
#include <cels-widgets/presets.h>
#include <cels-layout/layout.h>
#include <cels-clay/clay_layout.h>

/* ============================================================================
 * Text & Display Compositions
 * ============================================================================ */

CEL_Composition(WText, const char* text; int align; const Widget_TextStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_text_layout);
    cel_has(W_Text, .text = props.text, .align = props.align, .style = props.style);
}
#define Widget_Text(...) cel_init(WText, __VA_ARGS__)

CEL_Composition(WHint, const char* text; const Widget_HintStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_hint_layout);
    cel_has(W_Hint, .text = props.text, .style = props.style);
}
#define Widget_Hint(...) cel_init(WHint, __VA_ARGS__)

CEL_Composition(WCanvas, const char* title; int width; const Widget_CanvasStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_canvas_layout);
    cel_has(W_Canvas, .title = props.title, .width = props.width, .style = props.style);
}
#define Widget_Canvas(...) cel_init(WCanvas, __VA_ARGS__)

CEL_Composition(WInfoBox, const char* title; const char* content; bool border; const Widget_InfoBoxStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_info_box_layout);
    cel_has(W_InfoBox, .title = props.title, .content = props.content,
            .border = props.border, .style = props.style);
}
#define Widget_InfoBox(...) cel_init(WInfoBox, __VA_ARGS__)

CEL_Composition(WBadge, const char* text; unsigned char r; unsigned char g; unsigned char b; const Widget_BadgeStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_badge_layout);
    cel_has(W_Badge, .text = props.text, .r = props.r, .g = props.g, .b = props.b,
            .style = props.style);
}
#define Widget_Badge(...) cel_init(WBadge, __VA_ARGS__)

CEL_Composition(WTextArea, const char* text; int max_width; int max_height; bool scrollable; const Widget_TextAreaStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_text_area_layout);
    cel_has(W_TextArea, .text = props.text, .max_width = props.max_width,
            .max_height = props.max_height, .scrollable = props.scrollable,
            .style = props.style);
    /* W_Scrollable: scroll state for content overflow, populated by layout */
    cel_has(W_Scrollable, .scroll_offset = 0, .total_count = 0, .visible_count = 0);
}
#define Widget_TextArea(...) cel_init(WTextArea, __VA_ARGS__)

/* ============================================================================
 * Interactive Compositions
 *
 * Each interactive widget sets W_InteractState for centralized style
 * resolution via w_resolve_visual(). The .disabled prop is new (defaults
 * to false) and does not affect existing code.
 * ============================================================================ */

CEL_Composition(WButton, const char* label; bool selected; bool focused; bool disabled;
                 void (*on_press)(void); const Widget_ButtonStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_button_layout);
    cel_has(W_Button, .label = props.label, .on_press = props.on_press, .style = props.style);
    cel_has(W_Selectable, .selected = props.selected);
    cel_has(W_InteractState, .selected = props.selected,
            .focused = props.focused, .disabled = props.disabled);
}
#define Widget_Button(...) cel_init(WButton, __VA_ARGS__)

CEL_Composition(WSlider, const char* label; float value; float min; float max;
                 bool selected; bool disabled; const Widget_SliderStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_slider_layout);
    cel_has(W_Slider, .label = props.label, .style = props.style);
    cel_has(W_RangeValueF, .value = props.value, .min = props.min,
            .max = props.max, .step = 0.1f);
    cel_has(W_Selectable, .selected = props.selected);
    cel_has(W_InteractState, .selected = props.selected, .disabled = props.disabled);
}
#define Widget_Slider(...) cel_init(WSlider, __VA_ARGS__)

CEL_Composition(WToggle, const char* label; bool value; bool selected; bool disabled;
                 const Widget_ToggleStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_toggle_layout);
    cel_has(W_Toggle, .label = props.label, .value = props.value, .style = props.style);
    cel_has(W_Selectable, .selected = props.selected);
    cel_has(W_InteractState, .selected = props.selected, .disabled = props.disabled);
}
#define Widget_Toggle(...) cel_init(WToggle, __VA_ARGS__)

CEL_Composition(WCycle, const char* label; const char* value; bool selected; bool disabled;
                 const Widget_CycleStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_cycle_layout);
    cel_has(W_Cycle, .label = props.label, .value = props.value, .style = props.style);
    cel_has(W_Selectable, .selected = props.selected);
    cel_has(W_InteractState, .selected = props.selected, .disabled = props.disabled);
}
#define Widget_Cycle(...) cel_init(WCycle, __VA_ARGS__)

/* ============================================================================
 * Progress & Metric Compositions
 * ============================================================================ */

CEL_Composition(WProgressBar, const char* label; float value; bool color_by_value;
                 const Widget_ProgressBarStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_progress_bar_layout);
    cel_has(W_ProgressBar, .label = props.label,
            .color_by_value = props.color_by_value, .style = props.style);
    cel_has(W_RangeValueF, .value = props.value, .min = 0.0f,
            .max = 1.0f, .step = 0.01f);
}
#define Widget_ProgressBar(...) cel_init(WProgressBar, __VA_ARGS__)

CEL_Composition(WMetric, const char* label; const char* value; int status;
                 const Widget_MetricStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_metric_layout);
    cel_has(W_Metric, .label = props.label, .value = props.value,
            .status = props.status, .style = props.style);
}
#define Widget_Metric(...) cel_init(WMetric, __VA_ARGS__)

/* ============================================================================
 * Container Compositions
 * ============================================================================ */

CEL_Composition(WPanel, const char* title; int border_style; const Widget_PanelStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_panel_layout);
    cel_has(W_Panel, .title = props.title, .border_style = props.border_style, .style = props.style);
}
#define Widget_Panel(...) cel_init(WPanel, __VA_ARGS__)

CEL_Composition(WDivider, bool vertical; const Widget_DividerStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_divider_layout);
    cel_has(W_Divider, .vertical = props.vertical, .style = props.style);
}
#define Widget_Divider(...) cel_init(WDivider, __VA_ARGS__)

CEL_Composition(WTable, int row_count; const char** keys; const char** values;
                 const Widget_TableStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_table_layout);
    cel_has(W_Table, .row_count = props.row_count, .keys = props.keys,
            .values = props.values, .style = props.style);
}
#define Widget_Table(...) cel_init(WTable, __VA_ARGS__)

CEL_Composition(WCollapsible, const char* title; bool collapsed; int indent;
                 bool selected; const Widget_CollapsibleStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_collapsible_layout);
    cel_has(W_Collapsible, .title = props.title, .collapsed = props.collapsed,
            .indent = props.indent, .style = props.style);
    cel_has(W_Selectable, .selected = props.selected);
    cel_has(W_InteractState);
    cel_has(W_Focusable);
}
#define Widget_Collapsible(...) cel_init(WCollapsible, __VA_ARGS__)

CEL_Composition(WSplitPane, float ratio; int direction;
                 const Widget_SplitStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_split_pane_layout);
    cel_has(W_SplitPane, .ratio = props.ratio, .direction = props.direction,
            .style = props.style);
}
#define Widget_Split(...) cel_init(WSplitPane, __VA_ARGS__)

CEL_Composition(WScrollContainer, int height; int total_count; int scroll_offset;
                 const Widget_ScrollableStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_scrollable_layout);
    cel_has(W_ScrollContainer, .height = props.height, .style = props.style);
    cel_has(W_Scrollable, .scroll_offset = props.scroll_offset,
            .total_count = props.total_count,
            .visible_count = props.height);
}
#define Widget_Scrollable(...) cel_init(WScrollContainer, __VA_ARGS__)

/* ============================================================================
 * Radio Compositions
 * ============================================================================ */

CEL_Composition(WRadioButton, const char* label; bool selected; int group_id; bool disabled;
                 const Widget_RadioButtonStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_radio_button_layout);
    cel_has(W_RadioButton, .label = props.label, .group_id = props.group_id, .style = props.style);
    cel_has(W_Selectable, .selected = props.selected);
    cel_has(W_InteractState, .selected = props.selected, .disabled = props.disabled);
}
#define Widget_RadioButton(...) cel_init(WRadioButton, __VA_ARGS__)

CEL_Composition(WRadioGroup, int group_id; int selected_index; int count;
                 const Widget_RadioGroupStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_radio_group_layout);
    cel_has(W_RadioGroup, .group_id = props.group_id,
            .selected_index = props.selected_index, .count = props.count,
            .style = props.style);
}
#define Widget_RadioGroup(...) cel_init(WRadioGroup, __VA_ARGS__)

/* ============================================================================
 * Navigation Compositions
 * ============================================================================ */

CEL_Composition(WTabBar, int active; int count; const char** labels;
                 const Widget_TabBarStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_tab_bar_layout);
    cel_has(W_TabBar, .active = props.active, .count = props.count,
            .labels = props.labels, .style = props.style);
}
#define Widget_TabBar(...) cel_init(WTabBar, __VA_ARGS__)

CEL_Composition(WTabContent, const char* text; const char* hint;
                 const Widget_TabContentStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_tab_content_layout);
    cel_has(W_TabContent, .text = props.text, .hint = props.hint, .style = props.style);
}
#define Widget_TabContent(...) cel_init(WTabContent, __VA_ARGS__)

CEL_Composition(WStatusBar, const char* left; const char* right;
                 const Widget_StatusBarStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_status_bar_layout);
    cel_has(W_StatusBar, .left = props.left, .right = props.right, .style = props.style);
}
#define Widget_StatusBar(...) cel_init(WStatusBar, __VA_ARGS__)

/* ============================================================================
 * List Compositions
 * ============================================================================ */

CEL_Composition(WListView, int item_count; int selected_index; int scroll_offset;
                 int visible_count; const Widget_ListViewStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_list_view_layout);
    cel_has(W_ListView, .item_count = props.item_count,
            .selected_index = props.selected_index, .style = props.style);
    cel_has(W_Scrollable, .scroll_offset = props.scroll_offset,
            .total_count = props.item_count,
            .visible_count = props.visible_count);
}
#define Widget_ListView(...) cel_init(WListView, __VA_ARGS__)

CEL_Composition(WListItem, const char* label; bool selected; void* data; bool disabled;
                 const Widget_ListItemStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_list_item_layout);
    cel_has(W_ListItem, .label = props.label, .data = props.data, .style = props.style);
    cel_has(W_Selectable, .selected = props.selected);
    cel_has(W_InteractState, .selected = props.selected, .disabled = props.disabled);
}
#define Widget_ListItem(...) cel_init(WListItem, __VA_ARGS__)

/* ============================================================================
 * Text Input Compositions
 * ============================================================================ */

CEL_Composition(WTextInput, const char* placeholder; bool password;
                 int max_length; bool selected; bool disabled;
                 void (*on_change)(const char* text);
                 void (*on_submit)(const char* text);
                 const Widget_TextInputStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_text_input_layout);
    cel_has(W_TextInput, .placeholder = props.placeholder,
            .password = props.password,
            .max_length = props.max_length,
            .on_change = props.on_change, .on_submit = props.on_submit,
            .style = props.style);
    cel_has(W_TextInputBuffer); /* Zero-init; behavioral system inits once */
    cel_has(W_Selectable, .selected = props.selected);
    cel_has(W_InteractState, .selected = props.selected,
            .disabled = props.disabled);
    cel_has(W_Focusable);
}
#define Widget_TextInput(...) cel_init(WTextInput, __VA_ARGS__)

/* ============================================================================
 * Navigation Group Compositions
 * ============================================================================ */

CEL_Composition(WNavigationGroup,
    bool wrap; int direction; /* 0=vertical, 1=horizontal */
) {
    cel_has(ClayUI, .layout_fn = w_navigation_group_layout);
    cel_has(W_NavigationScope, .wrap = props.wrap, .direction = props.direction);
    cel_has(W_Focusable);
}
#define Widget_NavigationGroup(...) cel_init(WNavigationGroup, __VA_ARGS__)

/* ============================================================================
 * Overlay Compositions
 * ============================================================================ */

CEL_Composition(WPopup, const char* title; bool visible; bool backdrop; int width; int height;
                 const Widget_PopupStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_popup_layout);
    cel_has(W_Popup, .title = props.title, .visible = props.visible,
            .backdrop = props.backdrop,
            .width = props.width > 0 ? props.width : 40,
            .height = props.height, .style = props.style);
    cel_has(W_OverlayState, .visible = props.visible,
            .z_index = 100, .modal = false);
}
#define Widget_Popup(...) cel_init(WPopup, __VA_ARGS__)

CEL_Composition(WModal, const char* title; bool visible; int width; int height;
                 void (*on_dismiss)(void); const Widget_ModalStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_modal_layout);
    cel_has(W_Modal, .title = props.title, .visible = props.visible,
            .width = props.width > 0 ? props.width : 50,
            .height = props.height,
            .on_dismiss = props.on_dismiss, .style = props.style);
    cel_has(W_OverlayState, .visible = props.visible,
            .z_index = 200, .modal = true);
    cel_has(W_NavigationScope, .wrap = true, .direction = 0);
    cel_has(W_Focusable);
}
#define Widget_Modal(...) cel_init(WModal, __VA_ARGS__)

CEL_Composition(WWindow, const char* title; bool visible; int x; int y;
                 int width; int height; int z_order; bool draggable;
                 void (*on_close)(void); const Widget_WindowStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_window_layout);
    cel_has(W_Window, .title = props.title, .visible = props.visible,
            .x = props.x, .y = props.y,
            .width = props.width > 0 ? props.width : 40,
            .height = props.height,
            .z_order = props.z_order,
            .on_close = props.on_close, .style = props.style);
    cel_has(W_OverlayState, .visible = props.visible,
            .z_index = 150 + props.z_order, .modal = true);
    cel_has(W_NavigationScope, .wrap = true, .direction = 0);
    cel_has(W_Focusable);
    if (props.draggable) { cel_has(W_Draggable); }
}
#define Widget_Window(...) cel_init(WWindow, __VA_ARGS__)

CEL_Composition(WToast, const char* message; float duration; int severity; int position;
                 const Widget_ToastStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_toast_layout);
    cel_has(W_Toast, .message = props.message,
            .duration = props.duration > 0 ? props.duration : 3.0f,
            .elapsed = 0, .severity = props.severity,
            .position = props.position, .dismissed = false,
            .style = props.style);
    cel_has(W_OverlayState, .visible = true,
            .z_index = 300, .modal = false);
}
#define Widget_Toast(...) cel_init(WToast, __VA_ARGS__)

/* ============================================================================
 * Data Visualization Compositions
 * ============================================================================ */

CEL_Composition(WSpark, const float* values; int count;
                 float min; float max; bool has_min; bool has_max;
                 const Widget_SparkStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_spark_layout);
    cel_has(W_Spark, .values = props.values, .count = props.count,
            .min = props.min, .max = props.max,
            .has_min = props.has_min, .has_max = props.has_max,
            .style = props.style);
}
#define Widget_Spark(...) cel_init(WSpark, __VA_ARGS__)

CEL_Composition(WBarChart, const W_BarChartEntry* entries; int count;
                 float max_value; bool gradient;
                 const Widget_BarChartStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_bar_chart_layout);
    cel_has(W_BarChart, .entries = props.entries, .count = props.count,
            .max_value = props.max_value, .gradient = props.gradient,
            .style = props.style);
}
#define Widget_BarChart(...) cel_init(WBarChart, __VA_ARGS__)

/* ============================================================================
 * Log Viewer Compositions
 * ============================================================================ */

CEL_Composition(WLogViewer, const W_LogEntry* entries; int entry_count;
                 int visible_height; int severity_filter; int scroll_offset;
                 const Widget_LogViewerStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_log_viewer_layout);
    /* visible_height: >0 = FIXED, <0 = GROW (fill parent), 0 = default (10) */
    cel_has(W_LogViewer, .entries = props.entries,
            .entry_count = props.entry_count,
            .visible_height = props.visible_height != 0 ? props.visible_height : 10,
            .severity_filter = props.severity_filter > 0 ? props.severity_filter : 0xF,
            .style = props.style);
    cel_has(W_Scrollable, .scroll_offset = props.scroll_offset,
            .total_count = props.entry_count,
            .visible_count = props.visible_height > 0 ? (props.visible_height - 2) :
                             props.visible_height < 0 ? props.entry_count :
                             8 /* default: 10 - 2 */);
    cel_has(W_LogViewerState); /* Zero-init; layout inits once */
}
#define Widget_LogViewer(...) cel_init(WLogViewer, __VA_ARGS__)

/* ============================================================================
 * Powerline Compositions
 * ============================================================================ */

CEL_Composition(WPowerline, const W_PowerlineSegment* segments; int segment_count;
                 int separator_style; const Widget_PowerlineStyle* style;) {
    cel_has(ClayUI, .layout_fn = w_powerline_layout);
    cel_has(W_Powerline, .segments = props.segments,
            .segment_count = props.segment_count,
            .separator_style = props.separator_style,
            .style = props.style);
}
#define Widget_Powerline(...) cel_init(WPowerline, __VA_ARGS__)

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
#define WCollapsible(...) Widget_Collapsible(__VA_ARGS__)
#define WSplitPane(...)   Widget_Split(__VA_ARGS__)
#define WScrollContainer(...) Widget_Scrollable(__VA_ARGS__)
#define WPopup(...)       Widget_Popup(__VA_ARGS__)
#define WModal(...)       Widget_Modal(__VA_ARGS__)
#define WWindow(...)      Widget_Window(__VA_ARGS__)
#define WToast(...)       Widget_Toast(__VA_ARGS__)
#define WTextInput(...)   Widget_TextInput(__VA_ARGS__)
#define WSpark(...)       Widget_Spark(__VA_ARGS__)
#define WBarChart(...)    Widget_BarChart(__VA_ARGS__)
#define WPowerline(...)   Widget_Powerline(__VA_ARGS__)
#define WLogViewer(...)   Widget_LogViewer(__VA_ARGS__)

#endif /* CELS_WIDGETS_COMPOSITIONS_H */
