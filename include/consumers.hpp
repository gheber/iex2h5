/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once
#define ARMA_NO_DEBUG
#include <armadillo>
#include <h5cpp/core>
    #include "tick.hpp"
#include <h5cpp/io>

#include <unordered_set>
#include <cstdint>
#include <string>
#include <iostream>
#include <chrono>
#include <limits>
#include <date/date.h>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <shared_mutex>

#include <error.hpp>

#include <generics.hpp>
#include <patterns.hpp>
#include <filters.hpp>
#include <compat.hpp>
#include <utils.hpp>
#include <base64.hpp>
#include <iex.hpp>

namespace {
    inline uint64_t to_ns(std::chrono::system_clock::time_point tp) {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count()
        );
    }
}

namespace global {
    struct state {
        static inline std::vector<uint64_t> flat_map;
    };
}
namespace h5 {
    template<typename T, class... args_t>
    inline void write_or_replace(h5::fd_t fd, const std::string& path, const T& data, args_t&&... args) {
        if (H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0)
            H5Ldelete(fd, path.c_str(), H5P_DEFAULT);
        h5::write(fd, path, data, args...);
    }
}
namespace io::base {
    template <typename derived>
    struct consumer_t {
        using clock       = std::chrono::system_clock;
        using time_point  = typename clock::time_point;
        using duration    = typename clock::duration;
        using contract_t  = uint16_t;

        consumer_t(h5::fd_t fd, std::vector<std::string> rts, bool is_irts_enabled, bool is_rts_enabled)
            : fd(fd), T(rts.size()), contracts(*this), rts(rts), is_irts_enabled(is_irts_enabled), is_rts_enabled(is_rts_enabled) {
            std::vector<duration> time = utils::string_to_duration<duration>(rts);
            utils::require_uniform_interval(time);
            std::tie(start, interval, stop) = std::make_tuple(time.front(), time[1] - time[0], time.back());
        }

        void resize(size_t n_time_slots, size_t n_symbols) {
            I = n_symbols;
            if constexpr (requires(derived& d) { d.on_resize(n_time_slots, n_symbols); }) 
                static_cast<derived*>(this)->on_resize(n_time_slots, n_symbols); // rows x columns
        }

        void heart_beat(time_point tp) {
            if constexpr (requires(derived d) { d.on_heart_beat(tp); })
                static_cast<derived*>(this)->on_heart_beat(tp);
        }
        
        void day_begin(time_point day) {
            contract_t n_instruments;
            n_instruments = global::state::flat_map.size();
            resize(T, n_instruments);
            if constexpr (requires(derived d) { d.on_day_begin(day); })
                static_cast<derived*>(this)->on_day_begin(day);    
        }
        void day_end(time_point day) {
            if constexpr (requires(derived d) { d.on_day_end(day); })
                static_cast<derived*>(this)->on_day_end(day);
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
        
        [[nodiscard]] contract_t operator[](uint64_t iex_symbol) try {
            return find_or_insert(iex_symbol);
        } catch (const std::invalid_argument& err){
            TRACE << err.what() << " <" << utils::iex_symbol(iex_symbol) << ">" << std::endl;
            return 0;
        }
        
        contract_t find_or_insert(uint64_t iex_symbol) {
            uint64_t base64_encoded_symbol, n_instruments;
            try {
                base64_encoded_symbol = utils::base64::encode(iex_symbol, 0);
            } catch (const std::runtime_error& err){
                ERROR << err.what() << " |" <<  utils::iex_symbol(iex_symbol) <<"|" << std::endl;
            }
            if( auto it = std::ranges::lower_bound(global::state::flat_map, base64_encoded_symbol); it != global::state::flat_map.end()) {
                if((base64_encoded_symbol & SYMBOL_MASK) == (*it & SYMBOL_MASK))
                    return *it & CONTRACT_ID_MASK;
                else global::state::flat_map.insert(it, base64_encoded_symbol | global::state::flat_map.size());
            } else global::state::flat_map.emplace_back(base64_encoded_symbol | global::state::flat_map.size());
            n_instruments = global::state::flat_map.size();

            resize(T, n_instruments);
            return n_instruments - 1;
        }

        static void batch_insert(std::vector<std::string> instruments) {
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

            global::state::flat_map.reserve(global::state::flat_map.size() + instruments.size());
            for (const std::string& symbol : instruments)
                global::state::flat_map.emplace_back( utils::base64::encode(symbol, global::state::flat_map.size()));
            std::ranges::sort(global::state::flat_map);
        }
    
        h5::fd_t fd;
        contract_t T, I; //< time and instruments
        consumer_t<derived>& contracts;
        std::string rts_path, asset_path, tradingdays_path;
        duration start, stop, interval;
        std::vector<std::string> rts, trading_days;
        bool is_irts_enabled, is_rts_enabled;
        static inline std::shared_mutex contract_id_mtx;
        static inline std::shared_mutex container_mtx;
        static constexpr contract_t MAX_CONTRACT_ID     = (1 << 16) - 1;
        static constexpr uint64_t SYMBOL_MASK           = ~uint64_t{0xFFFF};  // upper 48 bits
        static constexpr uint64_t CONTRACT_ID_MASK      = 0xFFFF;             // lower 16 bits
    };
} // namespace io::base

namespace io::rts {
    struct consumer_t : public io::base::consumer_t<consumer_t> {
        using base = io::base::consumer_t<consumer_t>;
        using typename base::clock, typename base::duration, typename base::time_point, typename base::contract_t;
        using base::fd, base::rts, base::I, base::T, base::contracts;
        
        consumer_t(h5::fd_t fd, h5::dcpl_t dcpl, std::vector<std::string> rts, bool is_irts_enabled, bool is_rts_enabled)
            : base(fd, rts, is_irts_enabled, is_rts_enabled), dcpl(dcpl) {
            INFO << "starting consumer... " << std::hex << this << std::dec <<  std::endl;
        }
        void on_resize(size_t R, size_t C) { //n_rows, n_cols
            max_slot = R;
            generics::resize(R,C, h5_bid,h5_ask,h5_trade,  h5_bid_volume, h5_ask_volume, h5_trade_volume);
            generics::resize(C,
                start, stop, 
                event_count, trade_size, trade_count, fbid, fask, ftrade,
                avg_trade_count, avg_spread, day_high, day_low, day_close, day_open);
            INFO << "R:" << R << " C:" << C << " slots: "  << max_slot << " " << h5_ask.n_rows << "x" << h5_ask.n_cols << std::endl;            
        }
        void on_day_begin(time_point day) try {
            generics::zeros(
                fbid, fask, ftrade,
                h5_ask, h5_trade, h5_bid,  h5_bid_volume, h5_ask_volume, h5_trade_volume,
                trade_count, event_count, trade_size, avg_trade_count, avg_spread, day_high, day_low, day_close, slot
            );
            if(is_irts_enabled) irts = h5::create<iex::tick_t>(fd,
                "/irts/" + date::format("%F", floor<std::chrono::days>(day)), h5::max_dims{H5S_UNLIMITED}, h5::chunk{64 * 1024} | dcpl);
        } catch (const h5::error::io::dataset::create& err) {
            irts = h5::open(fd, "/irts/" + date::format("%F", floor<std::chrono::days>(day)) );
            h5::reset(irts);
        } catch (const h5::error::any& err) {
            ERROR << err.what() << std::endl;
        }
        void append(time_point now, contract_t contract, float price, uint32_t size, bool is_bid, bool is_trade, bool is_ask) {
            uint16_t flags = 
                (is_bid ? 1 << 0 : 0) | (is_trade ? 1 << 1 : 0) | (is_ask ? 1 << 2 : 0);
            h5::append(irts, iex::tick_t {
                .time = to_ns(now), .price = price, .size = size, .contract_id = contract, .flags = flags });
        }

        void on_trade_report(time_point time, contract_t id, float price, uint32_t size, uint8_t ) {
            h5_trade_volume(slot, id) += size;
            trade_size[id] += size;
            trade_count[id]++;
            ftrade(time, id, price, size);           
            event_count[id]++;
            append(time, id, price, size, false, true, false); 
        }
        void on_ask(time_point time, contract_t id, float price, uint32_t size, uint8_t flag) {
            h5_ask_volume(slot, id) += size;
            fask(time, id, price, size);
            event_count[id]++;
            append(time, id, price, size, false, false, true); 
        }
        void on_bid(time_point time, contract_t id, float price, uint32_t size, uint8_t flag) {
            h5_bid_volume(slot, id) += size;
            fbid(time, id, price, size);
            event_count[id]++;
            append(time, id, price, size, true, false, false); 
        }
        void on_heart_beat(time_point time) {
            auto tp = date::format("%H:%M:%S", date::floor<std::chrono::seconds>(time));
            h5_ask(slot, arma::span::all) = fask.predict(), h5_bid(slot, arma::span::all) = fbid.predict();
            h5_trade(slot, arma::span::all) = ftrade.predict();
            for (arma::uword i = 0; i < I; ++i) {
                float& trade = h5_trade(slot, i);
                const auto [ask, bid] = std::tuple{h5_ask(slot, i), h5_bid(slot, i)};
            
                if (trade == 0 && ask > 0 && bid > 0)
                    trade = bid + 0.5f * (ask - bid);
            
                if (trade > 0)
                    start[i].length() == 0 ? start[i] = tp : stop[i] = tp;
            }
            slot++;        
        }
        
        void on_day_end(time_point day) try {
            TRACE << "day end..." << std::endl;
            std::string today = date::format("%F", floor<std::chrono::days>(day));
            if(is_rts_enabled) {
                for( int i=0; i<avg_trade_count.size(); i++) { // rts
                    avg_trade_count[i] = trade_count[i] / static_cast<float>( slot );
                    // not traded assets/instruments have no `time` entries
                    // setting them to `max` is sensible, as it spans 0 length
                    if(start[i].empty()) start[i] = rts.back();
                    if(stop[i].empty()) stop[i] = rts.back(); 
                }
                
                h5::write(fd,"/stats/" + today + "/avg_trade_count", avg_trade_count);
                h5::write(fd,"/stats/" + today + "/first_trade", start);
                h5::write(fd,"/stats/" + today + "/last_trade", stop);

                generics::round<VALUE_PRECISION>(h5_ask, h5_trade, h5_bid, avg_trade_count);
                generics::zeros2nans(h5_ask, h5_trade, h5_bid);

                h5::dcpl_t all_dcpl =  h5::chunk{64,T} | dcpl;
                h5::write(fd,"/rts/ask/"	+ today, h5_ask,          h5::max_dims{H5S_UNLIMITED, T}, all_dcpl);
                h5::write(fd,"/rts/bid/"	+ today, h5_bid,          h5::max_dims{H5S_UNLIMITED, T}, all_dcpl);
                h5::write(fd,"/rts/trade/"  + today, h5_trade,        h5::max_dims{H5S_UNLIMITED, T}, all_dcpl);
                h5::write(fd,"/rts/volume/" + today, h5_trade_volume, h5::max_dims{H5S_UNLIMITED, T}, all_dcpl);
            }
            
            h5::write(fd,"/stats/" + today + "/trade_count", trade_count);
            h5::write(fd,"/stats/" + today + "/trade_size", trade_size);
            h5::write(fd,"/stats/" + today + "/event_count", event_count);
            
            std::cout << today << std::endl;
        } catch(const h5::error::any& err){
            ERROR << err.what() << std::endl;
        }

        uint64_t slot, max_slot, counter = 0;
    private:
        h5::pt_t irts;
        h5::dcpl_t dcpl;
        time_point last_time, today;
        arma::fmat h5_bid, h5_ask, h5_trade;
        arma::umat h5_bid_volume, h5_ask_volume, h5_trade_volume;
        arma::uvec trade_size, trade_count, event_count;
        arma::fvec avg_trade_count, avg_spread, day_high, day_low, day_close, day_open;
        std::vector<std::string> start, stop;
        filters::ema_filter_t<clock> fbid, fask, ftrade;            
    };
}
