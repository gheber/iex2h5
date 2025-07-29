/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */
#pragma once
#include <atomic>
#include <cstdint>

namespace global {
    struct shutdown_exception : public std::exception {
        const char* what() const noexcept override {
            return "graceful shutdown requested";
        }
    };
    struct state {
        static inline std::atomic<bool> shutdown_requested = false;
        static inline uint64_t event_count, event_rate, duration, event_latency, total_input, 
            total_output_before, total_output_after, total_output_delta, date_count, rts_count, instrument_count; 
    };
}
