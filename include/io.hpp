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
#include <functional>

#include <patterns.hpp>
#include <producers.hpp>
#include <consumers.hpp>
#include <utils.hpp>
#include <hdf5.h>

#ifdef HAVE_GOOGLE_PROFILER
    #include <gperftools/profiler.h>
#endif

namespace utils::file {
    enum class format { GZIP_PCAP, GZIP_PCAPNG, PCAP, PCAPNG, HDF5 };
    format detect(const std::string& path) {
        htri_t is_hdf5 = H5Fis_accessible(path.c_str(), H5P_DEFAULT);
    
        if (is_hdf5 > 0) return format::HDF5;
    
        FILE* fd = std::fopen(path.c_str(), "rb");
        if (!fd) THROW_RUNTIME_ERROR("Failed to open file: " + path);
    
        if (utils::is_gzip(fd)) {
            uint32_t magic = io::stream::gzip_t::peek(fd);
            std::fclose(fd);
            if (utils::pcapng::is_valid_magic(magic)) return format::GZIP_PCAPNG;
            if (utils::pcap::is_valid_magic(magic))  return format::GZIP_PCAP;
        } else {
            uint32_t magic = io::stream::file_t::peek(fd);
            std::fclose(fd);
            if (utils::pcapng::is_valid_magic(magic)) return format::PCAPNG;
            if (utils::pcap::is_valid_magic(magic))  return format::PCAP;
        }
    
        THROW_RUNTIME_ERROR("Unsupported or unknown capture file format: " + path);
    };
}

namespace io {
    template<typename consumer_t>
    requires io::consumer_concept<consumer_t>
    void execute(std::string path, std::pair<std::string, std::string> date, std::pair<std::string, std::string> time, std::string time_interval, consumer_t& consumer){
        using duration = typename consumer_t::duration;
        using pcap = iex::pcap::producer_t<stream::file_t, consumer_t>;
        using pcapng = iex::pcapng::producer_t<stream::file_t, consumer_t>;
        using gzip_pcap = iex::pcap::producer_t<stream::gzip_t, consumer_t>;
        using gzip_pcapng = iex::pcapng::producer_t<stream::gzip_t, consumer_t>;
        using hdf5 = h5::producer_t<consumer_t>;
        
        FILE* fd = nullptr;
        const bool is_stdin = (path == "-");
        auto [start, interval, stop] = utils::strings_to_duration<duration>(time.first, time_interval, time.second);
        try {
            utils::file::format fmt = utils::file::detect(path);
            if (!is_stdin && fmt != utils::file::format::HDF5) {
                fd = std::fopen(path.c_str(), "rb");
                if (!fd) THROW_RUNTIME_ERROR("unable to open " + path);
            } else fd = stdin;
        #ifdef HAVE_GOOGLE_PROFILER
            ProfilerStart("iex2h5.prof");
            INFO << "<<<<<<<<<<<<< profiler started (output: iex2h5.prof) >>>>>>>>>>>>" << std::endl;
        #endif
            switch (fmt) {
                case utils::file::format::PCAP:       pcap(fd, interval).run(consumer, start, stop); break;
                case utils::file::format::PCAPNG:     pcapng(fd, interval).run(consumer, start, stop); break;
                case utils::file::format::GZIP_PCAP:  gzip_pcap(fd, interval).run(consumer, start, stop); break;
                case utils::file::format::GZIP_PCAPNG:gzip_pcapng(fd, interval).run(consumer, start, stop); break;
                case utils::file::format::HDF5:       hdf5(path, date, interval).run(consumer, start, stop); break;
                default: THROW_RUNTIME_ERROR("unsupported format...");
            }
            if (!is_stdin && fd) std::fclose(fd);
        #ifdef HAVE_GOOGLE_PROFILER
            ProfilerStop();
            INFO << "<<<<<<<<<<<< profiler stopped >>>>>>>>>>>>" << std::endl;
        #endif
        } catch (...) {
            if (fd != stdin) std::fclose(fd);
            throw;
        }
    }
    
    template<typename consumer_t, typename... args_t>
    requires io::consumer_concept<consumer_t>
    std::function<void()> create(std::vector<std::string> all, std::pair<std::string, std::string> date, std::pair<std::string, std::string> time, std::string interval, args_t... args) {
        return [=]() mutable {
            consumer_t consumer( args... );
            consumer.session_begin(time.first, interval, time.second);
            for (const auto& path : all) {
                try {
                    io::execute(path, date, time, interval, consumer);
                } catch (const global::shutdown_exception& ex) {
                    throw;                    
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
