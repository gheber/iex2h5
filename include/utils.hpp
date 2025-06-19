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
#include <concepts>
#include <armadillo>
#include <date/date.h> 
#include "compat.hpp"
#include <bit> 

namespace utils {
	namespace ch = std::chrono;

	// ─────────────────────────────────────────────────────────────────────────────
	// Sequence generator for chrono durations
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename D>
	requires requires(D d) { D{1}; d + d; d <= d; }
	constexpr std::vector<D> sequence(D begin, D interval, D end) {
		std::vector<D> result;
		for (D current = begin + interval; current <= end; current += interval)
			result.push_back(current);
		return result;
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// Check if a value is "finite enough" (rounded to P decimal places)
	// ─────────────────────────────────────────────────────────────────────────────
	template <std::integral Int, std::floating_point T>
	constexpr bool is_finite(T value) {
		constexpr T factor = std::pow(10.0, static_cast<T>(Int{}));
		return std::ceil(value * factor) / factor > T{0.01};
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// Find diagonal finite elements in a matrix
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename T>
	requires std::floating_point<T>
	arma::uvec find_finite_diag(const arma::Mat<T>& Q) {
		std::vector<arma::uword> indices;
		indices.reserve(Q.n_rows);

		for (arma::uword i = 0; i < Q.n_rows; ++i)
			if (is_finite<4>(Q(i, i)))
				indices.push_back(i);

		return arma::uvec(indices);
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// Parse HH:MM:SS string to chrono duration
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename duration>
	duration string_to_duration(const std::string& time_str) {
		int h, m, s;
		char sep1, sep2;
		std::istringstream in(time_str);
		in >> h >> sep1 >> m >> sep2 >> s;

		if (!in || sep1 != ':' || sep2 != ':')
			throw std::runtime_error("Invalid time format: " + time_str);

		return ch::duration_cast<duration>(ch::hours{h} + ch::minutes{m} + ch::seconds{s});
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// Format chrono duration to HH:MM:SS string
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename duration>
	std::string duration_to_string(const duration& dur) {
		using namespace std::chrono;
		return date::format("%H:%M:%S", date::floor<seconds>(dur));
	}


	
} // namespace util

namespace utils::base40::impl {

    constexpr uint8_t SYMBOL_BASE = 40;
    constexpr size_t SYMBOL_WIDTH = 9;
    constexpr char SYMBOL_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_ ";

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
            : throw std::invalid_argument("Invalid base40 character: " + std::string(1, c));
    }

    inline char symbol_index_to_char(uint8_t index) {
        if (index >= SYMBOL_BASE)
            throw std::invalid_argument("Invalid base40 index: " + std::to_string(index));
        return SYMBOL_ALPHABET[index];
    }

    inline uint64_t encode(const std::string& symbol) {
        uint64_t encoded = 0;
        const size_t len = symbol.size();
        for (size_t i = 0; i < SYMBOL_WIDTH; ++i) {
            char c = (i < len) ? symbol[i] : ' ';
            encoded = encoded * SYMBOL_BASE + symbol_char_to_index(c);
        }
        return encoded;

    }

    inline uint64_t encode(uint64_t iex_symbol) {
        uint64_t encoded = 0;
        const char* chars = reinterpret_cast<const char*>(&iex_symbol);
        if constexpr (std::endian::native == std::endian::little) {
            for (int i = 7; i >= 0; --i)
                encoded = encoded * SYMBOL_BASE + symbol_char_to_index(chars[i]);
        } else {
            for (int i = 0; i < 8; ++i)
                encoded = encoded * SYMBOL_BASE + symbol_char_to_index(chars[i]);
        }
        return encoded;
    }

	inline uint64_t encode(std::string_view symbol) {
		if (symbol.size() > SYMBOL_WIDTH)
			throw std::invalid_argument("Symbol exceeds base40 encoding width (max 9 chars)");
	
		uint64_t encoded = 0;
		for (size_t i = 0; i < SYMBOL_WIDTH; ++i) {
			char c = (i < symbol.size()) ? symbol[i] : ' ';
			encoded = encoded * SYMBOL_BASE + symbol_char_to_index(c);
		}
		return encoded;
	}
	
    template<size_t N>
    inline uint64_t encode(const char (&symbol)[N]) {
        static_assert(N <= 10, "Symbol too long for base40 encoding");
        return encode(std::string_view(symbol, N - 1));
    }

    inline std::string decode(uint64_t encoded_symbol) {
        std::string result(SYMBOL_WIDTH, ' ');
        for (int i = SYMBOL_WIDTH - 1; i >= 0; --i) {
            result[i] = symbol_index_to_char(encoded_symbol % SYMBOL_BASE);
            encoded_symbol /= SYMBOL_BASE;
        }
        return result;
    }
}

namespace utils::base40 {

    constexpr uint64_t MAX_SYMBOL_BITS = (1ULL << 50) - 1;
    constexpr uint16_t MAX_INDEX = (1U << 14) - 1;
	inline uint64_t encode(uint64_t iex_symbol) {
		return impl::encode(iex_symbol) << 14;
	}

    inline uint64_t encode(uint64_t symbol, uint16_t index) {
        if (symbol > MAX_SYMBOL_BITS)
            throw std::invalid_argument("Encoded symbol exceeds 50 bits");
        if (index > MAX_INDEX)
            throw std::invalid_argument("Index exceeds 14-bit limit");
        return (symbol << 14) | index;
    }

    inline std::pair<std::string, uint16_t> decode(uint64_t encoded) {
        uint64_t symbol_bits = encoded >> 14;
        uint16_t index = encoded & 0x3FFF; // 14 bits
        return {impl::decode(symbol_bits), index};
    }
}
