/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <cstdio>
#include <concepts>
#include <bit> 
#include <armadillo>
#include <date/date.h> 
#include <compat.hpp>
#include <filesystem>
#include <regex>
#include <type_traits>
#include <unordered_set>

namespace utils::pcap {
    inline constexpr uint32_t MAGIC_NATIVE_USEC = 0xa1b2c3d4;
    inline constexpr uint32_t MAGIC_NATIVE_NSEC = 0xa1b23c4d;
    inline constexpr uint32_t MAGIC_SWAP_USEC = 0xd4c3b2a1;
    inline constexpr uint32_t MAGIC_SWAP_NSEC = 0x4d3cb2a1;

    inline bool is_little_endian(uint32_t magic) {
        if (std::endian::native == std::endian::little){
            return (magic == 0xa1b2c3d4 || magic == 0xa1b23c4d);
        } else return (magic == 0xd4c3b2a1 || magic == 0x4d3cb2a1); 
    }

    inline bool is_big_endian(uint32_t magic) {
        if (std::endian::native == std::endian::big){
            return (magic == 0xa1b2c3d4 || magic == 0xa1b23c4d);
        } else return (magic == 0xd4c3b2a1 || magic == 0x4d3cb2a1); 
    } 

    inline bool is_native_byte_order(uint32_t magic) {
        return magic == 0xa1b2c3d4 || magic == 0xa1b23c4d;
    }

    inline bool needs_byteswap(uint32_t magic) {
        return !is_native_byte_order(magic);
    }

    inline bool is_valid_magic(uint32_t magic) {
        return magic == MAGIC_NATIVE_USEC || magic == MAGIC_NATIVE_NSEC ||
            magic == MAGIC_SWAP_USEC || magic == MAGIC_SWAP_NSEC;
    }
} // namespace utils::pcap
