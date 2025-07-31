/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once
#define ARMA_NO_DEBUG
#include <concepts>
#include <cmath>
#include <limits>
#include <type_traits>
#include <armadillo>

namespace generics {
    namespace detail {
        template <typename T>
        using resize_1_arg_t = decltype(std::declval<T&>().resize(size_t{}));
    
        template <typename T>
        using resize_2_args_t = decltype(std::declval<T&>().resize(size_t{}, size_t{}));
    
        template <typename T>
        using resize_3_args_t = decltype(std::declval<T&>().resize(size_t{}, size_t{}, size_t{}));
    
        template <typename T>
        using resize_size_obj_t = decltype(std::declval<T&>().resize(arma::SizeMat{1,1}));
    
        template <typename T, template <typename> typename Op, typename = void>
        inline constexpr bool is_detected_v = false;
    
        template <typename T, template <typename> typename Op>
        inline constexpr bool is_detected_v<T, Op, std::void_t<Op<T>>> = true;
        template <typename T>
        using has_resize_void_t = decltype(std::declval<T&>().resize(size_t{}));
        template <typename T>
        using has_set_size_void_t = decltype(std::declval<T&>().set_size(size_t{}, size_t{}));
    
        template <typename T>
        using has_fill_void_t = decltype(std::declval<T&>().fill(0));        
    }
    
    template <typename T>
    concept arma_mat_like =
        detail::is_detected_v<T, detail::has_set_size_void_t> &&
        detail::is_detected_v<T, detail::has_fill_void_t>;
        
    template <typename T>
    concept arma_vector_like = requires(T x) {
        { x.n_elem } -> std::convertible_to<size_t>;
    } && detail::is_detected_v<T, detail::has_resize_void_t>;

    template <typename T, typename U>
    concept has_fill_method = requires(T x, U val) {
        { x.fill(val) } -> std::same_as<void>;
    };

    template <typename T, typename U>
    concept has_assign_method = requires(T x, U val) {
        { x.assign(x.size(), val) } -> std::same_as<void>;
    };

    template <typename T>
    concept fillable = requires(T x, typename T::value_type v) {
        { x.fill(v) } -> std::same_as<void>;
    };

    template <typename T>
    concept assignable = requires(T x, typename T::value_type v) {
        { x.assign(x.size(), v) } -> std::same_as<void>;
    };

    template <typename T>
    concept resizable_1d = requires(T x, size_t s) {
        { x.resize(s) } -> std::same_as<void>;
    };

    template <typename T>
    concept element_wise_mathable = requires(T x, size_t i) {
        typename T::value_type;
        { x[i] } -> std::convertible_to<typename T::value_type>;
        { x.n_elem } -> std::convertible_to<size_t>;
    };

    template <typename T>
    concept iterable_mutable = requires(T x) {
        { x.begin() } -> std::input_iterator;
        { x.end() } -> std::sentinel_for<decltype(x.begin())>;
    };

    template <typename T>
    concept nan_assignable_iterable = requires(T x) {
        typename T::value_type;
        requires std::floating_point<typename T::value_type>;
        { x.begin() } -> std::input_iterator;
        { x.end() } -> std::sentinel_for<decltype(x.begin())>;
    };
    
    template <typename T>
    concept floating_point_container = requires(T x) {
        typename T::value_type;
        requires std::floating_point<typename T::value_type>;
    };
}
namespace generics {
    template <typename T, typename Container>
    void fill(T val, Container& container) {
        if constexpr (fillable<Container> || has_fill_method<Container, T> || iterable_mutable<Container> || arma_mat_like<Container>) {
            container.fill(val);
        } else if constexpr (assignable<Container>) {
            container.assign(container.size(), val);
        } else if constexpr (std::is_arithmetic_v<std::remove_cvref_t<Container>>) {
            container = val;
        } else {
            static_assert([] { return false; }(), "Unsupported type in fill()");
        }
    }

    template <typename T, typename First, typename... Rest>
    void fill(T val, First& first, Rest&... rest) {
        fill(val, first);
        (fill(val, rest), ...);
    }

    template <typename... Args>
    void zeros(Args&... args) {
        fill(0, args...);
    }

    template <typename... Args>
    void ones(Args&... args) {
        fill(1, args...);
    }

    template <floating_point_container T>
    void nans(T& container) {
        fill(std::numeric_limits<typename T::value_type>::quiet_NaN(), container);
    }

    template <floating_point_container T, typename... Rest>
    void nans(T& head, Rest&... tail) {
        nans(head);
        (nans(tail), ...);
    }

    template <arma_vector_like T>
    void resize(size_t s, T& container) {
        container.resize(s);
    }

    template <typename T>
    requires (!arma_vector_like<T>)
    void resize(size_t s, T& container) {
        if constexpr (resizable_1d<T>) {
            container.resize(s);
        } else {
            static_assert([] { return false; }(), "resize() unsupported for this type");
        }
    }

    template <typename T, typename... Rest>
    requires (sizeof...(Rest) > 0)
    void resize(size_t s, T& head, Rest&... tail) {
        resize(s, head);
        (resize(s, tail), ...);
    }

    template <arma_mat_like T>
    void resize(size_t rows, size_t cols, T& mat) {
        mat.set_size(rows, cols);
    }

    template <arma_mat_like T, typename... Rest>
    void resize(size_t rows, size_t cols, T& head, Rest&... tail) {
        resize(rows, cols, head);
        (resize(rows, cols, tail), ...);
    }

    template <int precision, element_wise_mathable T>
    void round(T& container) {
        const double scale = std::pow(10.0, precision);
        for (auto& val : container)
            val = std::round(val * scale) / scale;
    }

    template <int precision, element_wise_mathable T, typename... Rest>
    void round(T& head, Rest&... tail) {
        round<precision>(head);
        (round<precision>(tail), ...);
    }

    template <element_wise_mathable T>
    void ceil(T& container) {
        for (auto& val : container)
            val = std::ceil(val);
    }

    template <element_wise_mathable T, typename... Rest>
    void ceil(T& head, Rest&... tail) {
        ceil(head);
        (ceil(tail), ...);
    }

    template <element_wise_mathable T>
    void floor(T& container) {
        for (auto& val : container)
            val = std::floor(val);
    }

    template <element_wise_mathable T, typename... Rest>
    void floor(T& head, Rest&... tail) {
        floor(head);
        (floor(tail), ...);
    }

    template <nan_assignable_iterable T>
    void zeros2nans(T& container) {
        using value_t = typename T::value_type;
        for (auto& v : container)
            if (v == value_t(0))
                v = std::numeric_limits<value_t>::quiet_NaN();
    }

    template <nan_assignable_iterable T, typename... Rest>
    void zeros2nans(T& head, Rest&... tail) {
        zeros2nans(head);
        (zeros2nans(tail), ...);
    }
} // namespace generics