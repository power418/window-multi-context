#include "Grid.hpp"

Grid::Grid(int cols, int rows) : m_cols(cols), m_rows(rows) {
    if (m_cols < 1) m_cols = 1;
    if (m_rows < 1) m_rows = 1;
    m_cells.resize(m_cols * m_rows);
    m_row_wrapped.resize(m_rows, false);
}

void Grid::resize(int cols, int rows) {
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;
    if (cols == m_cols && rows == m_rows) return;
    
    // Extract logical lines
    std::vector<std::vector<Cell>> logical_lines;
    std::vector<Cell> current_line;
    
    for (int y = 0; y < m_rows; ++y) {
        // Find last non-empty character in the row
        int last_char_col = m_cols - 1;
        while (last_char_col >= 0 && m_cells[y * m_cols + last_char_col].character == ' ') {
            last_char_col--;
        }
        
        // If the line is wrapped, we must include all characters up to m_cols.
        // If not wrapped, we only include up to the last non-empty character.
        int copy_len = m_row_wrapped[y] ? m_cols : (last_char_col + 1);
        
        for (int x = 0; x < copy_len; ++x) {
            current_line.push_back(m_cells[y * m_cols + x]);
        }
        
        if (!m_row_wrapped[y]) {
            logical_lines.push_back(current_line);
            current_line.clear();
        }
    }
    if (!current_line.empty()) {
        logical_lines.push_back(current_line);
    }
    
    // Reflow into new grid
    std::vector<Cell> new_cells(cols * rows);
    std::vector<bool> new_wrapped(rows, false);
    
    int new_y = 0;
    
    // We try to keep the bottom aligned, so we might need to skip top logical lines if they don't fit.
    // A better approach is to reflow from the top, and if it exceeds rows, we scroll up later.
    // For simplicity, we just reflow from top to bottom.
    
    for (const auto& line : logical_lines) {
        int chars_written = 0;
        int line_len = line.size();
        
        if (line_len == 0) {
            new_y++;
            continue;
        }
        
        while (chars_written < line_len) {
            int chunk = std::min(cols, line_len - chars_written);
            
            if (new_y >= rows) {
                // Scroll up one line in the new grid
                for (int ty = 1; ty < rows; ++ty) {
                    for (int tx = 0; tx < cols; ++tx) {
                        new_cells[(ty - 1) * cols + tx] = new_cells[ty * cols + tx];
                    }
                    new_wrapped[ty - 1] = new_wrapped[ty];
                }
                for (int tx = 0; tx < cols; ++tx) {
                    new_cells[(rows - 1) * cols + tx] = Cell();
                }
                new_wrapped[rows - 1] = false;
                new_y = rows - 1;
            }
            
            for (int x = 0; x < chunk; ++x) {
                new_cells[new_y * cols + x] = line[chars_written + x];
            }
            
            chars_written += chunk;
            
            if (chars_written < line_len) {
                new_wrapped[new_y] = true;
                new_y++;
            } else {
                new_wrapped[new_y] = false;
                new_y++;
            }
        }
    }
    
    m_cols = cols;
    m_rows = rows;
    m_cells = std::move(new_cells);
    m_row_wrapped = std::move(new_wrapped);
}

Cell& Grid::getCell(int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= m_cols) x = m_cols - 1;
    if (y >= m_rows) y = m_rows - 1;
    return m_cells[y * m_cols + x];
}

const Cell& Grid::getCell(int x, int y) const {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= m_cols) x = m_cols - 1;
    if (y >= m_rows) y = m_rows - 1;
    return m_cells[y * m_cols + x];
}

void Grid::clear() {
    for (auto& cell : m_cells) {
        cell = Cell(); // Reset to default
    }
    for (int y = 0; y < m_rows; ++y) {
        m_row_wrapped[y] = false;
    }
}

void Grid::scrollUp() {
    // Move all rows up by 1
    for (int y = 1; y < m_rows; ++y) {
        for (int x = 0; x < m_cols; ++x) {
            m_cells[(y - 1) * m_cols + x] = m_cells[y * m_cols + x];
        }
        m_row_wrapped[y - 1] = m_row_wrapped[y];
    }
    // Clear the bottom row
    for (int x = 0; x < m_cols; ++x) {
        m_cells[(m_rows - 1) * m_cols + x] = Cell();
    }
    m_row_wrapped[m_rows - 1] = false;
}

void Grid::scrollDown() {
    // Move all rows down by 1
    for (int y = m_rows - 2; y >= 0; --y) {
        for (int x = 0; x < m_cols; ++x) {
            m_cells[(y + 1) * m_cols + x] = m_cells[y * m_cols + x];
        }
        m_row_wrapped[y + 1] = m_row_wrapped[y];
    }
    // Clear the top row
    for (int x = 0; x < m_cols; ++x) {
        m_cells[x] = Cell();
    }
    m_row_wrapped[0] = false;
}

bool Grid::isWrapped(int y) const {
    if (y < 0 || y >= m_rows) return false;
    return m_row_wrapped[y];
}

void Grid::setWrapped(int y, bool wrapped) {
    if (y >= 0 && y < m_rows) {
        m_row_wrapped[y] = wrapped;
    }
}
