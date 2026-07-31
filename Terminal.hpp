#ifndef __TERMINAL_HPP__
#define __TERMINAL_HPP__

#include "PTY.hpp"
#include "TerminalState.hpp"
#include "AnsiParser.hpp"

#include <string>
#include <deque>
#include <chrono>

class Terminal {
public:
    Terminal(int cols, int rows);
    ~Terminal() = default;

    bool spawn(const std::string& shell = "/bin/bash");
    void close();

    // Process output from PTY and update the grid
    void update();

    // Send input to PTY (keyboard)
    void sendInput(const std::string& data);

    // Send output directly to parser (e.g., from cout logs)
    void writeOutput(const std::string& data);

    // Resize the terminal grid and notify PTY
    void resize(int cols, int rows, int width_px, int height_px);

    // Accessors
    TerminalState& getState() { return m_state; }
    PTY& getPTY() { return m_pty; }
    
    int getCols() const { return m_cols; }
    int getRows() const { return m_rows; }
    int getWidthPx() const { return m_width_px; }
    int getHeightPx() const { return m_height_px; }

private:
    int m_cols;
    int m_rows;
    int m_width_px;
    int m_height_px;

    PTY m_pty;
    TerminalState m_state;
    AnsiParser m_parser;
    
    char m_read_buffer[4096];
    
    bool m_shell_ready = false;
    std::deque<std::string> m_queued_logs;

    std::string m_last_written_data;
    std::chrono::time_point<std::chrono::steady_clock> m_last_write_time;
};

#endif // __TERMINAL_HPP__
