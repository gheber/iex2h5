/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <cstdio>
#include <concepts>
#include <bit> 
#include <armadillo>
#include <date/date.h> 
#include <compat.hpp>
#include <filesystem>
#include <regex>
#include <type_traits>
#include <unordered_set>


namespace utils {
	namespace ch = std::chrono;
    inline std::string iex_symbol(uint64_t iex_symbol) {
        return std::string(reinterpret_cast<const char*>(&iex_symbol), 8);
    }

    template <typename D>
    concept additive_range_type = requires(D d) { D{1}; d + d; d <= d; } && !std::is_same_v<D, std::string>;
    template<typename T> concept chrono_duration = requires { 
        typename T::rep;
	    typename T::period;
	    std::is_base_of_v<std::chrono::duration<typename T::rep, typename T::period>, T>;
    };

    template <int Precision, std::floating_point T>
    inline constexpr bool is_finite(T value) {
        constexpr T factor = std::pow(10.0, static_cast<T>(Precision));
        return std::ceil(value * factor) / factor > T{0.01};
    }

    template <typename T>
    requires std::floating_point<T>
    inline arma::uvec find_finite_diag(const arma::Mat<T>& Q) {
        std::vector<arma::uword> indices;
        indices.reserve(Q.n_rows);

        for (arma::uword i = 0; i < Q.n_rows; ++i)
            if (is_finite<4>(Q(i, i)))
                indices.push_back(i);

        return arma::uvec(indices);
    }

    template <typename duration>
    inline duration string_to_duration(const std::string& time_str) {
        int h, m, s;
        char sep1, sep2;
        std::istringstream in(time_str);
        in >> h >> sep1 >> m >> sep2 >> s;
    
        if (!in || sep1 != ':' || sep2 != ':' || in.peek() != EOF)
            throw std::runtime_error("Invalid time format: " + time_str);
    
        return ch::duration_cast<duration>(ch::hours{h} + ch::minutes{m} + ch::seconds{s});
    }

    template <typename duration_t, typename... strings_t>
    requires (std::convertible_to<strings_t, std::string> && ...)
    std::tuple<duration_t, duration_t, duration_t> strings_to_duration(strings_t&&... strs) {
        return std::make_tuple(string_to_duration<duration_t>(std::forward<strings_t>(strs))...);
    }

    template <typename duration_t>
    std::vector<duration_t> string_to_duration(const std::vector<std::string>& time) {
        std::vector<duration_t> result;
        result.reserve(time.size());
        for (const auto& str : time) 
            result.push_back(string_to_duration<duration_t>(str));
        return result;
    }

    template <typename duration>
    inline std::string duration_to_string(const duration& dur) {
        using namespace std::chrono;
        return date::format("%H:%M:%S", date::floor<seconds>(dur));
    }

	template <additive_range_type D>
	inline constexpr std::vector<D> sequence(D begin, D interval, D end) {
		std::vector<D> result;
        for (D current = begin; current <= end; current += interval)
			result.push_back(current);
		return result;
	}

    template <chrono_duration duration_t>
    inline std::vector<std::string> 
    sequence(const std::string& begin, const std::string& interval, const std::string& end) {
        using namespace std::chrono;
        auto [b, i, e] = utils::strings_to_duration<duration_t>(begin, interval, end);
        std::vector<std::string> result;
        for (auto d = b; d <= e; d += i)
            result.push_back(duration_to_string(d));
        return result;
    }

    template <typename duration_t>
    duration_t require_uniform_interval(const std::vector<duration_t>& times) {
        duration_t interval = times[1] - times[0];
        if (times.size() < 2) return interval;
        for (std::size_t i = 2; i < times.size(); ++i) {
            if (times[i] - times[i - 1] != interval)
                throw std::runtime_error("Non-uniform interval at index " + std::to_string(i));
        }
        return interval;
    }
        
    inline bool is_gzip(FILE* fd) {
        if (!fd) return false;

        unsigned char magic[2];
        long pos = std::ftell(fd);  // Save current position
        if (std::fread(magic, 1, 2, fd) != 2)
            return false;
        std::fseek(fd, pos, SEEK_SET);  // Rewind to original pos

        return magic[0] == 0x1F && magic[1] == 0x8B;
    }

    std::vector<std::string> expand_glob(const std::string& pattern) {
        namespace fs = std::filesystem;
        std::vector<std::string> result;
    
        const auto slash_pos = pattern.find_last_of("/\\");
        const std::string dir  = (slash_pos != std::string::npos) ? pattern.substr(0, slash_pos) : ".";
        const std::string glob = (slash_pos != std::string::npos) ? pattern.substr(slash_pos + 1) : pattern;
    
        std::string regex_str = std::regex_replace(
            glob, std::regex(R"([\.\^\$\|\(\)\[\]\+\{\}\\])"), R"(\\$&)"
        );
        regex_str = std::regex_replace(regex_str, std::regex(R"(\*)"), ".*");
        regex_str = std::regex_replace(regex_str, std::regex(R"(\?)"), ".");
    
        const std::regex pattern_regex(regex_str);
    
        for (const auto& entry : fs::directory_iterator(dir)) {
            const std::string filename = entry.path().filename().string();
            if (std::regex_match(filename, pattern_regex))
                result.push_back(entry.path().string());
        }
    
        return result;
    }

    std::vector<std::string> expand_glob_recursive(const std::string& pattern) {
        namespace fs = std::filesystem;
        std::vector<std::string> result;

        const auto slash_pos = pattern.find_last_of("/\\");
        const std::string dir  = (slash_pos != std::string::npos) ? pattern.substr(0, slash_pos) : ".";
        const std::string glob = (slash_pos != std::string::npos) ? pattern.substr(slash_pos + 1) : pattern;

        std::string regex_str = std::regex_replace(glob, std::regex(R"([\.\^\$\|\(\)\[\]\+\{\}\\])"), R"(\\$&)");

        regex_str = std::regex_replace(regex_str, std::regex(R"(\*\*/?)"), R"(.*?/)");
        regex_str = std::regex_replace(regex_str, std::regex(R"(\*)"), R"([^/]*?)");
        regex_str = std::regex_replace(regex_str, std::regex(R"(\?)"), R"([^/])");

        const std::regex pattern_regex(regex_str);

        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (!fs::is_regular_file(entry)) continue;

            const std::string relative_path = fs::relative(entry.path(), dir).string();
            if (std::regex_match(relative_path, pattern_regex))
                result.push_back(entry.path().string());
        }

        return result;
    }
    
    std::vector<std::string> resolve_input_paths(const std::vector<std::string>& raw_inputs) {
        namespace fs = std::filesystem;
        std::vector<std::string> files;
    
        for (const auto& filename : raw_inputs) {
            if (filename == "-") 
                files.emplace_back(filename);  // STDIN
            else if (fs::is_regular_file(filename)) 
                files.emplace_back(filename);  // file
            else if (fs::is_directory(filename))
                for (const auto& entry : fs::directory_iterator(filename)) 
                    if (fs::is_regular_file(entry))
                        files.emplace_back(entry.path().string());
            else if (filename.find("**") != std::string::npos)  {
                auto matches = expand_glob_recursive(filename);
                files.insert(files.end(), matches.begin(), matches.end());
            } else if (filename.find('*') != std::string::npos || filename.find('?') != std::string::npos) {
                auto matches = expand_glob(filename);
                files.insert(files.end(), matches.begin(), matches.end());
            } else {
                std::cerr << "[warn] Skipping unrecognized input: " << filename << std::endl;
            }
        }
    
        return files;
    }    
} // namespace util

namespace utils::pcap {
    inline constexpr uint32_t MAGIC_NATIVE_USEC = 0xa1b2c3d4;
    inline constexpr uint32_t MAGIC_NATIVE_NSEC = 0xa1b23c4d;
    inline constexpr uint32_t MAGIC_SWAP_USEC = 0xd4c3b2a1;
    inline constexpr uint32_t MAGIC_SWAP_NSEC = 0x4d3cb2a1;
    
    inline bool is_little_endian(uint32_t magic) {
        if (std::endian::native == std::endian::little){
            return (magic == 0xa1b2c3d4 || magic == 0xa1b23c4d);
        } else return (magic == 0xd4c3b2a1 || magic == 0x4d3cb2a1); 
    }

    inline bool is_big_endian(uint32_t magic) {
        if (std::endian::native == std::endian::big){
            return (magic == 0xa1b2c3d4 || magic == 0xa1b23c4d);
        } else return (magic == 0xd4c3b2a1 || magic == 0x4d3cb2a1); 
    } 

    inline bool is_native_byte_order(uint32_t magic) {
        return magic == 0xa1b2c3d4 || magic == 0xa1b23c4d;
    }

    inline bool needs_byteswap(uint32_t magic) {
        return !is_native_byte_order(magic);
    }

    inline bool is_valid_magic(uint32_t magic) {
        return magic == MAGIC_NATIVE_USEC || magic == MAGIC_NATIVE_NSEC ||
            magic == MAGIC_SWAP_USEC || magic == MAGIC_SWAP_NSEC;
    }
} // namespace utils::pcap

namespace utils {

    inline std::string trim(const std::string& str) {
        auto front = std::find_if_not(str.begin(), str.end(), [](int c) { return std::isspace(c); });
        auto back  = std::find_if_not(str.rbegin(), str.rend(), [](int c) { return std::isspace(c); }).base();
        return (back <= front ? std::string() : std::string(front, back));
    }

    inline std::string pad(const std::string& str, std::size_t width, char pad_char = ' ') {
        if (str.size() > width)
            throw std::invalid_argument("Input string longer than target width");
        std::string padded = str;
        padded.resize(width, pad_char);
        return padded;
    }

    inline std::vector<std::string> trim(std::vector<std::string> from) {
        std::transform(from.begin(), from.end(), from.begin(), [](std::string& element) {
            return utils::trim(element);
        });
        return from;
    }
} // namespace utils::pcap

    inline std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        if (s.empty()) return tokens;
        std::string token;
        std::istringstream token_stream(s);
        while (std::getline(token_stream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    inline std::string join(const std::vector<std::string>& v, const std::string& delimiter) {
        return std::accumulate(std::begin(v), std::end(v), std::string(),
            [&delimiter](const std::string& a, const std::string& b) -> std::string {
                return a + (a.length() > 0 ? delimiter : "") + b;
            });
    }

    inline std::string replace(std::string str, const char a, const char b) {
        for (auto& c : str) if (c == a) c = b;
        return str;
    }

    template <class T>
    inline void unique(T& container) {
        std::sort(container.begin(), container.end());
        auto last = std::unique(container.begin(), container.end());
        container.erase(last, container.end());
    }

    template<typename T>
    inline std::vector<T> merge(const std::vector<T>& a, const std::vector<T>& b) {
        std::unordered_set<T> seen(a.begin(), a.end());
        std::vector<T> result = a;
        for (const auto& item : b) {
            if (seen.insert(item).second)
                result.push_back(item);
        }
        return result;
    }

    inline std::string to_lower(const std::string& str) {
        std::string result; result.reserve(str.size());
        std::ranges::transform(str, std::back_inserter(result),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    inline std::string to_lower(std::string& str) {
        std::ranges::transform(str, str.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return str;
    }

    inline std::string to_upper(const std::string& str) {
        std::string result; result.reserve(str.size());
        std::ranges::transform(str, std::back_inserter(result),
            [](unsigned char c) { return std::toupper(c); });
        return result;
    }

    inline std::string to_upper(std::string& str) {
        std::ranges::transform(str, str.begin(),
            [](unsigned char c) { return std::toupper(c); });
        return str;
    }

    inline std::string env2redis_key(std::string str) {
        to_lower(str);
        std::ranges::replace(str, '_', '-');
        return str;
    }

    inline std::string mask_out(std::string str, char mask = 'x', int count = 4) {
        if (str.size() <= 2 * count)
            return std::string(str.size(), mask);
        std::fill(str.begin() + count, str.end() - count, mask);
        return str;
    }

    inline std::string today() {
        auto today = date::floor<date::days>(std::chrono::system_clock::now());
        return date::format(":%F", today);
    }

    inline std::chrono::system_clock::duration time_until_midnight_utc() {
        auto now = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(now);
        std::tm* utc_time = std::gmtime(&current_time);
        int seconds_since_midnight = utc_time->tm_hour * 3600 + utc_time->tm_min * 60 + utc_time->tm_sec;
        constexpr int seconds_per_day = 24 * 3600;
        int remaining_seconds = seconds_per_day - seconds_since_midnight;
        return std::chrono::seconds(remaining_seconds);
    }
} // namespace utils
