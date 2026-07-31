#pragma once
#include <X11/Xlib.h>

namespace ui {

class X11Rounded {
public:
    /**
     * @brief Draws a filled rounded rectangle using standard X11 drawing primitives.
     * @param r The corner radius. If r >= w/2 or r >= h/2, it becomes pill-shaped.
     */
    static void fillRoundedRect(Display* display, Drawable d, GC gc, int x, int y, int w, int h, int r) {
        if (w <= 0 || h <= 0) return;
        if (r <= 0) {
            XFillRectangle(display, d, gc, x, y, w, h);
            return;
        }
        
        // Clamp radius to prevent overlapping
        if (r * 2 > w) r = w / 2;
        if (r * 2 > h) r = h / 2;
        
        // Draw the main body using 3 rectangles
        XFillRectangle(display, d, gc, x + r, y, w - 2 * r, r);         // Top edge
        XFillRectangle(display, d, gc, x, y + r, w, h - 2 * r);         // Center block
        XFillRectangle(display, d, gc, x + r, y + h - r, w - 2 * r, r); // Bottom edge
        
        // Draw the 4 corner arcs
        // Angles are in units of 1/64th of a degree. Positive is counter-clockwise.
        // 0 degrees is 3 o'clock.
        XFillArc(display, d, gc, x, y, r * 2, r * 2, 90 * 64, 90 * 64);                 // Top-left
        XFillArc(display, d, gc, x + w - 2 * r, y, r * 2, r * 2, 0 * 64, 90 * 64);      // Top-right
        XFillArc(display, d, gc, x, y + h - 2 * r, r * 2, r * 2, 180 * 64, 90 * 64);    // Bottom-left
        XFillArc(display, d, gc, x + w - 2 * r, y + h - 2 * r, r * 2, r * 2, 270 * 64, 90 * 64); // Bottom-right
    }
};

} // namespace ui
