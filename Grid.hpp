#ifndef __GRID_HPP__
#define __GRID_HPP__

#include "Cell.hpp"
#include <vector>

class Grid {
public:
    Grid(int cols, int rows);
    ~Grid() = default;

    void resize(int cols, int rows);
    
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    
    void clear();
    void scrollUp();
    void scrollDown(); // For alternate screen or history insertion
    
    int getCols() const { return m_cols; }
    int getRows() const { return m_rows; }

    bool isWrapped(int y) const;
    void setWrapped(int y, bool wrapped);

private:
    int m_cols;
    int m_rows;
    std::vector<Cell> m_cells;
    std::vector<bool> m_row_wrapped;
};

#endif // __GRID_HPP__
