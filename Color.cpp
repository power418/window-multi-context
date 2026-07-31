#include "Color.hpp"

// Default ANSI Colors
const uint32_t Color::Black = 0x000000;
const uint32_t Color::Red = 0xCD0000;
const uint32_t Color::Green = 0x00CD00;
const uint32_t Color::Yellow = 0xCDCD00;
const uint32_t Color::Blue = 0x0000EE;
const uint32_t Color::Magenta = 0xCD00CD;
const uint32_t Color::Cyan = 0x00CDCD;
const uint32_t Color::White = 0xE5E5E5;

const uint32_t Color::BrightBlack = 0x7F7F7F;
const uint32_t Color::BrightRed = 0xFF0000;
const uint32_t Color::BrightGreen = 0x00FF00;
const uint32_t Color::BrightYellow = 0xFFFF00;
const uint32_t Color::BrightBlue = 0x5C5CFF;
const uint32_t Color::BrightMagenta = 0xFF00FF;
const uint32_t Color::BrightCyan = 0x00FFFF;
const uint32_t Color::BrightWhite = 0xFFFFFF;

uint32_t Color::fromRGB(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}

uint32_t Color::fromHex(const std::string& hexCode) {
    if (hexCode.empty()) return 0;
    
    std::string hex = hexCode;
    if (hex[0] == '#') hex = hex.substr(1);
    
    if (hex.length() == 6) {
        return std::stoul(hex, nullptr, 16);
    }
    return 0;
}

uint32_t Color::fromAnsi(uint8_t index) {
    static const uint32_t table[16] = {
        Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
        BrightBlack, BrightRed, BrightGreen, BrightYellow, 
        BrightBlue, BrightMagenta, BrightCyan, BrightWhite
    };
    
    if (index < 16) {
        return table[index];
    }
    
    // For 256 colors, basic mapping
    if (index >= 16 && index <= 231) {
        index -= 16;
        uint8_t r = (index / 36) * 51;
        uint8_t g = ((index / 6) % 6) * 51;
        uint8_t b = (index % 6) * 51;
        return fromRGB(r, g, b);
    }
    
    // Grayscale
    if (index >= 232 && index <= 255) {
        uint8_t gray = (index - 232) * 10 + 8;
        return fromRGB(gray, gray, gray);
    }
    
    return 0;
}
