/*
 * CELS Widgets - Module registration
 *
 * Registers the Widgets module with CELS and ensures all widget component
 * types are registered on init. Clay layout functions are provided by
 * layouts.c; the generic Clay renderer handles all visual output.
 */

#include <cels-widgets/widgets.h>
#include <cels-widgets/input.h>

/* ============================================================================
 * Module Definition
 * ============================================================================ */

CEL_Module(Widgets) {
    CEL_ModuleProvides(UI);
    CEL_ModuleProvides(Widgets);

    /* Ensure all widget component types are registered */
    W_Text_ensure();
    W_Hint_ensure();
    W_Canvas_ensure();
    W_InfoBox_ensure();
    W_Badge_ensure();
    W_TextArea_ensure();
    W_Button_ensure();
    W_Slider_ensure();
    W_Toggle_ensure();
    W_Cycle_ensure();
    W_ProgressBar_ensure();
    W_Metric_ensure();
    W_Panel_ensure();
    W_Divider_ensure();
    W_Table_ensure();
    W_Collapsible_ensure();
    W_SplitPane_ensure();
    W_ScrollContainer_ensure();
    W_RadioButton_ensure();
    W_RadioGroup_ensure();
    W_TabBar_ensure();
    W_TabContent_ensure();
    W_StatusBar_ensure();
    W_ListView_ensure();
    W_ListItem_ensure();
    W_Focusable_ensure();
    W_InteractState_ensure();

    /* Behavioral components */
    W_Selectable_ensure();
    W_RangeValueF_ensure();
    W_RangeValueI_ensure();
    W_Scrollable_ensure();
    W_NavigationScope_ensure();

    /* Overlay components */
    W_OverlayState_ensure();
    W_Toast_ensure();
    W_Popup_ensure();
    W_Modal_ensure();
    W_Window_ensure();

    /* Register focus system */
    widgets_focus_system_register();

    /* Register behavioral systems (RangeClamp, ScrollClamp) */
    widgets_behavioral_systems_register();
}
