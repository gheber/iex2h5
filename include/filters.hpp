/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once
#define ARMA_NO_DEBUG
#include <armadillo>
#include <concepts>
#include <cstdint>
#include <vector>
#include <queue>
#include <chrono>
#include <algorithm>
#include <limits>
#include <cassert>
#include "generics.hpp"

namespace filters {
    namespace detail {
        template <typename Derived, typename T>
        concept has_fill_impl = requires(Derived d, T val) {
            { d.fill_impl(val) } -> std::same_as<void>;
        };
        template <typename Derived, typename T>
        concept has_resize_impl = requires(Derived d, T val) {
            { d.resize_impl(val) } -> std::same_as<void>;
        };
        template <typename Derived, typename T>
        concept has_predict_impl = requires(Derived d, T val) {
            { d.predict_impl(val) } -> std::same_as<void>;
        };
        template <typename Derived, typename T>
        concept has_update_impl = requires(Derived d, T val) {
            { d.update_impl(val) } -> std::same_as<void>;
        };        
    }    

    struct crtp_filter_base_t {};

    template <typename derived_t, typename clock_t>
    struct filter_t : crtp_filter_base_t {
        using time_point = typename clock_t::time_point; /*!< Type alias for clock time points */
        using duration   = typename clock_t::duration;   /*!< Type alias for clock durations */

        filter_t() = default; /*!< Default constructor */

        void operator()(time_point tp, uint64_t stock, float value, uint64_t size) {
            static_cast<derived_t*>(this)->update_impl(tp, stock, value, size);
        }

        void predict(arma::fvec& out) {
            if constexpr (detail::has_predict_impl<derived_t&, arma::fvec>)
                static_cast<derived_t*>(this)->predict_impl(out);
            if (out.n_elem != price.n_elem)
                out.set_size(price.n_elem);
            std::ranges::copy(price, out.begin());
            if constexpr (detail::has_update_impl<derived_t&, void>)
                static_cast<derived_t*>(this)->update_impl();
        }
        void predict(arma::frowvec& out) {
            if constexpr (detail::has_predict_impl<derived_t&, arma::frowvec>)
                static_cast<derived_t*>(this)->predict_impl(out);
        
            if (out.n_elem != price.n_elem)
                out.set_size(price.n_elem);
        
            std::ranges::copy(price, out.begin());
        
            if constexpr (detail::has_update_impl<derived_t&, void>)
                static_cast<derived_t*>(this)->update_impl();
        }
        
        arma::frowvec predict() {
            arma::frowvec y(N);     // now it's 1×N
            predict(y);
            return y;
        }
        
        void update() {
            if constexpr (detail::has_update_impl<derived_t&, void>)
                static_cast<derived_t*>(this)->update_impl();
        }

        void resize(size_t n) {
            N = n;
            generics::resize(n, price, volume, start, stop);
            if constexpr (detail::has_resize_impl<derived_t&, size_t>)
                static_cast<derived_t*>(this)->resize_impl(n);
        }

        [[nodiscard]] constexpr size_t size() const noexcept { return N; } /*!< Returns the number of instruments */

        void fill(float val) {
            generics::fill(val, price, volume); /*!< Fill price and volume */
            const time_point min_tp = time_point::min();
            const time_point max_tp = time_point::max();

            if (val == 0.0f) {
                for (auto& s : start) s = min_tp;
                for (auto& s : stop)  s = max_tp;
            } else if (std::isnan(val)) {
                // leave start/stop unchanged
            }

            if constexpr (detail::has_fill_impl<derived_t&, float>)
                static_cast<derived_t*>(this)->fill_impl(val);
        }

        arma::fvec price; /*!< Internal buffer for estimated or smoothed prices */
        arma::fvec volume; /*!< Internal buffer for volume or weights */
        arma::fmat cov; /*!< Covariance matrix (optional use by derived filter) */
        std::vector<time_point> start; /*!< Start timestamps per instrument */
        std::vector<time_point> stop;  /*!< Stop timestamps per instrument */

    private:
        size_t N = 0; /*!< Number of instruments (stocks) */
    };

    template <typename clock_t>
    struct ema_filter_t : filter_t<ema_filter_t<clock_t>, clock_t> {
        using base_t     = filter_t<ema_filter_t<clock_t>, clock_t>; /*!< Base CRTP type providing common storage and interface */
        using time_point = typename base_t::time_point;              /*!< Timestamp type derived from clock_t */
        using duration   = typename base_t::duration;                /*!< Duration type derived from clock_t */

        explicit ema_filter_t(float alpha = 0.95f) : alpha(alpha) {
            assert(alpha > 0.0f && alpha <= 1.0f && "Alpha must be in (0, 1]");
        }

        void update_impl(time_point, uint64_t stock, float price, uint64_t size) {
            if (this->price(stock) > 0)
                this->price[stock] = alpha * price + (1.0f - alpha) * this->price[stock];
            else
                this->price(stock) = price;
        }

    private:
        float alpha; /*!< EMA smoothing factor ∈ (0, 1]. Controls exponential decay. */
    };

    template <typename clock_t>
    struct vcma_filter_t : filter_t<vcma_filter_t<clock_t>, clock_t> {
        using base_t     = filter_t<vcma_filter_t<clock_t>, clock_t>; /*!< Base CRTP type for storage and API */
        using time_point = typename base_t::time_point;                /*!< Clock-derived time point */
        using duration   = typename base_t::duration;                  /*!< Clock-derived duration */

        explicit vcma_filter_t(size_t window = 10) : window_size(window) {}

        void update_impl(time_point, uint64_t stock, float price_val, uint64_t size) {
            if (stock >= base_t::price.n_elem)
                return;

            const float w = static_cast<float>(size);
            const float x = price_val * w;

            if (!std::isfinite(base_t::price(stock))) {
                base_t::price(stock) = x / w;
                base_t::volume(stock) = w;
            } else {
                base_t::price(stock) = (base_t::price(stock) * base_t::volume(stock) + x) /
                                    (base_t::volume(stock) + w);
                base_t::volume(stock) += w;

                if (history[stock].size() >= window_size) {
                    auto [old_p, old_sz] = history[stock].front();
                    const float old_w = static_cast<float>(old_sz);
                    const float old_x = old_p * old_w;

                    base_t::price(stock) = (base_t::price(stock) * base_t::volume(stock) - old_x) /
                                        (base_t::volume(stock) - old_w);
                    base_t::volume(stock) -= old_w;
                    history[stock].pop_front();
                }

                history[stock].emplace_back(price_val, size);
            }
        }

        void update_impl() {}
        void predict_impl() {}

        void resize_impl(size_t N) {
            history.clear();
            history.resize(N);
        }

    private:
        size_t window_size; /*!< Maximum number of entries in the sliding window per stock */
        std::vector<std::deque<std::pair<float, uint64_t>>> history; /*!< Price-volume history for each stock */
    };

} // namespace filters
