/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once
#include "consumers.hpp"
#include "fmt/format.h"
#include "sw/redis++/errors.h"
#include <sw/redis++/redis++.h>

namespace io::redis {
    struct consumer_t : public io::base::consumer_t<consumer_t> {
        using base = io::base::consumer_t<consumer_t>;
        using typename base::clock, typename base::duration, typename base::time_point, typename base::contract_t;
        using base::I, base::T, base::contracts, base::rts, base::CONTRACT_ID_MASK;

        consumer_t(std::string uri, std::string asset_path, std::string tradingdays_path, bool is_irts_enabled, bool is_rts_enabled) : base(is_irts_enabled, is_rts_enabled),
             asset_path(asset_path), tradingdays_path(tradingdays_path) {
            using namespace sw::redis;

            try {
                redis = std::make_shared<Redis>(uri);
                pipe = std::make_shared<sw::redis::Pipeline>(redis->pipeline());
                redis->ping();
                auto pipe = redis->pipeline();
                INFO << "Connected to Redis backend: " << uri << std::endl;
                if (redis->exists(asset_path)) {
                    std::vector<std::string> instruments;
                    redis->smembers(asset_path, std::back_inserter(instruments));
                    if (!instruments.empty())  batch_insert(instruments);
                }
            } catch (const sw::redis::Error &e) {
                THROW_RUNTIME_ERROR("Redis connection failed: " + std::string(e.what()));
            }
        }

        void on_day_begin(time_point day) {
            using namespace std::chrono;
            auto start_time = floor<seconds>(day);
            try {
                today = date::format("%F", floor<days>(day));
                status = iex::compat::format("▫ {}", floor<seconds>(day));
                redis->sadd(tradingdays_path, today);
            } catch (const sw::redis::Error& err) {
                ERROR << err.what() << std::endl;
                status = iex::compat::format("⯑ {}", start_time);
            }
            std::cout << status << std::flush;
        }
        
        void append(time_point now, contract_t contract, float price, uint32_t size, bool is_bid, bool is_trade, bool is_ask) {
            auto ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            std::string key = fmt::format("irts:{}", today), value = fmt::format("{},{},{},{},{},{},{}", ns_since_epoch, contract, price, size,
                is_bid ? 1 : 0, is_trade ? 1 : 0, is_ask ? 1 : 0);
            try {
                pipe->rpush(key, value);
                global::state::event_count++;
            } catch (const sw::redis::Error& e) {
                ERROR << "Redis RPUSH failed: " << e.what() << std::endl;
            }
        }
        
        void on_trade_report(time_point time, contract_t id, float price, uint32_t size, uint8_t ) {
            if(is_irts_enabled) append(time, id, price, size, false, true, false); 
        }

        void on_ask(time_point time, contract_t id, float price, uint32_t size, uint8_t flag) {
            if(is_irts_enabled) append(time, id, price, size, false, false, true); 
        }
        
        void on_bid(time_point time, contract_t id, float price, uint32_t size, uint8_t flag) {
            if(is_irts_enabled) append(time, id, price, size, true, false, false); 
        }

        void on_heart_beat(time_point time) {
            try {
                pipe->exec();  // Flush all queued RPUSH commands
            } catch (const sw::redis::Error& e) {
                ERROR << "Pipeline exec failed: " << e.what() << std::endl;
                pipe = std::make_shared<sw::redis::Pipeline>(redis->pipeline());
            }
            auto tp = date::format("%H:%M:%S", date::floor<std::chrono::seconds>(time));
            std::cout << clear << status << " " << tp << std::flush;
        }        
        void on_day_end(time_point day) {
            std::cout << " ✓" << std::endl;
        }

        void on_session_end() {
            try {
                std::ranges::sort(flatmap, [](uint64_t a, uint64_t b) {
                    return (a & CONTRACT_ID_MASK) < (b & CONTRACT_ID_MASK);
                });
                for (const auto& contract : flatmap) {
                    auto [symbol, ok] = utils::base64::decode(contract);
                    if (ok) {
                        redis->sadd(asset_path, symbol);
                    }
                }
                INFO << "Session ended. Stored " << flatmap.size() << " instruments to Redis" << std::endl;
            } catch (const sw::redis::Error& e) {
                ERROR << "Redis instrument set update failed: " << e.what() << std::endl;
            }
        }

    private:
        std::shared_ptr<sw::redis::Redis> redis;
        std::shared_ptr<sw::redis::Pipeline> pipe;
        std::string asset_path, tradingdays_path, today, dir, filename;
        std::vector<std::string> start, stop;
    };
}
