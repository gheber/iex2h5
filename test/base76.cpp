/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <error.hpp>
#include <io.hpp>
#include <base76.hpp>
#include "mock.hpp"

TEST_CASE("base76 round-trip string <-> symbol corner cases 01") {
    using namespace utils::base76;
    std::pair<uint64_t, std::string> iex[] = {{2314885530822135622,"FOX     "},
                        {2314885530822133076,"TEX     "},{2314885531560070729,"INTL    "}};
    for(auto [iex_symbol, symbol]: iex ) {
        uint64_t base76_encoded_symbol = utils::base76::encode(iex_symbol);
        auto[symbol_, id_] = utils::base76::decode(base76_encoded_symbol);
    }
}

TEST_CASE("base76 round-trip string <-> symbol corner cases") {
    using namespace utils::base76;
    std::string symbols[] = {"ABCDEFGE","AB      ","        "};
    for(const std::string& symbol: symbols) {
        uint64_t packed = encode(symbol, 42);
        auto [decoded, idx] = decode(packed);
        CHECK(decoded.size() == symbol.size());
        CHECK(decoded == symbol);
        CHECK(idx == 42);
    }
}

TEST_CASE("string order") {
    std::string str ="1234567890";
    uint64_t data;
    std::memcpy(&data, str.data(), 8);
    char * ptr = str.data();
    for(int i=0; i<8; i++)
        TRACE << str[i] << " " << ptr[i] << " " << ((char*)&data)[i]<< std::endl;
}

TEST_CASE("base76 round-trip actual symbols") {
    using namespace utils::base76;
    uint16_t id = 0;
    static constexpr uint64_t SYMBOL_MASK = 0xFFFF'FFFF'FFFF'C000; // ~uint64_t{0x3FFF}; // upper 50 bits
    static constexpr uint64_t CONTRACT_ID_MASK = 0x3FFF;       // lower 14 bit
    std::vector<uint64_t> flat_map;

    for(const auto&[iex, symbol]: mock::some::symbols){
        //uint64_t base76_encoded_symbol = utils::base76::encode(iex, flat_map.size());
        uint64_t base76_encoded_symbol = utils::base76::encode(iex, 0);
        auto [s,i] = utils::base76::decode(base76_encoded_symbol);
        //TRACE << i << " <" << s << "> |" << symbol << "| [" << utils::iex_symbol(iex) << "]" << std::endl;
        if( auto it = std::ranges::lower_bound(flat_map, base76_encoded_symbol); it != flat_map.end()) {
            auto [x,y] = utils::base76::decode(*it);
            if((base76_encoded_symbol & SYMBOL_MASK) == (*it & SYMBOL_MASK)){
                TRACE << "found:" << i << " <" << s << "> |" << y << "| [" << x << "]" << std::endl;
            } else {
                flat_map.insert(it, base76_encoded_symbol | flat_map.size() );
                TRACE << "insert:" << i << " <" << s << "> |" << y << "| [" << x << "]" << std::endl;
            } 
        } else {
            TRACE << "insert:" << i << " <" << s << "> |" << symbol << "| [" << utils::iex_symbol(iex) << "]" << std::endl;
            flat_map.push_back(base76_encoded_symbol | flat_map.size());
        }
        /*
        uint64_t packed = encode(symbol, id);
        auto [symbol_, id_] = decode(packed);
        CHECK(symbol_ == symbol);
        CHECK(id_ == id);
        
        std::string padded = utils::pad(  utils::trim(utils::iex_symbol(iex_id)), 8);
        CHECK(padded == symbol);
        id++;
        */
    }
    
    for(auto iex: flat_map){
        auto[s,i] = utils::base76::decode(iex);
        TRACE << i << " " << s << std::endl;
    }
}
/*


TEST_CASE("demonstration of incorrect sorting of base76 encoded symbols") {
    using namespace utils::base76;
    std::vector<uint64_t> original, sorted;
    std::vector<std::string> alphabetical;
    uint16_t id = 0, j = 40;
    for(int i=0; i< j; i++) {
        const auto&[iex, symbol] = mock::all::symbols[i];
        original.push_back(utils::base76::encode(iex, id++));
        alphabetical.push_back(symbol);
    }
    sorted = original;
    std::ranges::sort(sorted);
    std::ranges::sort(alphabetical);
    
    for(int i=0; i< j; i++){
        auto [symbol, id] = utils::base76::decode(original[i]);
        auto [symbol_, id_] = utils::base76::decode(sorted[i]);
        TRACE << "["<< i << "]" << alphabetical[i] << " | "  << symbol << " " << id << " |(sorted) " << symbol_ << " " << id_ << std::endl;
    }
}
*/



TEST_CASE("demonstration of incorrect sorting of base76 encoded symbols") {
    using namespace utils::base76;
    std::vector<uint64_t> original, sorted;
    auto alphabetical = mock::random::symbols;
    
    uint16_t id = 0, j = mock::random::symbols.size();
    for(auto symbol: mock::random::symbols)
        original.push_back(utils::base76::encode(symbol, id++));
    
    sorted = original;
    std::ranges::sort(sorted);
    std::ranges::sort(alphabetical);
    
    std::cerr << 
        "|index|original|encoded|roundtrip|sorted original|encoded->sorted->decoded|" << std::endl;
    for(int i=0; i< j; i++) {
        auto [symbol, id]  = utils::base76::decode(original[i]);
        auto [symbol_, id_] = utils::base76::decode(sorted[i]);
        std::cerr << "["<< i << "]" << mock::random::symbols[i] << " 0x" << std::hex << original[i] << std::dec << " | {" <<  symbol << " " << id << "} |" 
        << alphabetical[i] << " | " 
        <<  " | " << symbol_ << " " << id_ << std::endl;
    }

    TRACE << "\n\nwith binary search finding element within the vector of encoded and sorted elements: " <<std::endl;
    for(int i=0; i<sorted.size() && sorted[i] <  utils::base76::encode("ocean   "); i++) {
        auto [symbol, index] = utils::base76::decode(sorted[i]);
        std::cout << i << " " << index << " " << symbol << std::endl;
    }

    TRACE << "\n\nwith binary search finding element within the vector of encoded and sorted elements: " <<std::endl;
    if (auto it = std::ranges::lower_bound(sorted, utils::base76::encode("ocean   ")); it != sorted.end()){
        auto [symbol, index] = utils::base76::decode(*it);
        TRACE << "position: " <<  std::distance(sorted.begin(), it) << " " << symbol << "{"<<index<<"}" << std::endl;
    } else TRACE << "not found...." <<std::endl;

    TRACE << "\n\nwith binary search finding element within the vector of encoded and sorted elements: " <<std::endl;
    sorted.clear();
    uint64_t base76_encoded_symbol = utils::base76::encode("orca    ");
    if (auto it = std::ranges::lower_bound(sorted, base76_encoded_symbol); it != sorted.end()){
        auto [symbol, index] = utils::base76::decode(*it);
        TRACE << "position: " <<  std::distance(sorted.begin(), it) << " " << symbol << "{"<<index<<"}" << std::endl;
        sorted.insert(it,
            base76_encoded_symbol | (sorted.size() & 0x3FFF));
    } else sorted.push_back(base76_encoded_symbol | (sorted.size() & 0x3FFF));

    for(int i=0; i< sorted.size(); i++) {
        auto [symbol, id]  = utils::base76::decode(sorted[i]);
        std::cerr << "["<< i << "]" << " | {" <<  symbol << " " << id << "}" << std::endl;
    }
}

