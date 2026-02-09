/*
 * CELS Widgets - Module registration
 *
 * Registers the Widgets module with CELS and ensures all widget component
 * types are registered on init. Backends link against cels-widgets and
 * provide renderers for these components via the feature/provider system.
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
    W_Button_ensure();
    W_Slider_ensure();
    W_Toggle_ensure();
    W_Cycle_ensure();
    W_RadioButton_ensure();
    W_RadioGroup_ensure();
    W_TabBar_ensure();
    W_TabContent_ensure();
    W_StatusBar_ensure();
    W_ListView_ensure();
    W_ListItem_ensure();
    W_Focusable_ensure();

    /* Register focus system */
    widgets_focus_system_register();
}
