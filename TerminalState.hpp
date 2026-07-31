#ifndef __TERMINAL_STATE_HPP__
#define __TERMINAL_STATE_HPP__

#include "Grid.hpp"
#include <cstdint>
#include <string>

class TerminalState {
public:
    TerminalState(int cols, int rows);
    ~TerminalState() = default;

    Grid& getGrid() { return m_grid; }
    
    // Core drawing
    void printChar(char32_t c);
    void newLine();
    void carriageReturn();
    void backspace();
    
    // Cursor control
    void moveCursor(int dx, int dy);
    void setCursorPos(int col, int row);
    
    // SGR (Select Graphic Rendition) - colors and styles
    void setForegroundColor(uint32_t color);
    void setBackgroundColor(uint32_t color);
    void resetAttributes();
    
    // Erase commands
    void eraseInDisplay(int mode);
    void eraseInLine(int mode);
    
    // Getters
    int getCursorX() const { return m_cursor_x; }
    int getCursorY() const { return m_cursor_y; }
    
    void resize(int cols, int rows);

private:
    Grid m_grid;
    int m_cursor_x = 0;
    int m_cursor_y = 0;
    
    // Current pen attributes
    uint32_t m_current_fg = 0xE5E5E5;
    uint32_t m_current_bg = 0x000000;
    bool m_bold = false;
    
    void scrollIfNeeded();
};

#endif // __TERMINAL_STATE_HPP__
