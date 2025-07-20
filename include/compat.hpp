/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 *
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <string>
#include <string_view>
#include <utility>

#if __has_include(<format>)
    #include <format>
    namespace iex::compat {
        template<typename... Args>
        std::string format(std::string_view fmt_str, Args&&... args) {
            return std::vformat(fmt_str, std::make_format_args(std::forward<Args>(args)...));
        }
    } 
#elif __has_include(<fmt/core.h>)
    #include <fmt/core.h>
    #include <fmt/chrono.h>
    namespace iex::compat {
        template<typename... Args>
        std::string format(std::string_view fmt_str, Args&&... args) {
            return fmt::format(fmt_str, std::forward<Args>(args)...);
        }
    }
#else
    #error "Neither std::format nor fmtlib is available"
#endif
