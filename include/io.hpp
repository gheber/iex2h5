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

#ifdef HAVE_GOOGLE_PROFILER
    #include <gperftools/profiler.h>
#endif

namespace io {
    template<typename consumer_t>
    requires io::consumer_concept<consumer_t>
    void execute(std::string path, std::string start, std::string interval, std::string stop, consumer_t& consumer){
        using duration = typename consumer_t::duration;
        using pcap = iex::pcap::producer_t<stream::file_t, consumer_t>;
        using pcapng = iex::pcapng::producer_t<stream::file_t, consumer_t>;
        using gzip_pcap = iex::pcap::producer_t<stream::gzip_t, consumer_t>;
        using gzip_pcapng = iex::pcapng::producer_t<stream::gzip_t, consumer_t>;

        enum class format { GZIP_PCAP, GZIP_PCAPNG, PCAP, PCAPNG };
        auto detect_format = [](FILE* fd) -> format {
            if (utils::is_gzip(fd)) {
                uint32_t magic = stream::gzip_t::peek(fd);
                if (utils::pcapng::is_valid_magic(magic)) return format::GZIP_PCAPNG;
                if (utils::pcap::is_valid_magic(magic))  return format::GZIP_PCAP;
            } else {
                uint32_t magic = stream::file_t::peek(fd);
                if (utils::pcapng::is_valid_magic(magic)) return format::PCAPNG;
                if (utils::pcap::is_valid_magic(magic))  return format::PCAP;
            }
            THROW_RUNTIME_ERROR("unsupported or unknown capture file format");
        };
        FILE* fd = (path == "-") ? stdin : std::fopen(path.c_str(), "rb");
        if (!fd)
            THROW_RUNTIME_ERROR("unable to open " + path);

        INFO << "processing " << path << std::endl;
        auto [start_, interval_, stop_] = utils::strings_to_duration<duration>(start, interval, stop);
        try {
        #ifdef HAVE_GOOGLE_PROFILER
            ProfilerStart("iex2h5.prof");
            INFO << "<<<<<<<<<<<<< profiler started (output: iex2h5.prof) >>>>>>>>>>>>" << std::endl;
        #endif
            switch (detect_format(fd)) {
                case format::PCAP:       pcap(fd, interval_).run(consumer, start_, stop_); break;
                case format::PCAPNG:     pcapng(fd, interval_).run(consumer, start_, stop_); break;
                case format::GZIP_PCAP:  gzip_pcap(fd, interval_).run(consumer, start_, stop_); break;
                case format::GZIP_PCAPNG:gzip_pcapng(fd, interval_).run(consumer, start_, stop_); break;
                default: THROW_RUNTIME_ERROR("unsupported format...");
            }
        #ifdef HAVE_GOOGLE_PROFILER
            ProfilerStop();
            INFO << "<<<<<<<<<<<< profiler stopped >>>>>>>>>>>>" << std::endl;
        #endif
        } catch (...) {
            if (fd != stdin) std::fclose(fd);
            throw;
        }
        if (fd != stdin) std::fclose(fd);
    }
    
    template<typename consumer_t, typename... args_t>
    requires io::consumer_concept<consumer_t>
    std::function<void()> create(std::vector<std::string> all, std::string start, std::string interval, std::string stop, args_t... args) {
        return [=]() mutable {
            consumer_t consumer( args... );
            consumer.session_begin(start, interval, stop);
            for (const auto& path : all) {
                try {
                    io::execute(path, start, interval, stop, consumer);
                } catch (const std::exception& ex) {
                    std::cerr << "[error] task threw exception: " << ex.what() << '\n';
                } catch (...) {
                    std::cerr << "[error] task threw unknown exception\n";
                }
            }
            consumer.session_end();
        };
    }
} // namespace io
