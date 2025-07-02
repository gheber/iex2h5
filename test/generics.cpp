/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <armadillo>
#include <error.hpp>
#include <generics.hpp>

TEST_CASE("arma_mat_like concept") {
    CHECK(generics::arma_mat_like<arma::mat>);
    CHECK_FALSE(generics::arma_mat_like<std::vector<int>>);
}

TEST_CASE("arma_vector_like concept") {
    CHECK(generics::arma_vector_like<arma::vec>);
    CHECK_FALSE(generics::arma_vector_like<int>);
}

TEST_CASE("fill function fills all vector elements") {
    arma::fvec a, b, c, d, e, f;
    generics::resize(5, a, b, c, d, e, f);
    generics::fill(3.14f, a, b, c, d, e, f);

    auto check_all = [](auto&&... vecs) {
        (..., [&]{
            for (size_t i = 0; i < vecs.n_elem; ++i)
                CHECK(vecs[i] == doctest::Approx(3.14f));
        }());
    };
    check_all(a, b, c, d, e, f);
}
TEST_CASE("fill function fills all matrix elements") {
    arma::fmat A, B, C, D, E, F;
    generics::resize(5,10,  A, B, C, D, E, F);
    generics::fill(3.14f,  A, B, C, D, E, F);

    auto check_all = [](auto&&... M) {
        (..., [&]{
            for (size_t i = 0; i < M.n_elem; ++i)
                CHECK(M[i] == doctest::Approx(3.14f));
        }());
    };
    check_all(A, B, C, D, E, F);
}

TEST_CASE("resize vector types") {
    arma::fvec a, b, c, d, e, f;
    generics::resize(5, a, b, c, d, e, f);

    auto check_all = [](auto&&... V) {
        (..., [&]{
            CHECK(V.n_elem == 5);
        }());
    };
    check_all(a, b, c, d, e, f);
}

TEST_CASE("resize matrix types") {
    arma::fmat A, B, C, D, E, F;
    generics::resize(5,10,  A, B, C, D, E, F);
    
    auto check_all = [](auto&&... M) {
        (..., [&]{
            CHECK(M.n_rows == 5); CHECK(M.n_cols == 10);
        }());
    };
    check_all(A, B, C, D, E, F);
}

TEST_CASE("generics::zeros fills all elements with 0") {
    arma::fvec a, b, c, d, e, f;
    generics::resize(5, a, b, c, d, e, f);
    generics::zeros(a, b, c, d, e, f);

    auto check_all_zero = [](auto&&... V) {
        (..., [&]{
            for (auto val : V)
                CHECK(val == doctest::Approx(0.0f));
        }());
    };
    check_all_zero(a, b, c, d, e, f);
}

TEST_CASE("generics::ones fills all elements with 1") {
    arma::fmat A, B, C, D, E, F;
    generics::resize(2, 3, A, B, C, D, E, F);
    generics::ones(A, B, C, D, E, F);

    auto check_all_one = [](auto&&... M) {
        (..., [&]{
            for (size_t i = 0; i < M.n_elem; ++i)
                CHECK(M[i] == doctest::Approx(1.0f));
        }());
    };
    check_all_one(A, B, C, D, E, F);
}


TEST_CASE("generics::nans fills all elements with 0") {
    arma::fvec a, b, c, d, e, f;
    generics::resize(5, a, b, c, d, e, f);
    generics::nans(a, b, c, d, e, f);

    auto check_all = [](auto&&... V) {
        (..., [&]{
            for (auto val : V)
                CHECK( std::isnan(val));
        }());
    };
    check_all(a, b, c, d, e, f);
}

TEST_CASE("generics::nans fills all elements with 1") {
    arma::fmat A, B, C, D, E, F;
    generics::resize(2, 3, A, B, C, D, E, F);
    generics::nans(A, B, C, D, E, F);

    auto check_all_one = [](auto&&... M) {
        (..., [&]{
            for (auto val : M)
                CHECK( std::isnan(val));
        }());
    };
    check_all_one(A, B, C, D, E, F);
}

TEST_CASE("generics::round<2> rounds all elements to 2 decimal places") {
    arma::fvec a, b, c, d, e, f;
    generics::resize(4, a, b, c, d, e, f);
    generics::fill(1.234f, a, b, c, d, e, f);
    a[1] = b[2] = c[3] = 2.718f;

    generics::round<2>(a, b, c, d, e, f);

    auto check_all = [](auto&&... V) {
        (..., [&]{
            for (auto val : V) {
                float expected = std::round(val * 100.0f) / 100.0f;
                CHECK(val == doctest::Approx(expected).epsilon(1e-5));
            }
        }());
    };
    check_all(a, b, c, d, e, f);
}

TEST_CASE("generics::ceil ceils all elements") {
    arma::fvec a, b, c, d, e, f;
    generics::resize(3, a, b, c, d, e, f);
    generics::fill(1.1f, a, b, c, d, e, f);
    b[1] = d[0] = 2.3f;

    generics::ceil(a, b, c, d, e, f);

    auto check_all = [](auto&&... V) {
        (..., [&]{
            for (auto val : V)
                CHECK(val == doctest::Approx(std::ceil(val)));
        }());
    };
    check_all(a, b, c, d, e, f);
}

TEST_CASE("generics::floor floors all elements") {
    arma::fvec a, b, c, d, e, f;
    generics::resize(3, a, b, c, d, e, f);
    generics::fill(1.9f, a, b, c, d, e, f);
    d[2] = e[0] = 2.8f;

    generics::floor(a, b, c, d, e, f);

    auto check_all = [](auto&&... V) {
        (..., [&]{
            for (auto val : V)
                CHECK(val == doctest::Approx(std::floor(val)));
        }());
    };
    check_all(a, b, c, d, e, f);
}

TEST_CASE("generics::zeros2nans replaces zeros with NaNs") {
    arma::fvec a, b, c, d, e, f;
    generics::resize(3, a, b, c, d, e, f);

    // Fill with 0, except one element to make sure non-zero values stay untouched
    generics::fill(0.0f, a, b, c, d, e, f);
    a[1] = b[2] = c[0] = 1.23f;

    generics::zeros2nans(a, b, c, d, e, f);

    auto check_all = [](auto&&... V) {
        (..., [&]{
            for (auto val : V)
                CHECK((val == 0.0f || std::isnan(val) || val == doctest::Approx(1.23f)));
        }());
    };
    check_all(a, b, c, d, e, f);

    // Additional check: ensure zeroes are now NaNs
    auto check_zeros_are_nans = [](auto&&... V) {
        (..., [&]{
            for (auto val : V) {
                if (val != 1.23f)
                    CHECK(std::isnan(val));
            }
        }());
    };
    check_zeros_are_nans(a, b, c, d, e, f);
}
