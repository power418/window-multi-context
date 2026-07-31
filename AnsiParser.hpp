#ifndef __ANSI_PARSER_HPP__
#define __ANSI_PARSER_HPP__

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

#include "UTF8Decoder.hpp"

// Forward declaration
class TerminalState;

class AnsiParser {
public:
    enum class State {
        Ground,
        Escape,
        CSI_Entry,
        CSI_Param,
        OSC_String
    };

    AnsiParser(TerminalState* state);
    ~AnsiParser() = default;

    // Parse incoming stream from PTY
    void parse(const char* data, size_t length);

private:
    TerminalState* m_state;
    UTF8Decoder m_utf8_decoder;
    State m_parse_state = State::Ground;
    
    std::vector<int> m_params;
    int m_current_param = 0;
    std::string m_osc_string;

    void processGround(char c);
    void processEscape(char c);
    void processCSIEntry(char c);
    void processCSIParam(char c);
    
    void executeCSI(char final_char);
    
    // Helpers
    void clearParams();
    void pushParam();
};

#endif // __ANSI_PARSER_HPP__
