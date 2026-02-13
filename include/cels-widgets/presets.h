/*
 * CELS Widgets - Style Preset Library
 *
 * Named style presets for common widget visual patterns. Each preset is
 * a `static const Widget_*Style` struct assigned via the `.style` pointer:
 *
 *   Widget_Button(.label = "Delete", .style = &W_BUTTON_DANGER) {}
 *
 * Design principles:
 *   - All presets use `static const` for zero-allocation compile-time values
 *   - Naming: W_WIDGET_VARIANT (screaming snake case)
 *   - Theme-aware: most fields zero-initialized ({0,0,0,0} / alpha=0)
 *     so they inherit from the active theme via the alpha sentinel
 *   - Only override fields that define the VARIANT
 *   - Presets work correctly across all 5 built-in themes
 *
 * Requires: #include <cels-widgets/style.h> before this header
 */

#ifndef CELS_WIDGETS_PRESETS_H
#define CELS_WIDGETS_PRESETS_H

#include <cels-widgets/style.h>

/* ============================================================================
 * Button Presets (5)
 * ============================================================================ */

/* Danger: red background when selected, red border -- destructive actions */
static const Widget_ButtonStyle W_BUTTON_DANGER = {
    .bg_selected  = { 180, 40, 40, 255 },
    .fg_selected  = { 255, 255, 255, 255 },
    .border_color = { 220, 80, 80, 255 },
};

/* Ghost: transparent background, text-only, no border */
static const Widget_ButtonStyle W_BUTTON_GHOST = {
    .bg          = { 0, 0, 0, 1 },   /* alpha=1 sentinel: transparent */
    .bg_selected = { 0, 0, 0, 1 },
    .border      = CEL_BORDER_NONE,
};

/* Primary: accent-colored background when selected */
static const Widget_ButtonStyle W_BUTTON_PRIMARY = {
    .bg_selected = { 100, 210, 220, 255 },
    .fg_selected = { 30, 33, 45, 255 },
};

/* Outline: border always visible, no background fill */
static const Widget_ButtonStyle W_BUTTON_OUTLINE = {
    .bg     = { 0, 0, 0, 1 },   /* transparent */
    .border = CEL_BORDER_ALWAYS,
};

/* Compact: fixed small width for tight layouts */
static const Widget_ButtonStyle W_BUTTON_COMPACT = {
    .width = { .mode = CEL_SIZE_FIXED, .value = 16 },
};

/* ============================================================================
 * Panel Presets (3)
 * ============================================================================ */

/* Card: rounded border always visible -- raised card appearance */
static const Widget_PanelStyle W_PANEL_CARD = {
    .border       = CEL_BORDER_ALWAYS,
    .border_style = CEL_BORDERSTYLE_ROUNDED,
};

/* Flat: no border, relies on background contrast */
static const Widget_PanelStyle W_PANEL_FLAT = {
    .border = CEL_BORDER_NONE,
};

/* Inset: dark recessed background, no border */
static const Widget_PanelStyle W_PANEL_INSET = {
    .bg     = { 20, 22, 35, 255 },
    .border = CEL_BORDER_NONE,
};

/* ============================================================================
 * Badge Presets (4)
 * ============================================================================ */

/* Success: green badge */
static const Widget_BadgeStyle W_BADGE_SUCCESS = {
    .badge_color = { 80, 200, 100, 255 },
};

/* Warning: yellow badge */
static const Widget_BadgeStyle W_BADGE_WARNING = {
    .badge_color = { 220, 180, 60, 255 },
};

/* Error: red badge */
static const Widget_BadgeStyle W_BADGE_ERROR = {
    .badge_color = { 220, 80, 80, 255 },
};

/* Info: blue badge */
static const Widget_BadgeStyle W_BADGE_INFO = {
    .badge_color = { 100, 200, 240, 255 },
};

/* ============================================================================
 * Text Presets (3)
 * ============================================================================ */

/* Heading: bold text */
static const Widget_TextStyle W_TEXT_HEADING = {
    .text_attr = { .bold = true },
};

/* Muted: dim secondary text */
static const Widget_TextStyle W_TEXT_MUTED = {
    .text_attr = { .dim = true },
};

/* Code: underline for monospace emphasis */
static const Widget_TextStyle W_TEXT_CODE = {
    .text_attr = { .underline = true },
};

/* ============================================================================
 * Divider Presets (2)
 * ============================================================================ */

/* Bold: prominent separator */
static const Widget_DividerStyle W_DIVIDER_BOLD = {
    .text_attr = { .bold = true },
};

/* Dim: subtle separator */
static const Widget_DividerStyle W_DIVIDER_DIM = {
    .text_attr = { .dim = true },
};

/* ============================================================================
 * ProgressBar Presets (2)
 * ============================================================================ */

/* Success: green fill */
static const Widget_ProgressBarStyle W_PROGRESS_SUCCESS = {
    .fill_color = { 80, 200, 100, 255 },
};

/* Danger: red fill */
static const Widget_ProgressBarStyle W_PROGRESS_DANGER = {
    .fill_color = { 220, 80, 80, 255 },
};

/* ============================================================================
 * TextInput Presets (2)
 * ============================================================================ */

/* Search: compact input with rounded border for search bars */
static const Widget_TextInputStyle W_INPUT_SEARCH = {
    .border       = CEL_BORDER_ALWAYS,
    .border_style = CEL_BORDERSTYLE_ROUNDED,
};

/* Secure: bordered input for password fields (visual only, .password controls masking) */
static const Widget_TextInputStyle W_INPUT_SECURE = {
    .border = CEL_BORDER_ALWAYS,
};

/* ============================================================================
 * Metric Presets (2)
 * ============================================================================ */

/* Success: green metric value */
static const Widget_MetricStyle W_METRIC_SUCCESS = {
    .fg = { 80, 200, 100, 255 },
};

/* Danger: red metric value */
static const Widget_MetricStyle W_METRIC_DANGER = {
    .fg = { 220, 80, 80, 255 },
};

/* ============================================================================
 * Table Preset (1)
 * ============================================================================ */

/* Striped: always-visible border */
static const Widget_TableStyle W_TABLE_STRIPED = {
    .border = CEL_BORDER_ALWAYS,
};

/* ============================================================================
 * TabBar Preset
 *
 * Note: W_TAB_POWERLINE is defined in style.h (powerline-styled tabs).
 * ============================================================================ */

/* ============================================================================
 * Collapsible Preset (1)
 * ============================================================================ */

/* Minimal: no border, clean tree appearance */
static const Widget_CollapsibleStyle W_COLLAPSIBLE_MINIMAL = {
    .border = CEL_BORDER_NONE,
};

/* ============================================================================
 * Spark Preset (1)
 * ============================================================================ */

/* Accent: use primary accent color for sparkline */
static const Widget_SparkStyle W_SPARK_ACCENT = {
    .spark_color = { 100, 210, 220, 255 },
};

/* ============================================================================
 * BarChart Preset (1)
 * ============================================================================ */

/* Monochrome: single-color bars (no gradient) using accent color */
static const Widget_BarChartStyle W_BARCHART_MONO = {
    .bar_color = { 100, 210, 220, 255 },
};

#endif /* CELS_WIDGETS_PRESETS_H */
