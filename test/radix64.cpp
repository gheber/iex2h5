/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <error.hpp>
#include <io.hpp>
#include "radix64.hpp"
#include "mock.hpp"

//IEX symbol characters size:34 {
// ' ','#','*','+','-','.','=','^','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',}

TEST_CASE("radix64 round-trip string <-> symbol corner cases") {
    using namespace utils::radix64;

    const std::array<std::string, 6> symbols = {
        "ABCDEFGE",   // max-length, alphabetic
        "AB      ",   // padded with spaces
        "        ",   // all spaces
        "12345678",   // all digits
        "# *+-.=^",   // special characters
        "TEST    "    // padded to 8 chars
    };

    for (const std::string& symbol : symbols) {
        CAPTURE(symbol); // helpful in test output
        uint64_t packed = encode(symbol, 42);
        auto [decoded, idx] = decode(packed);
        
        CHECK(decoded.size() == symbol.size());
        CHECK(decoded == symbol);
        CHECK(idx == 42);
    }
}

TEST_CASE("radix64 round-trip actual symbols") {
    using namespace utils::radix64;
    uint16_t id = 0;
    static constexpr uint64_t SYMBOL_MASK = 0xFFFF'FFFF'FFFF'0000; // upper 48 bits
    static constexpr uint64_t CONTRACT_ID_MASK = 0xFFFF;           // lower 16 bit
    std::vector<uint64_t> flat_map;

    for(const auto&[iex, symbol]: mock::some::symbols){
        uint64_t radix64_encoded_symbol = utils::radix64::encode(iex, 0);
        auto [s,i] = utils::radix64::decode(radix64_encoded_symbol);
        if( auto it = std::ranges::lower_bound(flat_map, radix64_encoded_symbol); it != flat_map.end()) {
            auto [x,y] = utils::radix64::decode(*it);
            if((radix64_encoded_symbol & SYMBOL_MASK) == (*it & SYMBOL_MASK)){
                TRACE << "found:" << i << " <" << s << "> |" << y << "| [" << x << "]" << std::endl;
            } else {
                flat_map.insert(it, radix64_encoded_symbol | flat_map.size() );
                TRACE << "insert:" << i << " <" << s << "> |" << y << "| [" << x << "]" << std::endl;
            } 
        } else {
            TRACE << "insert:" << i << " <" << s << "> |" << symbol << "| [" << utils::iex_symbol(iex) << "]" << std::endl;
            flat_map.push_back(radix64_encoded_symbol | flat_map.size());
        }
        uint64_t packed = encode(symbol, id);
        auto [symbol_, id_] = decode(packed);
        CHECK(symbol_ == symbol);
        CHECK(id_ == id);
       id++;
    }
    
    for(auto iex: flat_map){
        auto[s,i] = utils::radix64::decode(iex);
        TRACE << i << " " << s << std::endl;
    }
}

TEST_CASE("radix64 encoding breaks lexicographic sort order") {
    using namespace utils::radix64;

    std::vector<uint64_t> original, sorted;
    std::vector<std::string> alphabetical, decoded_sorted;

    uint16_t id = 0;
    constexpr int j = 40;

    for (int i = 0; i < j; ++i) {
        const auto& [iex, symbol] = mock::all::symbols[i];
        original.push_back(encode(iex, id++));
        alphabetical.push_back(symbol);
    }

    sorted = original;
    std::ranges::sort(sorted);
    std::ranges::sort(alphabetical);

    decoded_sorted.reserve(j);
    for (int i = 0; i < j; ++i) {
        auto [symbol_sorted, _] = decode(sorted[i]);
        decoded_sorted.push_back(symbol_sorted);
    }

    bool mismatch_found = false;
    for (int i = 0; i < j; ++i) {
        auto [symbol_original, id_original] = decode(original[i]);
        auto [symbol_sorted, id_sorted]     = decode(sorted[i]);

        TRACE << "[" << i << "] " << alphabetical[i] << " | " << symbol_original << " (" << id_original << ")"
              << " | sorted → " << symbol_sorted << " (" << id_sorted << ")" << std::endl;

        if (symbol_sorted != alphabetical[i]) mismatch_found = true;
    }

    CHECK(mismatch_found);
    CHECK(original != sorted);
    CHECK(decoded_sorted != alphabetical);
}

TEST_CASE("radix64-encoded symbol sorting leads to binary search mismatch") {
    using namespace utils::radix64;

    std::vector<uint64_t> original, sorted;
    auto alphabetical = mock::random::symbols;

    uint16_t id = 0;
    const size_t j = alphabetical.size();

    for (const auto& symbol : alphabetical)
        original.push_back(utils::radix64::encode(symbol, id++));

    sorted = original;
    std::ranges::sort(sorted);
    std::ranges::sort(alphabetical);

    std::cerr << "|index|original|encoded|roundtrip|sorted|decoded sorted|" << std::endl;
    for (size_t i = 0; i < j; i++) {
        auto [decoded_orig, id_orig] = decode(original[i]);
        auto [decoded_sorted, id_sorted] = decode(sorted[i]);

        std::cerr << "[" << i << "] " << alphabetical[i] << " 0x" << std::hex << original[i] << std::dec
            << " | {" << decoded_orig << " " << id_orig << "} | " << decoded_sorted << " " << id_sorted << std::endl;
        CHECK(alphabetical[i] == decoded_sorted);
    }

    // Binary search mismatch test
    TRACE << "\nSearching for 'OCEAN   ' in sorted radix64-encoded vector:\n";
    uint64_t ocean_encoded = encode("OCEAN   ", 0);
    if (auto it = std::ranges::lower_bound(sorted, ocean_encoded); it != sorted.end()) {
        auto [symbol, index] = decode(*it);
        TRACE << "position: " << std::distance(sorted.begin(), it)
              << " <" << symbol << "> {" << index << "}" << std::endl;
        CHECK(symbol == "OCEAN   ");  // This proves mismatch
    }
}
