/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca
 *
 * NOTE:
 *   - This header defines a generic execution harness for running IEX packet consumers.
 *   - The returned lambda captures all necessary runtime parameters and can be safely deferred.
 *   - Resource cleanup of the underlying `FILE*` is delegated to the producer implementation,
 *     which is expected to call `pcap_close()` or equivalent — RAII is intentionally avoided here.
 */

#pragma once

#include <cstdint>
#include <string>
#include <cstdio>
#include <utility>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <future>

#include <patterns.hpp>
#include <producers.hpp>
#include <utils.hpp>

namespace io {
    template < typename consumer_t, typename... args_t>
    requires io::consumer_concept<consumer_t> && requires(args_t&&... args) { consumer_t(std::forward<args_t>(args)...); }
    std::function<void()> task(std::string path, std::string start, std::string interval, std::string stop, args_t&&... args) {
        return [=, ... args_captured = std::forward<args_t>(args)]() mutable {
            using duration = typename consumer_t::duration;
            using gzip =  iex::pcap::producer_t<stream::gzip_t, consumer_t>;
            using pcap =  iex::pcap::producer_t<stream::file_t, consumer_t>;

            FILE* fd = (path == "-") ? stdin : std::fopen(path.c_str(), "rb");
            if (!fd)
                THROW_RUNTIME_ERROR("unable to open " + path);
            INFO << "processing " << path << std::endl;
            auto [start_, interval_, stop_] = utils::strings_to_duration<duration>(start, interval, stop);
            consumer_t consumer(args_captured...);
            try {
                if (utils::is_gzip(fd))
                    gzip(fd, interval_).run(consumer, start_, stop_);
                else pcap(fd, interval_).run(consumer, start_, stop_);
            } catch (...) {
                if (fd != stdin) std::fclose(fd);
                throw;
            }
    
            if (fd != stdin) std::fclose(fd);
        };
    }

    template<typename consumer_t, typename pool_t, typename tuple_t>
    std::future<void> submit_task(pool_t& pool, tuple_t&& args) {
        return std::apply(
            [&](auto&&... unpacked_args) {
                return pool.submit_task(io::task<consumer_t>(
                    std::forward<decltype(unpacked_args)>(unpacked_args)...));
            },
            std::forward<tuple_t>(args)
        );
    }
    
} // namespace io
