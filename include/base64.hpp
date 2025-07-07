/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca
 *
 * NOTE:
 *   - Implements fixed-width base64 encoding for stock symbols.
 *   - Encodes up to 8-character symbols into 48 bits using a 64-character alphabet.
 *   - The remaining 16 bits in a uint64_t are reserved for a symbol index (0–65536).
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>

namespace utils::base64::impl {
    constexpr uint8_t SYMBOL_BASE = 64;
    constexpr size_t SYMBOL_WIDTH = 8;
    constexpr std::array<char, 64> SYMBOL_ALPHABET = {
        '0','1','2','3','4','5','6','7','8','9',
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z', // 10 + 26 
        ' ','#','*','+','-','.','=','^',
        '!','$','&',',','/',':',';','<','>','?','@',
        '{','|','}','[',']','_','(',')',  '\0'};
    
    constexpr std::array<int8_t, 128> build_lookup() {
        std::array<int8_t, 128> table{};
        for (int i = 0; i < 128; ++i) table[i] = -1;
        for (uint8_t i = 0; i < SYMBOL_BASE; ++i)
            table[static_cast<size_t>(SYMBOL_ALPHABET[i])] = i;
        return table;
    }
    constexpr auto CHAR_TO_INDEX = build_lookup();

    constexpr uint8_t symbol_char_to_index(char c) {
        return (c >= 0 && c < 128 && CHAR_TO_INDEX[c] != -1)
            ? static_cast<uint8_t>(CHAR_TO_INDEX[c])
            : throw std::invalid_argument( fmt_compat::format("invalid base64 symbol char {}", c ) );
    }

    inline uint64_t encode(uint64_t raw_symbol) {
        constexpr uint8_t terminator_index = symbol_char_to_index('\0');
        uint64_t result = 0;
        for (size_t i = 0; i < SYMBOL_WIDTH; ++i) {
            size_t shift = i * 8;
            char c = static_cast<char>((raw_symbol >> shift) & 0xFF);
            if (c == '\0')
                result = result * SYMBOL_BASE + terminator_index;
            else result = result * SYMBOL_BASE + symbol_char_to_index(c);
        }
        return result;
    }

    inline uint64_t encode(const std::string& symbol) {
        if (symbol.size() > SYMBOL_WIDTH)
            throw std::invalid_argument("symbol too long for base64 encoding");
        uint64_t raw_symbol = 0;
        std::memcpy(&raw_symbol, symbol.data(), symbol.size());
        return encode(raw_symbol);
    }

    inline std::string decode(uint64_t encoded) {
        std::string result(SYMBOL_WIDTH, ' ');
        for (size_t i = 0; i < SYMBOL_WIDTH; ++i) {
            uint8_t index = encoded % SYMBOL_BASE;
            result[SYMBOL_WIDTH - i -1] = SYMBOL_ALPHABET[index];
            encoded /= SYMBOL_BASE;
        }
        return result;
    }
} // namespace utils::base64::impl

namespace utils::base64 {
    using impl::SYMBOL_WIDTH;
 
    inline uint64_t encode(const std::string& symbol, uint16_t index) {
        return (impl::encode(symbol) << 16) | (index & 0xFFFF);
    }

    inline uint64_t encode(const std::string& symbol) {
        return (impl::encode(symbol) << 16);
    }

    inline uint64_t encode(uint64_t iex_symbol, uint16_t index) {
        return (impl::encode(iex_symbol) << 16) | (index & 0xFFFF);
    }

    inline uint64_t encode(uint64_t iex_symbol) {
        return (impl::encode(iex_symbol) << 16);
    }

    inline std::pair<std::string, uint16_t> decode(uint64_t encoded) {
        uint64_t symbol_bits = encoded >> 16;
        uint16_t index = encoded & 0xFFFF;
        return {impl::decode(symbol_bits), index};
    }
} // namespace utils::base64
 