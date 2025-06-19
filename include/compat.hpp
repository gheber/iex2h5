/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#if __has_include(<format>)
    #include <format>
    namespace fmt_compat = std;
#elif __has_include(<fmt/core.h>)
    #include <fmt/core.h>
    #include <fmt/chrono.h>
    namespace fmt_compat = fmt;
#else
    #error "Neither <format> nor <fmt/core.h> is available."
#endif

