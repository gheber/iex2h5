/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once
#include <sys/types.h>
#include <set>
#include <cstdint>
#include <string>
#include <iostream>
#include <chrono>
#include <date/date.h>
#include <atomic>
#include <error.hpp>

#include <generics.hpp>
#include <patterns.hpp>
#include <filters.hpp>
#include <compat.hpp>
#include <utils.hpp>
#include <base64.hpp>
#include <iex.hpp>
#include <global_state.hpp>

namespace io::base {
    template <typename derived>
    struct consumer_t {
        using clock       = std::chrono::system_clock;
        using time_point  = typename clock::time_point;
        using duration    = typename clock::duration;
        using contract_t  = uint16_t;

        consumer_t(bool is_irts_enabled, bool is_rts_enabled)
            : contracts(*this), is_irts_enabled(is_irts_enabled), is_rts_enabled(is_rts_enabled) {
        }

        void resize(size_t n_time_slots, size_t n_symbols) {
            I = n_symbols;
            if constexpr (requires(derived& d) { d.on_resize(n_time_slots, n_symbols); }) 
                static_cast<derived*>(this)->on_resize(n_time_slots, n_symbols); // rows x columns
        }

        void heart_beat(time_point tp) {
            if(global::state::shutdown_requested.load())  throw global::shutdown_exception();
            if constexpr (requires(derived d) { d.on_heart_beat(tp); })
                static_cast<derived*>(this)->on_heart_beat(tp);
        }
        
        void day_begin(time_point day) {
            contract_t n_instruments;
            n_instruments = flat_map.size();
            resize(T, n_instruments);
            if constexpr (requires(derived d) { d.on_day_begin(day); })
                static_cast<derived*>(this)->on_day_begin(day);    
        }

        void day_end(time_point day) {
            if constexpr (requires(derived d) { d.on_day_end(day); })
                static_cast<derived*>(this)->on_day_end(day);
            global::state::date_count++;
        }

        void trade_report(time_point time, uint64_t symbol, float price, uint32_t size, uint8_t flag) {
            static_cast<derived*>(this)->on_trade_report(time, contracts[symbol], price, size, flag);
        }
        void ask(time_point time, uint64_t symbol, float price, uint32_t size, uint8_t flag) {
            static_cast<derived*>(this)->on_ask(time, contracts[symbol], price, size, flag);
        }
        void bid(time_point time, uint64_t symbol, float price, uint32_t size, uint8_t flag) {
            static_cast<derived*>(this)->on_bid(time, contracts[symbol], price, size, flag);
        }
        void trade_break(time_point time, uint64_t symbol, float price, uint32_t size, uint8_t flag) {
            if constexpr (requires(derived& d) {d.trade_break(time, symbol, price, size, flag);})
                static_cast<derived*>(this)->trade_break(time, contracts[symbol], price, size, flag);
        }
        void syscall(time_point time, iex::system::message msg) {
            if constexpr (requires(derived& d) {d.on_syscall(time, msg);})
                static_cast<derived*>(this)->on_syscall(time, msg);            
        }

        [[nodiscard]] contract_t operator[](uint64_t iex_symbol) try {
            return find_or_insert(iex_symbol);
        } catch (const std::invalid_argument& err){
            TRACE << err.what() << " <" << utils::iex_symbol(iex_symbol) << ">" << std::endl;
            return 0;
        }
        void batch_insert(std::vector<std::string> instruments) {
            std::ranges::transform(instruments, instruments.begin(), [](const std::string& symbol) {
                if (symbol.size() > 8) throw std::invalid_argument("Symbol too long: " + symbol);
                return symbol.size() < 8 ? utils::pad(symbol, 8, ' ') : symbol;
            });
            std::unordered_set<std::string_view> seen;
            for (const auto& symbol : instruments) // verify if all elements are uniqe
                if (!seen.insert(symbol).second) throw std::invalid_argument("Duplicate symbol in instruments: " + symbol);
            std::set<char> character_table;
            for (const auto& symbol : instruments) for (char c : symbol)
                character_table.insert(c);
        
            std::stringbuf buf;
            std::ostream os(&buf);
            for (char c : character_table) os << "'" << c << "',";
            TRACE << "size:" << character_table.size() << " {" << buf.str() << "}" << std::endl;

            flat_map.reserve(flat_map.size() + instruments.size());
            for (const std::string& symbol : instruments)
                flat_map.emplace_back( utils::base64::encode(symbol, flat_map.size()));
            std::ranges::sort(flat_map);
        }        
        contract_t find_or_insert(uint64_t iex_symbol) {
            uint64_t base64_encoded_symbol, n_instruments;
            try {
                base64_encoded_symbol = utils::base64::encode(iex_symbol, 0);
            } catch (const std::runtime_error& err){
                ERROR << err.what() << " |" <<  utils::iex_symbol(iex_symbol) <<"|" << std::endl;
            }
            if( auto it = std::ranges::lower_bound(flat_map, base64_encoded_symbol); it != flat_map.end()) {
                if((base64_encoded_symbol & SYMBOL_MASK) == (*it & SYMBOL_MASK))
                    return *it & CONTRACT_ID_MASK;
                else flat_map.insert(it, base64_encoded_symbol | flat_map.size());
            } else flat_map.emplace_back(base64_encoded_symbol | flat_map.size());
            n_instruments = flat_map.size();

            resize(T, n_instruments);
            return n_instruments - 1;
        }
        
        void session_begin(std::string start, std::string interval, std::string stop) {
            using gs = global::state;
            benchmark_start = clock::now();
            generics::zeros(gs::event_count, gs::event_rate, gs::duration, gs::event_latency,
                gs::total_output_after, gs::total_output_delta, gs::date_count, gs::rts_count, gs::instrument_count);

            if constexpr (requires(derived d) { 
                { d.on_session_begin(start, interval, stop) } -> std::same_as<std::vector<duration>>;
            }) {
                rts = static_cast<derived*>(this)->on_session_begin(start, interval, stop);
            } else rts = utils::sequence<std::chrono::seconds>(start, interval, stop);

            std::tie(original_contract_size, T) = std::make_tuple(flat_map.size(), rts.size() - 1);
        }
        void session_end() {
            using namespace std::chrono;
            using gs = global::state;
            if constexpr (requires(derived d) { d.on_session_end(); })
                static_cast<derived*>(this)->on_session_end();
            benchmark_stop = clock::now();
            auto ms = duration_cast<milliseconds>(benchmark_stop - benchmark_start).count();
            gs::event_rate = gs::event_count * 1000.0 / ms, gs::event_latency = ms * 1e6 / gs::event_count;
            gs::duration = ms;
            gs::rts_count = T, gs::instrument_count = I;
        }

        time_point benchmark_start, benchmark_stop;
        contract_t T, I, original_contract_size;
        consumer_t<derived>& contracts;
        std::string status, clear = "\033[2K\r";
        duration start, stop, interval;
        std::vector<std::string> rts, trading_days;
        bool is_irts_enabled, is_rts_enabled;
        std::vector<uint64_t> flat_map;
        static constexpr contract_t MAX_CONTRACT_ID     = (1 << 16) - 1;
        static constexpr uint64_t SYMBOL_MASK           = ~uint64_t{0xFFFF};  // upper 48 bits
        static constexpr uint64_t CONTRACT_ID_MASK      = 0xFFFF;             // lower 16 bits
    };
} // namespace io::base

