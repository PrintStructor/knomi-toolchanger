# Temp Graph Fix (Optional / Nice to Have)

Fixes the temp graph view so the progress ring/arc no longer covers the labels.

---

## When You Need This
- You switch to the temp graph view and still see the colorful ring or black arc on top of the temperature labels.
- You want the temp graph view to show only the chart + labels, with the progress visuals hidden.

---

## What It Changes
- Adds a real `ui_progress_enable()` implementation so progress updates can be disabled while the temp graph is visible.
- Uses `ui_get_progress_enabled()` inside `update_print_progress()` to skip ring/arc updates when progress is disabled.
- Raises the temp graph labels to the top of the LVGL object stack so text is never obscured.

---

## Files Touched
- `src/lvgl_usr.cpp` (progress enable flag + z-index adjustments for the temp view)
- `src/ui_overlay/lv_print_progress_update.cpp` (reads `ui_get_progress_enabled()` before drawing)

---

## Expected Result
- Temp graph view: black background, only the chart and two labels visible, no ring/arc artifacts.
- Progress view: unchanged behavior when `ui_progress_enable(true)` is active.

---

## Quick Verification
- Switch to the temp graph view: ring/arc stays hidden and labels stay on top.
- Serial monitor shows `[Progress] Updates DISABLED` when entering the temp graph view.
