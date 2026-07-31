#include "Terminal.hpp"

Terminal::Terminal(int cols, int rows) 
    : m_cols(cols), m_rows(rows), m_width_px(0), m_height_px(0),
      m_state(cols, rows), m_parser(&m_state) {
}

bool Terminal::spawn(const std::string& shell) {
    TerminalSize size{m_cols, m_rows, m_width_px, m_height_px};
    m_shell_ready = false;
    m_queued_logs.clear();
    return m_pty.spawn(shell, size);
}

void Terminal::close() {
    m_pty.close();
}

void Terminal::update() {
    int bytes_read = m_pty.read(m_read_buffer, sizeof(m_read_buffer));
    if (bytes_read > 0) {
        m_parser.parse(m_read_buffer, bytes_read);
        
        if (!m_shell_ready) {
            std::string out(m_read_buffer, bytes_read);
            if (out.find('@') != std::string::npos || out.find('$') != std::string::npos || out.find('%') != std::string::npos) {
                m_shell_ready = true;
                
                // Print all queued logs now that bash is ready
                for (const auto& log : m_queued_logs) {
                    m_parser.parse(log.c_str(), log.size());
                }
                m_queued_logs.clear();
            }
        }
    }
}

void Terminal::sendInput(const std::string& data) {
    m_pty.write(data);
}

void Terminal::writeOutput(const std::string& data) {
    if (!data.empty()) {
        // Prevent double rendering of identical consecutive requests from client
        // Only deduplicate if the string has actual content (length > 2) to avoid eating multiple newlines
        if (data.length() > 2 && data == m_last_written_data) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_write_time).count() < 100) {
                return; // Ignore exact same message sent within 100ms
            }
        }
        m_last_written_data = data;
        m_last_write_time = std::chrono::steady_clock::now();

        if (!m_shell_ready && m_pty.getChildPID() > 0) {
            m_queued_logs.push_back(data);
        } else {
            m_parser.parse(data.c_str(), data.size());
        }
    }
}

void Terminal::resize(int cols, int rows, int width_px, int height_px) {
    bool cols_rows_changed = (m_cols != cols || m_rows != rows);

    m_cols = cols;
    m_rows = rows;
    m_width_px = width_px;
    m_height_px = height_px;

    if (cols_rows_changed) {
        // Update internal state grid
        m_state.resize(cols, rows);

        // Notify PTY only if grid size changed to prevent infinite prompt redraws
        TerminalSize size{cols, rows, width_px, height_px};
        m_pty.resize(size);
    }
}
