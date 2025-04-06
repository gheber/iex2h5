/*
 *   ALL RIGHTS RESERVED.
 *   _________________________________________________________________________________
 *   NOTICE: All information contained herein is, and remains the property of Varga
 *   Consulting and its suppliers, if any. The intellectual and technical concepts
 *   contained herein are proprietary to Varga Consulting and its suppliers and may be
 *   covered by Canadian and Foreign Patents, patents in process, and are protected
 *   by trade secret or copyright law. Dissemination of this information or reproduc-
 *   tion of this material is strictly forbidden unless prior written permission is
 *   obtained from Varga Consulting.
 *
 *   Copyright © <2017–2025> Varga Consulting, Toronto, On     info@vargaconsulting.ca
 *   _________________________________________________________________________________
 */
#pragma once

#include <chrono>
#include <concepts>
#include <string>
#include <functional>
namespace io {

	// ─────────────────────────────────────────────────────────────────────────────
	// Concepts
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename T>
	concept has_clock = requires {
		typename T::clock;
		typename T::clock::duration;
		typename T::clock::time_point;
	};

	template <typename T>
	concept consumer_concept = has_clock<T> && requires(T c,
		typename T::clock::time_point tp,
		uint64_t stock,
		float price,
		uint64_t size,
		uint8_t flag) {
		c.heart_beat(tp);
		c.begin(tp);
		c.end(tp);
		c.day_begin(tp);
		c.day_end(tp);
		c.trade_report(tp, stock, price, size, flag);
		c.ask(tp, stock, price, size, flag);
		c.bid(tp, stock, price, size, flag);
		c.trade_break(tp, stock, price, size, flag);
	};

	template <typename T, typename C>
	concept producer_concept = has_clock<C> && requires(T p, C& consumer,
		typename C::clock::duration start, typename C::clock::duration stop) {
		p.run(consumer, start, stop);
	};

	// ─────────────────────────────────────────────────────────────────────────────
	// Producer CRTP
	// ─────────────────────────────────────────────────────────────────────────────
	template <typename derived_t, consumer_concept consumer_t>
	struct producer_t {
		using type       = derived_t;
		using clock      = typename consumer_t::clock;
		using duration   = typename clock::duration;
		using time_point = typename clock::time_point;

		void run(consumer_t& ref, duration start_, duration stop_) {
			consumer = &ref;
			start = start_;
			stop = stop_;
			static_cast<derived_t*>(this)->run_impl();
		}

		void heart_beat(time_point tp)     { consumer->heart_beat(tp); }
		void begin(time_point tp)          { consumer->begin(tp); }
		void end(time_point tp)            { consumer->end(tp); }
		void day_begin(time_point day)     { consumer->day_begin(day); }
		void day_end(time_point day)       { consumer->day_end(day); }
		void trade_report(time_point t, uint64_t s, float p, uint64_t z, uint8_t f) { consumer->trade_report(t, s, p, z, f); }
		void ask(time_point t, uint64_t s, float p, uint64_t z, uint8_t f)          { consumer->ask(t, s, p, z, f); }
		void bid(time_point t, uint64_t s, float p, uint64_t z, uint8_t f)          { consumer->bid(t, s, p, z, f); }
		void trade_break(time_point t, uint64_t s, float p, uint64_t z, uint8_t f)  { consumer->trade_break(t, s, p, z, f); }

		duration start{}, stop{}, heart_beat_interval{};
	private:
		consumer_t* consumer = nullptr;
		producer_t() = default;
		friend derived_t;
	};
} // namespace io
