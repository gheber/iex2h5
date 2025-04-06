#pragma once

#include <unordered_map>
#include <cstdint>
#include <string>
#include <iostream>
#include <chrono>
#include <format>
#include <limits>
#include <date/date.h>

#include <error.hpp>
#include <armadillo>
#include <generics.hpp>
#include <patterns.hpp>
#include <iex.hpp>
#include <h5cpp/core>
#include "tick.hpp"
#include <h5cpp/io>

namespace {
    inline std::string to_string(uint64_t symbol) {
        char *c = (char*) &symbol;
        return std::string(c, c + 8);
    }
    inline uint64_t to_ns(std::chrono::system_clock::time_point tp) {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count()
        );
    }
}

namespace io::stats {

    template <typename clock_t>
    struct consumer_t {
        using clock       = clock_t;
        using time_point  = typename clock::time_point;
        using duration    = typename clock::duration;

        consumer_t(h5::fd_t fd) {}
        void heart_beat(time_point tp) {}
        void begin(time_point tp) {
            INFO << std::format("[begin] {}", date::format("%F %T", date::floor<std::chrono::seconds>(tp))) << std::endl;
        }

        void end(time_point tp) {
            INFO << std::format("[end] {}",  date::format("%F %T", date::floor<std::chrono::seconds>(tp))) << std::endl;
            for (const auto& [id, data] : trades)
                INFO << std::format("  symbol {:6d}: {} trades, avg price {:.4f}, total size {}",
                    id, data.count, data.total_price / data.total_size, data.total_size) << std::endl;
        }

        void day_begin(time_point day) {
            INFO << std::format("[day_begin] {}", date::format("%F %T", date::floor<std::chrono::seconds>(day))) << std::endl;
            begin(day);
        }

        void day_end(time_point day) {
            end(day);
            INFO << std::format("[day_end] {}", date::format("%F %T", date::floor<std::chrono::seconds>(day))) << std::endl;
        }

        void trade_report(time_point /*t*/, uint64_t symbol_id, float price, uint64_t size, uint8_t /*flag*/) {
            auto& stat = trades[symbol_id];
            stat.total_price += price * size;
            stat.total_size  += size;
            stat.count += 1;
        }

        void ask(time_point /*t*/, uint64_t /*symbol_id*/, float /*price*/, uint64_t /*size*/, uint8_t /*flag*/) {}
        void bid(time_point /*t*/, uint64_t /*symbol_id*/, float /*price*/, uint64_t /*size*/, uint8_t /*flag*/) {}
        void trade_break(time_point /*t*/, uint64_t /*symbol_id*/, float /*price*/, uint64_t /*size*/, uint8_t /*flag*/) {}

    private:
        struct stats_t {
            double total_price = 0.0;
            uint64_t total_size = 0, count = 0;
        };

        std::unordered_map<uint64_t, stats_t> trades;
    };
} // namespace io::stats

namespace io::base {
    template <typename clock_t>
    struct consumer_t {
        using clock       = clock_t;
        using time_point  = typename clock::time_point;
        using duration    = typename clock::duration;

        consumer_t(h5::fd_t fd, std::string rts_path, std::string asset_path, std::string tradingdays_path,
            duration start, duration stop, duration interval) : fd(fd), rts_path(rts_path), asset_path(asset_path), 
            tradingdays_path(tradingdays_path), start(start), stop(stop), interval(interval), I(0), T(0) {
            
        }
        consumer_t(h5::fd_t fd, std::string rts_path, std::string asset_path, std::string tradingdays_path)
            : consumer_t(fd, rts_path, asset_path, tradingdays_path, duration(0), duration(0), duration(0) ) {
            load_index();
        }
        void load_index() try {
            if (H5Lexists(fd, asset_path.data(), H5P_DEFAULT) > 0) {
                instruments = h5::read<std::vector<std::string>>(fd, asset_path);
                for( std::string symbol: instruments ) 
                    map.emplace(std::make_pair(* (uint64_t*) symbol.data(), I++));
            }
            if (H5Lexists(fd, rts_path.data(), H5P_DEFAULT) <= 0) {
                for(const auto& index: utils::sequence(start, interval, stop))
                    rts.push_back(utils::duration_to_string(index));
                h5::write(fd, rts_path, rts);
            } else rts = h5::read<std::vector<std::string>>(fd, rts_path);
            T = rts.size();
            INFO << "setting I = " << I << " T = " << T << std::endl;
        } catch(h5::error::any err){
            ERROR << err.what() << std::endl;
        }

        void heart_beat(time_point tp) {}
        void begin(time_point tp) {
            load_index();
        } 
        void end(time_point tp) {}
        void day_begin(time_point day) { begin(day); }
        void day_end(time_point day) try {
            std::vector<std::string> assets;
            for(const auto[key, value] :map )
                assets.push_back( to_string(key) );
            std::sort(assets.begin(), assets.end());
            h5::write(fd, asset_path, assets);
        } catch (const h5::error::any err){
            ERROR << err.what() << std::endl;
        }

        void trade_report(time_point /*t*/, uint64_t iex_symbol_id, float price, uint64_t size, uint8_t /*flag*/) {
            find_or_insert(iex_symbol_id);
        }

        void ask(time_point /*t*/, uint64_t /*symbol_id*/, float /*price*/, uint64_t /*size*/, uint8_t /*flag*/) {}
        void bid(time_point /*t*/, uint64_t /*symbol_id*/, float /*price*/, uint64_t /*size*/, uint8_t /*flag*/) {}
        void trade_break(time_point /*t*/, uint64_t /*symbol_id*/, float /*price*/, uint64_t /*size*/, uint8_t /*flag*/) {}

        uint32_t to_id(uint64_t symbol) {
            const auto& it = map.find( symbol );
            return it != map.end() ? it->second : std::numeric_limits<uint32_t>::max();
        }
        uint32_t find_or_insert(uint64_t symbol) {
            const auto& it = map.find(symbol);
            if(it != map.end()) 
                return it->second;
            return map[symbol] = I++;
        }

        h5::fd_t fd;
        std::string rts_path, asset_path, tradingdays_path;
        duration start, stop, interval;
        std::unordered_map<uint64_t, uint32_t> map;
        std::vector<std::string> instruments, rts, trading_days;
        uint32_t I, T;
    };
} // namespace io::base

namespace io::rts {
    template <typename clock_t>
    struct consumer_t : public io::base::consumer_t<clock_t> {
        using base = io::base::consumer_t<clock_t>;
        using clock       = clock_t;
        using time_point  = typename clock::time_point;
        using duration    = typename clock::duration;
        using base::map, base::fd, base::rts, base::I, base::T;
        
        consumer_t(h5::fd_t fd, std::string rts_path, std::string asset_path, std::string tradingdays_path)
            : io::base::consumer_t<clock>(fd, rts_path, asset_path, tradingdays_path) {
            INFO << "starting consumer... " << std::endl;
            resize(I, T);
        }
        void resize(size_t R, size_t C) { //n_rows, n_cols
            this->max_slot = C;
            generics::resize(R,C, h5_bid,h5_ask,h5_trade,  h5_bid_volume, h5_ask_volume, h5_trade_volume);
            generics::resize(R,
                start, stop, 
                event_count, trade_size, trade_count, fbid, fask, ftrade,
                avg_trade_count, avg_spread, day_high, day_low, day_close, day_open);
            INFO << "R:" << R << " C:" << C << " slots: "  << max_slot << " " << h5_ask.n_rows << "x" << h5_ask.n_cols << std::endl;            
        }
        void day_begin(time_point day) {
            generics::zeros(
                fbid, fask, ftrade,
                h5_ask, h5_trade, h5_bid,  h5_bid_volume, h5_ask_volume, h5_trade_volume,
                trade_count, event_count, trade_size, avg_trade_count, avg_spread, day_high, day_low, day_close, slot
            );
        }
        void trade_report(time_point time, uint64_t symbol, float price, uint64_t size, uint8_t /*flag*/) {
            uint32_t id = this->to_id(symbol);
            if(slot >= max_slot || id > I) return;
            h5_trade_volume(id, slot) += size;
            trade_size[id] += size;
            trade_count[id]++;
            event_count[id]++;
            ftrade(time, id, price, size);            
        }
        void ask(time_point time, uint64_t symbol, float price, uint64_t size, uint8_t flag) {
            uint32_t id = this->to_id(symbol);
            if(slot >= max_slot || id > I) return;
            h5_ask_volume(id, slot) += size;
            fask(time, id, price, size);
            event_count[id]++;              
        }
        void bid(time_point time, uint64_t symbol, float price, uint64_t size, uint8_t flag) {
            uint32_t id = this->to_id(symbol);
            if(slot >= max_slot || id > I) return;
            h5_bid_volume(id, slot) += size;
            fbid(time, id, price, size);
            event_count[id]++;            
        }
        void trade_break(time_point time, uint64_t symbol_id, float price, uint64_t size, uint8_t flag) {}
        void heart_beat(time_point time) {
            auto ask = h5_ask.unsafe_col(slot), // direct memory access to matrix columns
                trade =  h5_trade.unsafe_col(slot), bid   =  h5_bid.unsafe_col(slot);
        
            auto tp = date::format("%H:%M:%S", date::floor<std::chrono::seconds>(time));
            fask.predict(ask); ftrade.predict(trade); fbid.predict(bid);
        
            for(uint64_t i = 0; i < trade.n_rows; i++) {
                if(trade[i] == 0 ) { // trade is the estimate of the value of an instrument, if no trade happens we split the spread
                    if( ask[i] > 0 && bid[i] > 0 )
                        trade[i] = bid[i] + 0.5 * (ask[i] - bid[i]);
                }
        
                if(start[i].length() == 0 && trade[i] > 0) 
                    start[i] = tp;
                else if(trade[i] > 0) stop[i] = tp; 
            }		
            slot++;        
        }
        void day_end(time_point day) {
            std::string today = date::format("%F", floor<std::chrono::days>(day));
            for( int i=0; i<avg_trade_count.size(); i++) { // rts
                avg_trade_count[i] = trade_count[i] / static_cast<float>( slot );
                // not traded assets/instruments have no `time` entries
                // setting them to `max` is sensible, as it spans 0 length
                if(start[i].empty()) start[i] = rts.back();
                if(stop[i].empty()) stop[i] = rts.back(); 
            }
        
            generics::round<VALUE_PRECISION>(h5_ask, h5_trade, h5_bid, avg_trade_count);
            generics::zeros2nans(h5_ask, h5_trade, h5_bid);
            h5::write(fd,"/rts/ask/"	+ today, h5_ask);
            h5::write(fd,"/rts/bid/"	+ today, h5_bid);
            h5::write(fd,"/rts/trade/"  + today, h5_trade);
            h5::write(fd,"/rts/volume/" + today, h5_trade_volume);
            h5::write(fd,"/stats/" + today + "/avg_trade_count", avg_trade_count);
            h5::write(fd,"/stats/" + today + "/trade_count", trade_count);
            h5::write(fd,"/stats/" + today + "/first_trade", start);
            h5::write(fd,"/stats/" + today + "/last_trade", stop);
            std::cout << today << std::endl;
        }

        uint64_t slot, max_slot, counter = 0;
    private:
        const std::string file_path, tradingdays_path, rts_path;
        time_point last_time, today;
        arma::fmat h5_bid, h5_ask, h5_trade;
        arma::umat h5_bid_volume, h5_ask_volume, h5_trade_volume;
        arma::uvec trade_size, trade_count, event_count;
        arma::fvec avg_trade_count, avg_spread, day_high, day_low, day_close, day_open;
        std::vector<std::string> start, stop;
        filters::ema_filter_t<clock> fbid, fask, ftrade;            
    };
}

namespace io::irts {
    template <typename clock_t>
    struct consumer_t : public io::base::consumer_t<clock_t> {
        using base = io::base::consumer_t<clock_t>;
        using clock       = clock_t;
        using time_point  = typename clock::time_point;
        using duration    = typename clock::duration;
        using base::map, base::fd, base::rts, base::find_or_insert;
        
        consumer_t(h5::fd_t fd, std::string rts_path, std::string asset_path,
                   std::string tradingdays_path)
            : io::base::consumer_t<clock>(fd, rts_path, asset_path, tradingdays_path) {}
    
        void day_begin(time_point day) try {
            irts = h5::create<iex::tick_t>(fd,
                "/irts/" + date::format("%F", floor<std::chrono::days>(day)), h5::max_dims{H5S_UNLIMITED}, h5::chunk{64 * 1024} | h5::gzip(9));
        } catch (const h5::error::io::dataset::create& err) {
            irts = h5::open(fd, "/irts/" + date::format("%F", floor<std::chrono::days>(day)) );
        } catch (const h5::error::any& err) {
            ERROR << err.what() << std::endl;
        }
        void trade_report(time_point now, uint64_t symbol, float price, uint64_t size, uint8_t /*flag*/) {
            h5::append(irts, iex::tick_t {
                .time = to_ns(now),
                .size = size, .price = price, .contract_id = find_or_insert(symbol),
                .is_trade = true, .is_bid = false, .is_ask = false, .remove_level = false, .reserved = 0});
        }
        void ask(time_point now, uint64_t symbol, float price, uint64_t size, uint8_t flag) {
            h5::append(irts, iex::tick_t {
                .time = to_ns(now),
                .size = size, .price = price, .contract_id = find_or_insert(symbol),
                .is_trade = false, .is_bid = false, .is_ask = true, .remove_level = false, .reserved = 0});
        }
        void bid(time_point now, uint64_t symbol, float price, uint64_t size, uint8_t flag) {
            h5::append(irts, iex::tick_t {
                .time = to_ns(now),
                .size = size, .price = price, .contract_id = find_or_insert(symbol),
                .is_trade = false, .is_bid = true, .is_ask = false, .remove_level = false, .reserved = 0});
        }
        void day_end(time_point day) {}

        h5::pt_t irts;
    };
}
