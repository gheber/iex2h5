/*
 *   ALL RIGHTS RESERVED.
 *   _________________________________________________________________________________
 *   NOTICE: All information contained  herein is, and remains the property  of  Varga
 *   Consulting and  its suppliers, if  any. The intellectual and  technical  concepts
 *   contained herein are proprietary to Varga Consulting and its suppliers and may be
 *   covered  by  Canadian and  Foreign Patents, patents in process, and are protected
 *   by  trade secret or copyright law. Dissemination of this information or reproduc-
 *   tion  of  this  material is strictly forbidden unless prior written permission is
 *   obtained from Varga Consulting.
 *
 *   Copyright © <2017-2025> Varga Consulting, Toronto, On     info@vargaconsulting.ca
 *   _________________________________________________________________________________
 */
#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <concepts>
#include <armadillo>
#include <date/date.h> 
#include "compat.hpp"
namespace utils {
	namespace ch = std::chrono;

	// ─────────────────────────────────────────────────────────────────────────────
	// Sequence generator for chrono durations
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename D>
	requires requires(D d) { D{1}; d + d; d <= d; }
	constexpr std::vector<D> sequence(D begin, D interval, D end) {
		std::vector<D> result;
		for (D current = begin + interval; current <= end; current += interval)
			result.push_back(current);
		return result;
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// Check if a value is "finite enough" (rounded to P decimal places)
	// ─────────────────────────────────────────────────────────────────────────────
	template <std::integral Int, std::floating_point T>
	constexpr bool is_finite(T value) {
		constexpr T factor = std::pow(10.0, static_cast<T>(Int{}));
		return std::ceil(value * factor) / factor > T{0.01};
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// Find diagonal finite elements in a matrix
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename T>
	requires std::floating_point<T>
	arma::uvec find_finite_diag(const arma::Mat<T>& Q) {
		std::vector<arma::uword> indices;
		indices.reserve(Q.n_rows);

		for (arma::uword i = 0; i < Q.n_rows; ++i)
			if (is_finite<4>(Q(i, i)))
				indices.push_back(i);

		return arma::uvec(indices);
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// Parse HH:MM:SS string to chrono duration
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename duration>
	duration string_to_duration(const std::string& time_str) {
		int h, m, s;
		char sep1, sep2;
		std::istringstream in(time_str);
		in >> h >> sep1 >> m >> sep2 >> s;

		if (!in || sep1 != ':' || sep2 != ':')
			throw std::runtime_error("Invalid time format: " + time_str);

		return ch::duration_cast<duration>(ch::hours{h} + ch::minutes{m} + ch::seconds{s});
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// Format chrono duration to HH:MM:SS string
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename duration>
	std::string duration_to_string(const duration& dur) {
		using namespace std::chrono;
		return date::format("%H:%M:%S", date::floor<seconds>(dur));
	}
} // namespace util
