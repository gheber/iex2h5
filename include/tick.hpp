/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <cstdint>
#include <h5cpp/core>

/**
 * @file tick.hpp
 * @brief Defines the `iex::tick_t` structure and its HDF5 registration for seamless persistence using H5CPP.
 * @ingroup io
 *
 * This header introduces the compact, packed `tick_t` struct to represent individual market events (trades or quotes).
 * It is registered with HDF5 using the H5CPP `register_struct` mechanism, enabling automatic serialization/deserialization.
 *
 * Fields:
 * - `time`: nanosecond-resolution timestamp.
 * - `size`: order size or trade volume.
 * - `price`: trade or quote price.
 * - `contract_id`: 14-bit identifier for the associated trading instrument.
 * - Flags (bitfields):
 *   - `is_trade`: 1 if the event is a trade.
 *   - `is_bid`: 1 if the event is a bid update.
 *   - `is_ask`: 1 if the event is an ask update.
 *   - `remove_level`: 1 if the price level is removed.
 *   - `reserved`: 14 bits reserved for future use.
 *
 * This struct is exactly 24 bytes and packed for optimal I/O performance.
 */
namespace iex {
	/**
     * @brief Compact representation of a market tick (trade or quote).
     * @ingroup io */
	struct tick_t {
		uint64_t time, size; 
		float price;
		uint32_t contract_id: 14,
			is_trade: 1,
			is_bid: 1,
			is_ask: 1,
			remove_level: 1,
			reserved: 14;
	}__attribute__((packed));
	static_assert( sizeof(tick_t) == 24, "not aligned to byte!!!");
	static_assert(std::is_standard_layout_v<tick_t>);
}

namespace h5 {
	/**
     * @brief HDF5 compound datatype registration for `iex::tick_t`.
     * @ingroup io
     * @return HDF5 type ID suitable for dataset creation or writing. */
    template<> hid_t inline register_struct<iex::tick_t>(){
		hid_t type = H5Tcreate(H5T_COMPOUND, sizeof (iex::tick_t));
		H5Tinsert(type, "time",     0, H5T_STD_U64LE);
		H5Tinsert(type, "size",     8, H5T_STD_U64LE);
		H5Tinsert(type, "price",   16, H5T_NATIVE_FLOAT);
		H5Tinsert(type, "contract",20, H5T_STD_U32LE);
		return type;
	};
}
H5CPP_REGISTER_STRUCT(iex::tick_t);