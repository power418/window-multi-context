#ifndef __CELL_HPP__
#define __CELL_HPP__

#include <cstdint>

struct Cell {
    char32_t character = ' ';
    uint32_t fg_color = 0xE5E5E5; // Default white
    uint32_t bg_color = 0x000000; // Default black
    bool is_bold = false;
    bool is_underline = false;
    bool is_inverse = false;

    bool operator==(const Cell& other) const {
        return character == other.character &&
               fg_color == other.fg_color &&
               bg_color == other.bg_color &&
               is_bold == other.is_bold &&
               is_underline == other.is_underline &&
               is_inverse == other.is_inverse;
    }
    
    bool operator!=(const Cell& other) const {
        return !(*this == other);
    }
};

#endif // __CELL_HPP__
