/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <cstdint>
#include <h5cpp/core>

namespace iex {
	/**
     * @brief Compact representation of a market tick (trade or quote).
     * @ingroup io */
	struct tick_t {
		uint64_t time;
		float    price;
		uint32_t size;
		uint16_t contract_id;
		union {
			uint16_t flags;
			struct {
				uint16_t is_bid        : 1;
				uint16_t is_trade      : 1;
				uint16_t is_ask        : 1;
				uint16_t remove_level  : 1;
				uint16_t reserved      : 12;
			};
		};
	};
	static_assert(std::is_standard_layout_v<tick_t>);
}

namespace h5 {
    template<> hid_t inline register_struct<iex::tick_t>(){
		hid_t type = H5Tcreate(H5T_COMPOUND, sizeof (iex::tick_t));
		H5Tinsert(type, "time",        0, H5T_STD_U64LE);
		H5Tinsert(type, "price",       8, H5T_IEEE_F32LE);
		H5Tinsert(type, "size",       12, H5T_STD_U32LE);
		H5Tinsert(type, "contract_id",16, H5T_STD_U16LE);
		H5Tinsert(type, "flags",      18, H5T_STD_U16LE);
		return type;
	};
}
H5CPP_REGISTER_STRUCT(iex::tick_t);