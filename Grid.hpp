#ifndef __GRID_HPP__
#define __GRID_HPP__

#include "Cell.hpp"
#include <vector>
#include <deque>
#include <utility>

class Grid {
public:
    Grid(int cols, int rows);
    ~Grid() = default;

    void resize(int cols, int rows, int& cursor_x, int& cursor_y);
    
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    const Cell& getVisibleCell(int x, int y) const;
    
    void clear();
    void scrollUp();
    void scrollDown(); // For alternate screen or history insertion
    
    int getCols() const { return m_cols; }
    int getRows() const { return m_rows; }

    bool isWrapped(int y) const;
    void setWrapped(int y, bool wrapped);

    int getScrollOffset() const { return m_scroll_offset; }
    void setScrollOffset(int offset);
    int getHistorySize() const { return m_history.size(); }

private:
    int m_cols;
    int m_rows;
    std::vector<Cell> m_cells;
    std::vector<bool> m_row_wrapped;
    
    std::deque<std::pair<std::vector<Cell>, bool>> m_history;
    int m_max_history = 1000;
    int m_scroll_offset = 0;
};

#endif // __GRID_HPP__
