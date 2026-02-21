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
 * CELS Widgets - Module registration
 *
 * Registers the Widgets module with CELS and ensures all widget component
 * types are registered on init. Clay layout functions are provided by
 * layouts.c; the generic Clay renderer handles all visual output.
 */

#include <cels-widgets/widgets.h>
#include <cels-widgets/input.h>
#include <cels-layout/compositions.h>

/* ============================================================================
 * Powerline Glyph State
 * ============================================================================ */

static bool s_powerline_glyphs = false;

void Widget_set_powerline_glyphs(bool enabled) {
    s_powerline_glyphs = enabled;
}

bool Widget_powerline_glyphs_enabled(void) {
    return s_powerline_glyphs;
}

/* ============================================================================
 * Module Definition
 * ============================================================================ */

cel_module(Widgets) {
    cel_module_provides(UI);
    cel_module_provides(Widgets);

    /* Ensure all widget component types are registered */
    cel_register(W_Text);
    cel_register(W_Hint);
    cel_register(W_Canvas);
    cel_register(W_InfoBox);
    cel_register(W_Badge);
    cel_register(W_TextArea);
    cel_register(W_Button);
    cel_register(W_Slider);
    cel_register(W_Toggle);
    cel_register(W_Cycle);
    cel_register(W_ProgressBar);
    cel_register(W_Metric);
    cel_register(W_Panel);
    cel_register(W_Divider);
    cel_register(W_Table);
    cel_register(W_Collapsible);
    cel_register(W_SplitPane);
    cel_register(W_ScrollContainer);
    cel_register(W_RadioButton);
    cel_register(W_RadioGroup);
    cel_register(W_TabBar);
    cel_register(W_TabContent);
    cel_register(W_StatusBar);
    cel_register(W_ListView);
    cel_register(W_ListItem);
    cel_register(W_Focusable);
    cel_register(W_InteractState);

    /* Behavioral components */
    cel_register(W_Selectable);
    cel_register(W_RangeValueF);
    cel_register(W_RangeValueI);
    cel_register(W_Scrollable);
    cel_register(W_NavigationScope);

    /* Overlay components */
    cel_register(W_OverlayState);
    cel_register(W_Toast);
    cel_register(W_Popup);
    cel_register(W_Modal);
    cel_register(W_Window);
    cel_register(W_Draggable);

    /* Powerline components */
    cel_register(W_Powerline);

    /* Layout config components (from cels-layout) */
    Layout_StackConfig_register();
    Layout_CenterConfig_register();

    /* Register focus system */
    widgets_focus_system_register();

    /* Register behavioral systems (RangeClamp, ScrollClamp) */
    widgets_behavioral_systems_register();
}
