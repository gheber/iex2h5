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
#include <atomic>
#include <error.hpp>

#include <generics.hpp>
#include <patterns.hpp>
#include <filters.hpp>
#include <compat.hpp>
#include <utils.hpp>
#include <base64.hpp>
#include <iex.hpp>
namespace global {
    struct shutdown_exception : public std::exception {
        const char* what() const noexcept override {
            return "graceful shutdown requested";
        }
    };
    struct state {
        static inline std::atomic<bool> shutdown_requested = false;
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

        consumer_t(bool is_irts_enabled, bool is_rts_enabled)
            : contracts(*this), rts(rts), is_irts_enabled(is_irts_enabled), is_rts_enabled(is_rts_enabled) {
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
        }

        void trade_report(time_point time, uint64_t symbol, float price, uint32_t size, uint8_t flag) {
            static_cast<derived*>(this)->on_trade_report(time, contracts[symbol], price, size, flag);
            n_events++;
        }
        void ask(time_point time, uint64_t symbol, float price, uint32_t size, uint8_t flag) {
            static_cast<derived*>(this)->on_ask(time, contracts[symbol], price, size, flag);
            n_events++;
        }
        void bid(time_point time, uint64_t symbol, float price, uint32_t size, uint8_t flag) {
            static_cast<derived*>(this)->on_bid(time, contracts[symbol], price, size, flag);
            n_events++;
        }
        void trade_break(time_point time, uint64_t symbol, float price, uint32_t size, uint8_t flag) {
            if constexpr (requires(derived& d) {d.trade_break(time, symbol, price, size, flag);})
                static_cast<derived*>(this)->trade_break(time, contracts[symbol], price, size, flag);
            n_events++;
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
            benchmark_start = clock::now();
            n_events = 0LL;
            if constexpr (requires(derived d) {
                { d.on_session_begin(start, interval, stop) } -> std::same_as<std::vector<duration>>;
            }) {
                rts = static_cast<derived*>(this)->on_session_begin(start, interval, stop);
            } else rts = utils::sequence<std::chrono::seconds>(start, interval, stop);
            T = rts.size();
        }
        void session_end() {
            using namespace std::chrono;
            if constexpr (requires(derived d) { d.on_session_end(); })
                static_cast<derived*>(this)->on_session_end();
            benchmark_stop = clock::now();
            auto ms = duration_cast<milliseconds>(benchmark_stop - benchmark_start).count();
            double rate = n_events * 1000.0 / ms, ell = ms * 1000.0 / n_events;
            std::cout << "✔ " << n_events << " ticks in " << ms << " ms  " << fmt::format("{:.1f} kilo ticks/s, {:.6f} µs/tick latency\n", rate/1e3, ell);
        }

        uint64_t n_events;
        time_point benchmark_start, benchmark_stop;
        contract_t T, I; //< time and instruments
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

namespace io::hdf5 {
    struct consumer_t : public io::base::consumer_t<consumer_t> {
        using base = io::base::consumer_t<consumer_t>;
        using typename base::clock, typename base::duration, typename base::time_point, typename base::contract_t;
        using base::I, base::T, base::contracts, base::rts;

        consumer_t(std::string path, std::string rts_path, std::string asset_path, std::string trading_days_path,
            bool is_irts_enabled, bool is_rts_enabled, uint8_t compression_level) : base(is_irts_enabled, is_rts_enabled),
            rts_path(rts_path), asset_path(asset_path), tradingdays_path(trading_days_path) {
            namespace fs = std::filesystem;
            namespace ch = std::chrono;
            
			dcpl = (compression_level != 0) ?  h5::gzip{compression_level} : h5::default_dcpl;
			try {
				fd = h5::open(path, H5F_ACC_RDWR);
			} catch (const h5::error::any& err){
				fd = h5::create(path, H5F_ACC_TRUNC);
			}
			std::vector<std::string> instruments;
			if (H5Lexists(fd, asset_path.data(), H5P_DEFAULT) > 0) {
				ds = h5::open(fd, asset_path);
				instruments = h5::read<std::vector<std::string>>(fd, asset_path);
			} else ds = h5::create<std::string>(fd, asset_path, h5::current_dims{0}, h5::max_dims{IEX_MAX_SYMBOLS}, h5::chunk{512}| h5::gzip{9}); 
			
            if(!instruments.empty()) batch_insert(instruments);
        }

        std::vector<std::string> on_session_begin(std::string start, std::string interval, std::string stop) {
            if(! is_rts_enabled) return {};

            h5::ds_t ds = H5Lexists(fd, rts_path.data(), H5P_DEFAULT) <= 0
                ? h5::write(fd, rts_path, utils::sequence<std::chrono::seconds>(start, interval, stop))
                : h5::open(fd, rts_path);
            return h5::read<std::vector<std::string>>(ds);
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
        void on_day_begin(time_point day) {
            auto start_time = std::chrono::floor<std::chrono::seconds>(day);
            generics::zeros(
                fbid, fask, ftrade,
                h5_ask, h5_trade, h5_bid,  h5_bid_volume, h5_ask_volume, h5_trade_volume,
                trade_count, event_count, trade_size, avg_trade_count, avg_spread, day_high, day_low, day_close, slot
            );
            try {
                if(is_irts_enabled) irts = h5::create<iex::tick_t>(fd,
                    "/irts/" + date::format("%F", floor<std::chrono::days>(day)), h5::max_dims{H5S_UNLIMITED}, h5::chunk{64 * 1024} | dcpl);
                status = iex::compat::format("▫ {}", start_time);
                std::cout << status;
            } catch (const h5::error::io::dataset::create& err) {
                irts = h5::open(fd, "/irts/" + date::format("%F", floor<std::chrono::days>(day)) );
                h5::reset(irts);
                status = iex::compat::format("▪ {}", start_time);
            } catch (const h5::error::any& err) {
                ERROR << err.what() << std::endl;
                status = iex::compat::format("⯑ {}", start_time);
            }
            std::cout << status << std::flush;
        }
        void append(time_point now, contract_t contract, float price, uint32_t size, bool is_bid, bool is_trade, bool is_ask) {
            uint16_t flags = 
                (is_bid ? 1 << 0 : 0) | (is_trade ? 1 << 1 : 0) | (is_ask ? 1 << 2 : 0);
            h5::append(irts, iex::tick_t {
                .time = utils::to_ns(now), .price = price, .size = size, .contract_id = contract, .flags = flags });
        }
        
        void on_trade_report(time_point time, contract_t id, float price, uint32_t size, uint8_t ) {
            if(is_rts_enabled)
                h5_trade_volume(slot, id) += size,
                ftrade(time, id, price, size);           
            
            trade_size[id] += size;
            trade_count[id]++;
            event_count[id]++;
            if(is_irts_enabled) append(time, id, price, size, false, true, false); 
        }

        void on_ask(time_point time, contract_t id, float price, uint32_t size, uint8_t flag) {
            if(is_rts_enabled)
                h5_ask_volume(slot, id) += size,
                fask(time, id, price, size);
            event_count[id]++;
            if(is_irts_enabled) append(time, id, price, size, false, false, true); 
        }
        void on_bid(time_point time, contract_t id, float price, uint32_t size, uint8_t flag) {
            if(is_rts_enabled)
                h5_bid_volume(slot, id) += size,
                fbid(time, id, price, size);
            event_count[id]++;
            if(is_irts_enabled) append(time, id, price, size, true, false, false); 
        }
        void on_heart_beat(time_point time) {
            auto tp = date::format("%H:%M:%S", date::floor<std::chrono::seconds>(time));
            std::cout << clear << status << " " << tp << std::flush;
            if(!is_rts_enabled) return;
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
                
                h5::write_or_replace(fd,"/stats/" + today + "/avg_trade_count", avg_trade_count);
                h5::write_or_replace(fd,"/stats/" + today + "/first_trade", start);
                h5::write_or_replace(fd,"/stats/" + today + "/last_trade", stop);

                generics::round<VALUE_PRECISION>(h5_ask, h5_trade, h5_bid, avg_trade_count);
                generics::zeros2nans(h5_ask, h5_trade, h5_bid);

                h5::dcpl_t all_dcpl =  h5::chunk{64,T} | dcpl;
                h5::write_or_replace(fd,"/rts/ask/"	+ today, h5_ask,          h5::max_dims{H5S_UNLIMITED, T}, all_dcpl);
                h5::write_or_replace(fd,"/rts/bid/"	+ today, h5_bid,          h5::max_dims{H5S_UNLIMITED, T}, all_dcpl);
                h5::write_or_replace(fd,"/rts/trade/"  + today, h5_trade,        h5::max_dims{H5S_UNLIMITED, T}, all_dcpl);
                h5::write_or_replace(fd,"/rts/volume/" + today, h5_trade_volume, h5::max_dims{H5S_UNLIMITED, T}, all_dcpl);
            }
            
            h5::write_or_replace(fd,"/stats/" + today + "/trade_count", trade_count);
            h5::write_or_replace(fd,"/stats/" + today + "/trade_size", trade_size);
            h5::write_or_replace(fd,"/stats/" + today + "/event_count", event_count);
            
            std::cout << " ✓" << std::endl;
        } catch(const h5::error::any& err){
            ERROR << err.what() << std::endl;
            std::cout << " ✗" << std::endl;
        }

        void on_session_end(){
			// condionally update trading days, given there has been RTS data processed
			if ( H5Lexists(fd, "stats", H5P_DEFAULT) > 0) try {
				std::vector<std::string> active_days = h5::ls(fd, "stats");
				h5::ds_t ds;
				if (H5Lexists(fd, tradingdays_path.data(), H5P_DEFAULT) > 0)
					ds = h5::open(fd, tradingdays_path);
				else ds = h5::create<std::string>(fd, tradingdays_path, h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{512}| h5::gzip{9});
				h5::set_extent(ds, h5::current_dims{active_days.size()});
				h5::write(ds, active_days, h5::offset{0}, h5::count{active_days.size()});
			} catch(const h5::error::any& err) {}

			const auto& all_contracts = flat_map;
			std::vector<std::string> asset_names(all_contracts.size());
			TRACE << "instruments: " << all_contracts.size() << std::endl;
			for(uint64_t contract: all_contracts) {
				auto[symbol, index] = utils::base64::decode(contract);
				if(index >= asset_names.size())
					throw std::runtime_error("Decoded index out of bounds.");
				asset_names[index] = utils::trim(symbol);
			}
			TRACE << "asset decoding has been completed" << std::endl;
            if (H5Fflush(fd, H5F_SCOPE_GLOBAL) < 0)
                THROW_RUNTIME_ERROR("hdf5 flush has failed...");
            if(flat_map.size() != all_contracts.size()) try {
                h5::set_extent(ds, h5::current_dims{asset_names.size()});
                h5::write(fd, asset_path, asset_names, h5::offset{0}, h5::count{asset_names.size()});
            } catch(const h5::error::any& err) {
                ERROR << err.what() << std::endl;
            } else INFO << "symbol/contract table has not changed, total: " << all_contracts.size() << std::endl;
        }

        uint64_t slot, max_slot, counter = 0;
    private:
        std::string rts_path, asset_path, tradingdays_path;
        h5::fd_t fd;
        h5::ds_t ds;
        h5::dcpl_t dcpl;
        h5::pt_t irts;
        time_point last_time, today;
        arma::fmat h5_bid, h5_ask, h5_trade;
        arma::umat h5_bid_volume, h5_ask_volume, h5_trade_volume;
        arma::uvec trade_size, trade_count, event_count;
        arma::fvec avg_trade_count, avg_spread, day_high, day_low, day_close, day_open;
        std::vector<std::string> start, stop;
        filters::ema_filter_t<clock> fbid, fask, ftrade;            
    };
}
