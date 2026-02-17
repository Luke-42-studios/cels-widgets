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

CEL_Module(Widgets) {
    CEL_ModuleProvides(UI);
    CEL_ModuleProvides(Widgets);

    /* Ensure all widget component types are registered */
    CEL_Register(W_Text);
    CEL_Register(W_Hint);
    CEL_Register(W_Canvas);
    CEL_Register(W_InfoBox);
    CEL_Register(W_Badge);
    CEL_Register(W_TextArea);
    CEL_Register(W_Button);
    CEL_Register(W_Slider);
    CEL_Register(W_Toggle);
    CEL_Register(W_Cycle);
    CEL_Register(W_ProgressBar);
    CEL_Register(W_Metric);
    CEL_Register(W_Panel);
    CEL_Register(W_Divider);
    CEL_Register(W_Table);
    CEL_Register(W_Collapsible);
    CEL_Register(W_SplitPane);
    CEL_Register(W_ScrollContainer);
    CEL_Register(W_RadioButton);
    CEL_Register(W_RadioGroup);
    CEL_Register(W_TabBar);
    CEL_Register(W_TabContent);
    CEL_Register(W_StatusBar);
    CEL_Register(W_ListView);
    CEL_Register(W_ListItem);
    CEL_Register(W_Focusable);
    CEL_Register(W_InteractState);

    /* Behavioral components */
    CEL_Register(W_Selectable);
    CEL_Register(W_RangeValueF);
    CEL_Register(W_RangeValueI);
    CEL_Register(W_Scrollable);
    CEL_Register(W_NavigationScope);

    /* Overlay components */
    CEL_Register(W_OverlayState);
    CEL_Register(W_Toast);
    CEL_Register(W_Popup);
    CEL_Register(W_Modal);
    CEL_Register(W_Window);
    CEL_Register(W_Draggable);

    /* Powerline components */
    CEL_Register(W_Powerline);

    /* Layout config components (from cels-layout) */
    Layout_StackConfig_register();
    Layout_CenterConfig_register();

    /* Register focus system */
    widgets_focus_system_register();

    /* Register behavioral systems (RangeClamp, ScrollClamp) */
    widgets_behavioral_systems_register();
}
