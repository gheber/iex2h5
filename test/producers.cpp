/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <fstream>

#include <doctest/all>
#include <threadpool.hpp>

#include <error.hpp>
#include <consumers.hpp>
#include <producers.hpp>
#include <io.hpp>
#include "mock.hpp"

namespace test {
    struct consumer_t {
        using clock       = std::chrono::system_clock;
        using time_point  = typename clock::time_point;
        using duration    = typename clock::duration;

        consumer_t() {
            INFO << "consumer CTOR"<< std::endl;
        }
        void heart_beat(time_point tp) {
            TRACE << tp << std::endl;
        }
        void begin(time_point tp) {
            INFO << fmt_compat::format("[begin] {}", date::format("%F %T", date::floor<std::chrono::seconds>(tp))) << std::endl;
        }

        void end(time_point tp) {
            INFO << fmt_compat::format("[end] {}",  date::format("%F %T", date::floor<std::chrono::seconds>(tp))) << std::endl;
            for (const auto& [id, data] : trades)
                INFO << fmt_compat::format("  symbol {:6d}: {} trades, avg price {:.4f}, total size {}",
                    id, data.count, data.total_price / data.total_size, data.total_size) << std::endl;
        }

        void day_begin(time_point day) {
            INFO << fmt_compat::format("[day_begin] {}", date::format("%F %T", date::floor<std::chrono::seconds>(day))) << std::endl;
            begin(day);
        }

        void day_end(time_point day) {
            end(day);
            INFO << fmt_compat::format("[day_end] {}", date::format("%F %T", date::floor<std::chrono::seconds>(day))) << std::endl;
            std::set<uint64_t> symbols;
            auto aggregate_all = [&symbols](auto&&... map) {
                (..., [&] {
                    for (const auto& [key, value] : map)
                        symbols.insert(key);
                }());
            };
            aggregate_all(asks, bids, trades);
            TRACE << "total symbols: " << symbols.size() << std::endl;
        }

        void trade_report(time_point /*t*/, uint64_t symbol_id, float price, uint64_t size, uint8_t /*flag*/) {
            auto& stat = trades[symbol_id];
            stat.total_price += price * size;
            stat.total_size  += size;
            stat.count += 1;
        }

        void ask(time_point time, uint64_t symbol_id, float price, uint64_t size, uint8_t flag) {
            //TRACE << time << " " <<  symbol_id << " " << price << " " << size << std::endl;
            auto& stat = asks[symbol_id];
        }
        void bid(time_point time, uint64_t symbol_id, float price, uint64_t size, uint8_t flag) {
            auto& stat = bids[symbol_id];
        }
        void trade_break(time_point time, uint64_t symbol_id, float price, uint64_t size, uint8_t flag) {}

        struct stats_t {
            double total_price = 0.0;
            uint64_t total_size = 0, count = 0;
        };

        std::unordered_map<uint64_t, stats_t> trades,asks,bids;
    };
}

TEST_CASE("pcap: parse real IEX data.pcap and count packets") {
    TRACE << "start ==================" << std::endl;
    const size_t concurrency = std::thread::hardware_concurrency();
    bs::thread_pool pool(concurrency);

    const std::future<void> future = pool.submit_task(
        io::task<test::consumer_t>("/home/steven/data/TOPS-2017-01-05.pcap.gz", "14:00:00", "00:01:00", "14:10:00"));
        //io::task<test::consumer_t>(IEX_PCAP_FILE, "14:00:00", "00:01:00", "18:00:00"));
    future.wait();
    
    TRACE << "end ==================" << std::endl;
}


