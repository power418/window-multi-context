#include "TerminalState.hpp"

TerminalState::TerminalState(int cols, int rows) : m_grid(cols, rows) {
}

void TerminalState::printChar(char32_t c) {
    if (m_cursor_x >= m_grid.getCols()) {
        m_grid.setWrapped(m_cursor_y, true);
        m_cursor_x = 0;
        m_cursor_y++;
        scrollIfNeeded();
    }
    
    Cell& cell = m_grid.getCell(m_cursor_x, m_cursor_y);
    cell.character = c;
    cell.fg_color = m_current_fg;
    cell.bg_color = m_current_bg;
    cell.is_bold = m_bold;
    
    m_cursor_x++;
}

void TerminalState::newLine() {
    m_grid.setWrapped(m_cursor_y, false);
    m_cursor_y++;
    scrollIfNeeded();
}

void TerminalState::carriageReturn() {
    m_grid.setWrapped(m_cursor_y, false);
    m_cursor_x = 0;
}

void TerminalState::backspace() {
    if (m_cursor_x > 0) {
        m_cursor_x--;
    }
}

void TerminalState::moveCursor(int dx, int dy) {
    m_cursor_x += dx;
    m_cursor_y += dy;
    
    if (m_cursor_x < 0) m_cursor_x = 0;
    if (m_cursor_y < 0) m_cursor_y = 0;
    if (m_cursor_x >= m_grid.getCols()) m_cursor_x = m_grid.getCols() - 1;
    if (m_cursor_y >= m_grid.getRows()) m_cursor_y = m_grid.getRows() - 1;
}

void TerminalState::setCursorPos(int col, int row) {
    m_cursor_x = col - 1; // ANSI is 1-based, we are 0-based
    m_cursor_y = row - 1;
    
    if (m_cursor_x < 0) m_cursor_x = 0;
    if (m_cursor_y < 0) m_cursor_y = 0;
    if (m_cursor_x >= m_grid.getCols()) m_cursor_x = m_grid.getCols() - 1;
    if (m_cursor_y >= m_grid.getRows()) m_cursor_y = m_grid.getRows() - 1;
}

void TerminalState::setForegroundColor(uint32_t color) {
    m_current_fg = color;
}

void TerminalState::setBackgroundColor(uint32_t color) {
    m_current_bg = color;
}

void TerminalState::resetAttributes() {
    m_current_fg = 0xE5E5E5;
    m_current_bg = 0x000000;
    m_bold = false;
}

void TerminalState::eraseInDisplay(int mode) {
    // 0: below, 1: above, 2: all
    if (mode == 2) {
        m_grid.clear();
        m_cursor_x = 0;
        m_cursor_y = 0;
    }
    // TODO: implement mode 0 and 1
}

void TerminalState::eraseInLine(int mode) {
    // 0: to right, 1: to left, 2: all
    if (mode == 2) {
        for (int x = 0; x < m_grid.getCols(); ++x) {
            m_grid.getCell(x, m_cursor_y) = Cell();
        }
    } else if (mode == 0) {
        for (int x = m_cursor_x; x < m_grid.getCols(); ++x) {
            m_grid.getCell(x, m_cursor_y) = Cell();
        }
    } else if (mode == 1) {
        for (int x = 0; x <= m_cursor_x; ++x) {
            m_grid.getCell(x, m_cursor_y) = Cell();
        }
    }
}

void TerminalState::resize(int cols, int rows) {
    m_grid.resize(cols, rows);
    if (m_cursor_x >= cols) m_cursor_x = cols - 1;
    if (m_cursor_y >= rows) m_cursor_y = rows - 1;
}

void TerminalState::scrollIfNeeded() {
    if (m_cursor_y >= m_grid.getRows()) {
        m_grid.scrollUp();
        m_cursor_y = m_grid.getRows() - 1;
    }
}
