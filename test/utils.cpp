/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <error.hpp>
#include <chrono>
#include <utils.hpp>
 
TEST_CASE("utils::sequence works for chrono::seconds") {
using namespace std::chrono;
seconds start{0}, step{5},stop{60};
std::vector<seconds> result = utils::sequence(start, step, stop);
CHECK(result.size() == 13); // range follows julia/matlab convetion: start - stop are included
for(int i=0; i<result.size(); i++)
    CHECK(result[i] == seconds{5*i});
}

TEST_CASE("utils::sequence returns empty when begin >= end") {
    using namespace std::chrono;
    std::vector<seconds> result = utils::sequence(seconds{10}, seconds{1}, seconds{5});
    CHECK(result.empty());
}

TEST_CASE("utils::is_finite basic threshold logic") {
    using utils::is_finite;
    CHECK( is_finite<2>(0.011));     // 0.011 -> ceil(1.1) -> 2 → 0.02 > 0.01
    // At 2 decimal places (0.01 precision)
    CHECK( is_finite<2>(0.0101));    // 0.0101 -> ceil(1.01) -> 2 → 0.02 > 0.01
    CHECK_FALSE( is_finite<2>(0.0099));  // 0.0099 * 100 = 0.99 -> ceil(0.99)=1 -> 0.01 == 0.01 → false

    // At 3 decimal places
    CHECK( is_finite<3>(0.01001));   // → 0.011 > 0.01
    CHECK_FALSE( is_finite<3>(0.00999));

    // Bigger numbers always pass
    CHECK( is_finite<2>(1.23));
    CHECK( is_finite<5>(123.000001));

    // Edge case: exactly 0.01
    CHECK_FALSE( is_finite<2>(0.01));

    // Negative values: ceil still works
    CHECK_FALSE( is_finite<2>(-0.01));
    CHECK_FALSE( is_finite<2>(-0.1));
    CHECK_FALSE( is_finite<2>(-1.0));
}

TEST_CASE("utils::find_finite_diag filters based on is_finite<4>") {
    using arma::mat;
    using arma::uvec;
    using utils::find_finite_diag;

    mat Q = arma::eye<mat>(5, 5);

    arma::vec values = {0.00001, 1.0, 0.009, 0.1, 0.000001};
    Q.diag() = values;

    auto result = find_finite_diag(Q);
    CHECK(result.n_elem == 2);
    CHECK(result(0) == 1);
    CHECK(result(1) == 3);
}

TEST_CASE("utils::string_to_duration parses valid HH:MM:SS strings") {
    using namespace std::chrono;
    using utils::string_to_duration;

    CHECK(string_to_duration<seconds>("01:02:03") == hours{1} + minutes{2} + seconds{3});
    CHECK(string_to_duration<minutes>("01:30:00") == minutes{90});
    CHECK(string_to_duration<seconds>("00:00:00") == seconds{0});
    CHECK(string_to_duration<seconds>("23:59:59") == hours{23} + minutes{59} + seconds{59});
}

TEST_CASE("utils::string_to_duration throws on invalid format") {
    using utils::string_to_duration;

    CHECK_THROWS_AS(string_to_duration<std::chrono::seconds>("01-02-03"), std::runtime_error);
    CHECK_THROWS_AS(string_to_duration<std::chrono::seconds>("badstring"), std::runtime_error);
    CHECK_THROWS_AS(string_to_duration<std::chrono::seconds>("01:02"), std::runtime_error);
    CHECK_THROWS_AS(string_to_duration<std::chrono::seconds>("01:02:03:04"), std::runtime_error);
}

TEST_CASE("utils::duration_to_string formats durations as HH:MM:SS") {
    using namespace std::chrono;
    using utils::duration_to_string;

    CHECK(duration_to_string(hours{0} + minutes{0} + seconds{0}) == "00:00:00");
    CHECK(duration_to_string(hours{1} + minutes{2} + seconds{3}) == "01:02:03");
    CHECK(duration_to_string(seconds{3723}) == "01:02:03");
    CHECK(duration_to_string(hours{23} + minutes{59} + seconds{59}) == "23:59:59");

    // higher precision inputs → truncation to floor(seconds)
    CHECK(duration_to_string(milliseconds{3723001}) == "01:02:03");
    CHECK(duration_to_string(duration_cast<nanoseconds>(hours{1})) == "01:00:00");
}

TEST_CASE("sequence<int> generates expected sequence") {
	const auto seq = utils::sequence(1, 2, 7);
	CHECK(seq == std::vector<int>{1, 3, 5, 7});
}

TEST_CASE("sequence<double> generates expected sequence") {
	const auto seq = utils::sequence(0.5, 0.5, 2.0);
	CHECK(seq.size() == 4);
	CHECK(std::fabs(seq[0] - 0.5) < std::numeric_limits<double>::epsilon());
	CHECK(std::fabs(seq[1] - 1.0) < std::numeric_limits<double>::epsilon());
	CHECK(std::fabs(seq[2] - 1.5) < std::numeric_limits<double>::epsilon());
	CHECK(std::fabs(seq[3] - 2.0) < std::numeric_limits<double>::epsilon());
}

TEST_CASE("sequence<std::string> generates expected sequence") {
    std::vector<std::string> 
        expected = {"09:00:00","09:10:00","09:20:00","09:30:00","09:40:00","09:50:00","10:00:00"},
        values = utils::sequence<std::chrono::seconds>("09:00:00", "00:10:00", "10:00:00");
    CHECK(expected == values);
}