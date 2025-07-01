/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <chrono>
#include <error.hpp>
#include <filters.hpp>

struct mock_clock {
    using time_point = std::chrono::steady_clock::time_point;
    using duration   = std::chrono::steady_clock::duration;
    static time_point now() { return std::chrono::steady_clock::now(); }
};

TEST_CASE("filters::ema_filter_t basic behavior") {
    filters::ema_filter_t<mock_clock> filter(0.5f); // alpha = 0.5
    filter.resize(2);

    auto now = mock_clock::now();
    filter(now, 0, 100.0f, 1);  // initial value
    filter(now, 0, 110.0f, 1);  // updated value

    CHECK(filter.price(0) == doctest::Approx(105.0f));
}

TEST_CASE("filters::vcma_filter_t volume-weighted average") {
    filters::vcma_filter_t<mock_clock> filter(3);  // window = 3
    filter.resize(1);

    auto now = mock_clock::now();
    filter(now, 0, 100.0f, 2);  // weighted: 200
    filter(now, 0, 120.0f, 1);  // weighted: 120, total vol = 3

    float expected_avg = (100.0f * 2 + 120.0f * 1) / 3;
    CHECK(filter.price(0) == doctest::Approx(expected_avg));
}

TEST_CASE("filters::predict and fill behavior") {
    filters::ema_filter_t<mock_clock> filter;
    filter.resize(1);
    filter.fill(0.0f);

    arma::fvec result;
    filter.predict(result);

    CHECK(result.n_elem == 1);
    CHECK(result(0) == doctest::Approx(0.0f));
}

TEST_CASE("filters::resize correctly resizes internal state") {
    filters::vcma_filter_t<mock_clock> filter(2);
    filter.resize(3);
    CHECK(filter.price.n_elem == 3);
    CHECK(filter.volume.n_elem == 3);
}

TEST_CASE("EMA filter with alpha = 1.0 uses last value only") {
    using clock_t = std::chrono::high_resolution_clock;
    filters::ema_filter_t<clock_t> filter(1.0f);
    filter.resize(1);

    auto now = clock_t::now();
    filter(now, 0, 100.0f, 10);
    filter(now, 0, 200.0f, 10); // Should fully replace the value

    CHECK(filter.price(0) == doctest::Approx(200.0f));
}

TEST_CASE("VCMA filter with window size = 1 behaves like regular weighted average") {
    using clock_t = std::chrono::high_resolution_clock;
    filters::vcma_filter_t<clock_t> filter(1);
    filter.resize(1);

    auto now = clock_t::now();
    filter(now, 0, 10.0f, 1);
    filter(now, 0, 20.0f, 3); // New weighted average: (10*1 + 20*3)/(1+3) = 17.5

    CHECK( filter.price(0) == doctest::Approx(20.0f) ); // only last tick retained
}

TEST_CASE("EMA filter predict reflects internal state") {
    using clock_t = std::chrono::high_resolution_clock;
    filters::ema_filter_t<clock_t> filter(0.9f);
    filter.resize(2);

    auto now = clock_t::now();
    filter(now, 0, 30.0f, 5);
    filter(now, 1, 70.0f, 5);

    arma::fvec prediction;
    filter.predict(prediction);

    CHECK(prediction.n_elem == 2);
    CHECK(prediction(0) == doctest::Approx(filter.price(0)));
    CHECK(prediction(1) == doctest::Approx(filter.price(1)));
}

TEST_CASE("Fill with NaN preserves timestamp fields") {
    using clock_t = std::chrono::high_resolution_clock;
    filters::ema_filter_t<clock_t> filter;
    filter.resize(2);
    
    filter.start[0] = clock_t::now();
    filter.stop[0] = clock_t::now();

    auto old_start = filter.start[0];
    auto old_stop = filter.stop[0];

    filter.fill(std::numeric_limits<float>::quiet_NaN());

    CHECK(filter.start[0] == old_start);
    CHECK(filter.stop[0] == old_stop);
}

TEST_CASE("Resize shrinks VCMA filter safely") {
    using clock_t = std::chrono::high_resolution_clock;
    filters::vcma_filter_t<clock_t> filter(5);
    filter.resize(10);

    // fill some dummy history
    auto now = clock_t::now();
    for (int i = 0; i < 10; ++i)
        filter(now, i, 10.0f * i, i + 1);

    filter.resize(2);
    CHECK(filter.price.n_elem == 2);
}
