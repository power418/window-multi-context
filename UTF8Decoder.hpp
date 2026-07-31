#ifndef __UTF8_DECODER_HPP__
#define __UTF8_DECODER_HPP__

#include <cstdint>

class UTF8Decoder {
public:
    UTF8Decoder();
    ~UTF8Decoder() = default;

    // Decodes one byte. Returns the complete codepoint when finished, 
    // or 0 if it's waiting for more bytes.
    char32_t decode(uint8_t byte);
    
    void reset();

private:
    uint32_t m_state;
    char32_t m_codepoint;
};

#endif // __UTF8_DECODER_HPP__
