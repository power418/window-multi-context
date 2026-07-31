#include "AnsiParser.hpp"
#include "TerminalState.hpp"
#include "Color.hpp"

AnsiParser::AnsiParser(TerminalState* state) : m_state(state) {
}

void AnsiParser::parse(const char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        char c = data[i];
        
        switch (m_parse_state) {
            case State::Ground:
                processGround(c);
                break;
            case State::Escape:
                processEscape(c);
                break;
            case State::CSI_Entry:
                processCSIEntry(c);
                break;
            case State::CSI_Param:
                processCSIParam(c);
                break;
            case State::OSC_String:
                // Handle OSC string
                if (c == '\x07' || c == '\x1b') {
                    m_parse_state = State::Ground;
                } else {
                    m_osc_string += c;
                }
                break;
        }
    }
}

void AnsiParser::processGround(char c) {
    if (c == '\x1b') {
        m_parse_state = State::Escape;
    } else if (c == '\n') {
        m_state->newLine();
    } else if (c == '\r') {
        m_state->carriageReturn();
    } else if (c == '\b') {
        m_state->backspace();
    } else if (c >= 32 || c < 0) { // printable ascii + utf8 continuation bytes
        char32_t codepoint = m_utf8_decoder.decode(static_cast<uint8_t>(c));
        if (codepoint != 0) {
            m_state->printChar(codepoint);
        }
    }
}

void AnsiParser::processEscape(char c) {
    if (c == '[') {
        clearParams();
        m_parse_state = State::CSI_Entry;
    } else if (c == ']') {
        m_osc_string.clear();
        m_parse_state = State::OSC_String;
    } else {
        // Unhandled escape, return to ground
        m_parse_state = State::Ground;
    }
}

void AnsiParser::processCSIEntry(char c) {
    if (c >= '0' && c <= '9') {
        m_current_param = c - '0';
        m_parse_state = State::CSI_Param;
    } else if (c >= 0x40 && c <= 0x7E) {
        pushParam();
        executeCSI(c);
        m_parse_state = State::Ground;
    } else {
        // Unknown, ignore
    }
}

void AnsiParser::processCSIParam(char c) {
    if (c >= '0' && c <= '9') {
        m_current_param = m_current_param * 10 + (c - '0');
    } else if (c == ';') {
        pushParam();
    } else if (c >= 0x40 && c <= 0x7E) {
        pushParam();
        executeCSI(c);
        m_parse_state = State::Ground;
    }
}

void AnsiParser::clearParams() {
    m_params.clear();
    m_current_param = 0;
}

void AnsiParser::pushParam() {
    m_params.push_back(m_current_param);
    m_current_param = 0;
}

void AnsiParser::executeCSI(char final_char) {
    switch (final_char) {
        case 'A': // CUU (Cursor Up)
            m_state->moveCursor(0, - (m_params.empty() || m_params[0] == 0 ? 1 : m_params[0]));
            break;
        case 'B': // CUD (Cursor Down)
            m_state->moveCursor(0, (m_params.empty() || m_params[0] == 0 ? 1 : m_params[0]));
            break;
        case 'C': // CUF (Cursor Forward)
            m_state->moveCursor((m_params.empty() || m_params[0] == 0 ? 1 : m_params[0]), 0);
            break;
        case 'D': // CUB (Cursor Back)
            m_state->moveCursor(- (m_params.empty() || m_params[0] == 0 ? 1 : m_params[0]), 0);
            break;
        case 'H': // CUP (Cursor Position)
        case 'f': // HVP (Horizontal and Vertical Position)
        {
            int row = (m_params.size() > 0 && m_params[0] > 0) ? m_params[0] : 1;
            int col = (m_params.size() > 1 && m_params[1] > 0) ? m_params[1] : 1;
            m_state->setCursorPos(col, row);
            break;
        }
        case 'J': // ED (Erase in Display)
            m_state->eraseInDisplay(m_params.empty() ? 0 : m_params[0]);
            break;
        case 'K': // EL (Erase in Line)
            m_state->eraseInLine(m_params.empty() ? 0 : m_params[0]);
            break;
        case 'm': // SGR (Select Graphic Rendition)
        {
            if (m_params.empty()) {
                m_state->resetAttributes();
            } else {
                for (size_t i = 0; i < m_params.size(); ++i) {
                    int p = m_params[i];
                    if (p == 0) m_state->resetAttributes();
                    else if (p >= 30 && p <= 37) {
                        m_state->setForegroundColor(Color::fromAnsi(p - 30));
                    }
                    else if (p >= 40 && p <= 47) {
                        m_state->setBackgroundColor(Color::fromAnsi(p - 40));
                    }
                    else if (p >= 90 && p <= 97) {
                        m_state->setForegroundColor(Color::fromAnsi(p - 90 + 8)); // Bright FG
                    }
                    else if (p >= 100 && p <= 107) {
                        m_state->setBackgroundColor(Color::fromAnsi(p - 100 + 8)); // Bright BG
                    }
                }
            }
            break;
        }
        default:
            // std::cout << "Unhandled CSI: " << final_char << std::endl;
            break;
    }
}
