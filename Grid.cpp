#include "Grid.hpp"

Grid::Grid(int cols, int rows) : m_cols(cols), m_rows(rows) {
    if (m_cols < 1) m_cols = 1;
    if (m_rows < 1) m_rows = 1;
    m_cells.resize(m_cols * m_rows);
    m_row_wrapped.resize(m_rows, false);
}

void Grid::resize(int cols, int rows, int& cursor_x, int& cursor_y) {
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;
    if (cols == m_cols && rows == m_rows) return;
    
    // Extract logical lines
    std::vector<std::vector<Cell>> logical_lines;
    std::vector<Cell> current_line;
    
    int logical_line_idx = 0;
    int current_logical_len = 0;
    int c_logic_line = -1;
    int c_logic_offset = 0;
    
    for (const auto& hline : m_history) {
        logical_lines.push_back(hline.first);
        // Wait, m_history stores physical lines! We need to extract logical lines from m_history too.
    }
    // But since m_history stores physical lines just like m_cells, let's treat them all as a single contiguous array of rows!
    
    int total_old_rows = m_history.size() + m_rows;
    for (int y = 0; y < total_old_rows; ++y) {
        bool is_history = (y < (int)m_history.size());
        int local_y = is_history ? y : (y - m_history.size());
        
        int last_char_col = m_cols - 1;
        while (last_char_col >= 0) {
            char c = is_history ? m_history[local_y].first[last_char_col].character 
                                : m_cells[local_y * m_cols + last_char_col].character;
            if (c != ' ' && c != 0) break;
            last_char_col--;
        }
        
        bool wrapped = is_history ? m_history[local_y].second : m_row_wrapped[local_y];
        int copy_len = wrapped ? m_cols : (last_char_col + 1);
        
        if (!is_history && local_y == cursor_y) {
            c_logic_line = logical_line_idx;
            c_logic_offset = current_logical_len + cursor_x;
        }
        
        current_logical_len += copy_len;
        
        for (int x = 0; x < copy_len; ++x) {
            current_line.push_back(is_history ? m_history[local_y].first[x] 
                                              : m_cells[local_y * m_cols + x]);
        }
        
        if (!wrapped) {
            logical_lines.push_back(current_line);
            current_line.clear();
            logical_line_idx++;
            current_logical_len = 0;
        }
    }
    if (!current_line.empty()) {
        logical_lines.push_back(current_line);
        if (c_logic_line == -1 && cursor_y >= m_rows) {
            c_logic_line = logical_line_idx;
            c_logic_offset = cursor_x;
        }
    } else if (c_logic_line == -1) {
        c_logic_line = logical_line_idx;
        c_logic_offset = cursor_x;
    }
    
    // Reflow into new grid and new history
    std::deque<std::pair<std::vector<Cell>, bool>> new_history;
    std::vector<Cell> new_cells(cols * rows);
    std::vector<bool> new_wrapped(rows, false);
    
    int new_y = 0;
    int new_c_x = 0;
    int new_c_y = 0;
    int current_logic_line_idx = 0;
    
    // Calculate total new lines
    int total_new_lines = 0;
    for (const auto& line : logical_lines) {
        int len = line.size();
        if (len == 0) total_new_lines++;
        else total_new_lines += (len + cols - 1) / cols;
    }
    
    // We only keep the last `rows` lines in new_cells, the rest goes to new_history
    int lines_to_history = total_new_lines - rows;
    if (lines_to_history < 0) lines_to_history = 0;

    
    // We try to keep the bottom aligned, so we might need to skip top logical lines if they don't fit.
    // A better approach is to reflow from the top, and if it exceeds rows, we scroll up later.
    // For simplicity, we just reflow from top to bottom.
    
    for (const auto& line : logical_lines) {
        int chars_written = 0;
        int line_len = line.size();
        
        if (line_len == 0) {
            if (current_logic_line_idx == c_logic_line) {
                new_c_y = new_y - lines_to_history;
                new_c_x = c_logic_offset;
            }
            if (new_y < lines_to_history) {
                new_history.push_back({std::vector<Cell>(cols), false});
            } else {
                int ty = new_y - lines_to_history;
                new_wrapped[ty] = false;
            }
            new_y++;
            current_logic_line_idx++;
            continue;
        }
        
        while (chars_written < line_len) {
            int chunk = std::min(cols, line_len - chars_written);
            bool wrapped = (chars_written + chunk < line_len);
            
            if (current_logic_line_idx == c_logic_line) {
                if (c_logic_offset >= chars_written && c_logic_offset < chars_written + chunk) {
                    new_c_y = new_y - lines_to_history;
                    new_c_x = c_logic_offset - chars_written;
                } else if (c_logic_offset >= line_len && chars_written + chunk == line_len) {
                    new_c_y = new_y - lines_to_history;
                    new_c_x = c_logic_offset - chars_written;
                }
            }
            
            if (new_y < lines_to_history) {
                std::vector<Cell> hline(cols);
                for (int x = 0; x < chunk; ++x) {
                    hline[x] = line[chars_written + x];
                }
                new_history.push_back({hline, wrapped});
            } else {
                int ty = new_y - lines_to_history;
                if (ty < rows) {
                    for (int x = 0; x < chunk; ++x) {
                        new_cells[ty * cols + x] = line[chars_written + x];
                    }
                    new_wrapped[ty] = wrapped;
                }
            }
            
            chars_written += chunk;
            new_y++;
        }
        current_logic_line_idx++;
    }
    
    if (c_logic_line >= current_logic_line_idx) {
        new_c_y = new_y - lines_to_history;
        new_c_x = c_logic_offset;
    }
    
    if (new_history.size() > (size_t)m_max_history) {
        int excess = new_history.size() - m_max_history;
        for (int i=0; i<excess; i++) new_history.pop_front();
    }

    
    if (new_c_y < 0) new_c_y = 0;
    if (new_c_x < 0) new_c_x = 0;
    if (new_c_y >= rows) new_c_y = rows - 1;
    if (new_c_x >= cols) new_c_x = cols - 1;
    
    cursor_x = new_c_x;
    cursor_y = new_c_y;
    
    m_cols = cols;
    m_rows = rows;
    m_cells = std::move(new_cells);
    m_row_wrapped = std::move(new_wrapped);
    m_history = std::move(new_history);
    
    if (m_scroll_offset > (int)m_history.size()) {
        m_scroll_offset = m_history.size();
    }
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

const Cell& Grid::getVisibleCell(int x, int y) const {
    if (x < 0) x = 0;
    if (x >= m_cols) x = m_cols - 1;
    
    int eff_y = y - m_scroll_offset;
    if (eff_y < 0) {
        int hist_idx = m_history.size() + eff_y;
        if (hist_idx < 0) hist_idx = 0;
        if (hist_idx >= (int)m_history.size()) hist_idx = m_history.size() - 1;
        return m_history[hist_idx].first[x];
    } else {
        if (eff_y >= m_rows) eff_y = m_rows - 1;
        return m_cells[eff_y * m_cols + x];
    }
}

void Grid::clear() {
    for (auto& cell : m_cells) {
        cell = Cell(); // Reset to default
    }
    for (int y = 0; y < m_rows; ++y) {
        m_row_wrapped[y] = false;
    }
    m_history.clear();
    m_scroll_offset = 0;
}

void Grid::scrollUp() {
    // Push top row to history
    std::vector<Cell> top_row(m_cols);
    for (int x = 0; x < m_cols; ++x) {
        top_row[x] = m_cells[x];
    }
    m_history.push_back({top_row, m_row_wrapped[0]});
    if (m_history.size() > (size_t)m_max_history) {
        m_history.pop_front();
    }
    
    if (m_scroll_offset > 0) {
        m_scroll_offset++;
        if (m_scroll_offset > (int)m_history.size()) {
            m_scroll_offset = m_history.size();
        }
    }
    
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

void Grid::setScrollOffset(int offset) {
    if (offset < 0) offset = 0;
    if (offset > (int)m_history.size()) offset = m_history.size();
    m_scroll_offset = offset;
}
