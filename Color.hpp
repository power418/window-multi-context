#ifndef __COLOR_HPP__
#define __COLOR_HPP__

#include <cstdint>
#include <string>

class Color {
public:
    static uint32_t fromRGB(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t fromHex(const std::string& hexCode);
    static uint32_t fromAnsi(uint8_t index);
    
    // Standard Terminal Colors (Default Theme)
    static const uint32_t Black;
    static const uint32_t Red;
    static const uint32_t Green;
    static const uint32_t Yellow;
    static const uint32_t Blue;
    static const uint32_t Magenta;
    static const uint32_t Cyan;
    static const uint32_t White;
    
    static const uint32_t BrightBlack;
    static const uint32_t BrightRed;
    static const uint32_t BrightGreen;
    static const uint32_t BrightYellow;
    static const uint32_t BrightBlue;
    static const uint32_t BrightMagenta;
    static const uint32_t BrightCyan;
    static const uint32_t BrightWhite;
};

#endif // __COLOR_HPP__
