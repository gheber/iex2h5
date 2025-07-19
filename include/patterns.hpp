/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <string>
#include <functional>

namespace iex::system {
	enum class message: char {
		start_of_msgs = 'O', start_of_sys_hours = 'S', start_of_market = 'R',
		end_of_msgs = 'C', end_of_sys_hours = 'E', end_of_market = 'M' };
}

namespace io {
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
		uint32_t size,
		uint8_t flag,
		iex::system::message msg) {
		c.heart_beat(tp);
		c.day_begin(tp);
		c.day_end(tp);
		c.trade_report(tp, stock, price, size, flag);
		c.ask(tp, stock, price, size, flag);
		c.bid(tp, stock, price, size, flag);
		c.trade_break(tp, stock, price, size, flag);
		c.syscall(tp, msg);
	};

	template <typename T, typename C>
	concept producer_concept = has_clock<C> && requires(T p, C& consumer,
		typename C::clock::duration start, typename C::clock::duration stop) {
		p.run(consumer, start, stop);
	};
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
		void trade_report(time_point t, uint64_t s, float p, uint32_t z, uint8_t f) { consumer->trade_report(t, s, p, z, f); }
		void ask(time_point t, uint64_t s, float p, uint32_t z, uint8_t f)          { consumer->ask(t, s, p, z, f); }
		void bid(time_point t, uint64_t s, float p, uint32_t z, uint8_t f)          { consumer->bid(t, s, p, z, f); }
		void trade_break(time_point t, uint64_t s, float p, uint32_t z, uint8_t f)  { consumer->trade_break(t, s, p, z, f); }
		void syscall(time_point t, iex::system::message m)  { consumer->syscall(t, m); }

		duration start, stop, heart_beat_interval;
	private:
		consumer_t* consumer = nullptr;
		producer_t() = default;
		friend derived_t;
	};

	template<typename T>
	concept stream_t = requires(T stream, uint8_t* ptr, size_t size) {
		{ stream.pull(ptr, size) } -> std::convertible_to<size_t>;
	};
} // namespace io
