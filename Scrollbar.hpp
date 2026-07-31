#ifndef __SCROLLBAR_HPP__
#define __SCROLLBAR_HPP__

#include <X11/Xlib.h>
#include <algorithm>
#include "UiHoverUtility.h"
#include "X11Rounded.hpp"

class Scrollbar {
public:
    Scrollbar() = default;
    
    void updateMouse(int mouse_x, int mouse_y) {
        m_mouse_x = mouse_x;
        m_mouse_y = mouse_y;
    }

    bool onMousePress(int x, int y, int button) {
        if (button == 1) { // Left click
            if (ui::HoverUtility::containsPoint(m_thumb_rect, x, y)) {
                m_is_dragging = true;
                m_drag_start_y = y;
                m_drag_start_offset = m_last_scroll_offset;
                return true;
            }
        }
        return false;
    }
    
    void onMouseRelease(int button) {
        if (button == 1) {
            m_is_dragging = false;
        }
    }
    
    bool onMouseMove(int x, int y, int& out_scroll_offset) {
        updateMouse(x, y);
        if (m_is_dragging && m_last_max_scroll > 0) {
            int delta_y = y - m_drag_start_y;
            float ratio = (float)delta_y / (float)m_last_track_h;
            int delta_scroll = - (int)(ratio * m_last_max_scroll);
            
            int new_offset = m_drag_start_offset + delta_scroll;
            if (new_offset < 0) new_offset = 0;
            if (new_offset > m_last_max_scroll) new_offset = m_last_max_scroll;
            
            out_scroll_offset = new_offset;
            return true; // We are dragging, so the mouse move is consumed by the scrollbar
        }
        return false;
    }
    
    // Render the scrollbar on the given pixmap
    // x, y, w, h define the scrollbar area
    // total_lines = history size + screen rows
    // screen_lines = screen rows
    // scroll_offset = how many lines we are scrolled up from the bottom (0 = bottom)
    void render(Display* display, Pixmap pixmap, GC gc, int x, int y, int w, int h,
                int total_lines, int screen_lines, int scroll_offset)
    {
        if (total_lines <= screen_lines || screen_lines <= 0) return;
        
        // Draw track background (very subtle or hidden)
        // Big Sur style usually hides the track until hovered, but since we disabled track hover,
        // we'll just draw a very subtle rounded track or leave it empty.
        // Let's draw a subtle rounded track.
        uint32_t track_color = 0x1A1A1A;
        XSetForeground(display, gc, track_color);
        ui::X11Rounded::fillRoundedRect(display, pixmap, gc, x + 2, y + 2, w - 4, h - 4, (w - 4)/2);
        
        // Calculate thumb size (proportional to visible lines)
        float thumb_ratio = (float)screen_lines / (float)total_lines;
        int thumb_h = (int)(thumb_ratio * h);
        if (thumb_h < 30) thumb_h = 30; // min height for grab ability
        
        // Offset mapping:
        // scroll_offset = 0 means thumb is at the very bottom
        // scroll_offset = max_scroll means thumb is at the very top
        int max_scroll = total_lines - screen_lines;
        float scroll_ratio = (float)scroll_offset / (float)max_scroll;
        
        // thumb_y: 0 means top, (h - thumb_h) means bottom
        int thumb_y = y + (h - thumb_h) - (int)(scroll_ratio * (h - thumb_h));
        
        // Add padding to make the thumb floating (pill-shaped)
        int pad = 4;
        int t_x = x + pad;
        int t_y = thumb_y + pad;
        int t_w = w - (pad * 2);
        int t_h = thumb_h - (pad * 2);
        
        // Save variables needed for dragging logic
        m_thumb_rect = { (float)x, (float)thumb_y, (float)(x + w), (float)(thumb_y + thumb_h) };
        m_last_scroll_offset = scroll_offset;
        m_last_max_scroll = max_scroll;
        m_last_track_h = h - thumb_h;
        if (m_last_track_h <= 0) m_last_track_h = 1;
        
        // Thumb hover & active state
        bool is_thumb_hovered = ui::HoverUtility::containsPoint(m_thumb_rect, m_mouse_x, m_mouse_y) || m_is_dragging;
        float thumb_hover = m_hover_tracker.update(2, is_thumb_hovered, 0.20f);
        
        // Draw thumb
        uint32_t thumb_base_color = 0x555555;
        uint32_t thumb_color = ui::HoverUtility::applyHover(thumb_base_color, thumb_hover);
        
        // If dragging, make it slightly brighter as active feedback
        if (m_is_dragging) {
            thumb_color = ui::HoverUtility::applyHover(thumb_color, 0.5f);
        }
        
        XSetForeground(display, gc, thumb_color);
        ui::X11Rounded::fillRoundedRect(display, pixmap, gc, t_x, t_y, t_w, t_h, t_w / 2);
    }

private:
    ui::HoverTracker m_hover_tracker;
    ui::Rect m_thumb_rect = {0,0,0,0};
    int m_mouse_x = -1;
    int m_mouse_y = -1;
    
    // Dragging state
    bool m_is_dragging = false;
    int m_drag_start_y = 0;
    int m_drag_start_offset = 0;
    int m_last_scroll_offset = 0;
    int m_last_max_scroll = 0;
    int m_last_track_h = 1;
};

#endif // __SCROLLBAR_HPP__