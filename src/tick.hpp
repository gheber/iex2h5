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
 *   Copyright © <2018> Varga Consulting, Toronto, On          info@vargaconsulting.ca
 *   _________________________________________________________________________________
 */
#pragma once

#include <cstdint>
#include <h5cpp/core>

namespace iex {
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