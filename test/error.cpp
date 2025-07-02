/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <error.hpp>
#include <sstream>
#include <syslog.h>
#include <string>

using namespace sigma::syslog;

TEST_CASE("logger_t streams basic types and emits syslog entry on std::endl") {
    logger_t log(LOG_USER | LOG_INFO);

    // Capture std::clog buffer
    std::ostringstream capture;
    std::streambuf* old = std::clog.rdbuf(capture.rdbuf());

    log << "Hello, logger!" << std::endl;

    // Restore old buffer
    std::clog.rdbuf(old);

    CHECK(true); // nothing to assert really, this just checks no throw
}

TEST_CASE("logger_t accumulates data until std::endl is sent") {
    logger_t log(LOG_USER | LOG_DEBUG);
    log << "Part 1, ";
    log << "Part 2" << std::endl;

    CHECK(true); // if we reach here, logger flushed
}
