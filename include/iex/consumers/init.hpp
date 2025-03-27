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

#ifndef IEX_ASSET_CONSUMER_HPP
#define	IEX_ASSET_CONSUMER_HPP

#include <string>
#include <vector>
#include <io/interface>
#include <date/tz.h>
#include <glog/logging.h>
#include <h5cpp/all>
#include <map>

using namespace std;
using namespace date;

namespace iex {
	class WithSymbols{};
	namespace ch = std::chrono;
	template <class Clock> struct InitConsumer :
		public io::Consumer<InitConsumer<Clock>, Clock>, io::WithSymbols {
		using time_point = typename Clock::time_point;
		using duration = typename Clock::duration;

		InitConsumer(std::string file_path, std::string tradingdays_path, 
			std::string asset_path, std::string rts_path, std::string day_begin_, std::string day_end_, unsigned interval)
			: file_path(file_path), asset_path(asset_path), tradingdays_path(tradingdays_path), rts_path(rts_path),
			day_begin_(day_begin_), day_end_(day_end_), interval(interval) {
		}

		void begin(uint64_t I, uint64_t S,  const std::vector<duration>& rts ){
			std::copy(rts.begin(), rts.end(), std::back_inserter(this->rts));
		};
		void trade_report_impl(time_point time,  uint64_t stock, float price, uint64_t size, uint8_t flag );
		void ask_impl(time_point time,  uint64_t stock, float price, uint64_t size, uint8_t flag ){};
		void bid_impl(time_point time,  uint64_t stock, float price, uint64_t size, uint8_t flag ){};
		void trade_break_impl(time_point time,  uint64_t stock, float price, uint64_t size, uint8_t flag ){};
		void heart_beat_impl( time_point time ){};
		void day_begin_impl( time_point day );
		void day_end_impl( time_point day );

		const std::string file_path, asset_path, tradingdays_path, rts_path,
			day_begin_, day_end_;
		unsigned count, interval;
		std::map<std::string,int> map;
		std::vector<duration> rts;

		void insert(uint64_t stock ){
			char *c = (char*) &stock;
			std::string key(c,c+8);
			map[key] = count++;
		}
	};
}

template <class Clock>
void iex::InitConsumer<Clock>::trade_report_impl(time_point time,  uint64_t stock, float price, uint64_t size, uint8_t flag ){
	insert(stock);
}

template <class Clock>
void iex::InitConsumer<Clock>::day_begin_impl( time_point day ){
	count = 0;
}

template <class Clock>
void iex::InitConsumer<Clock>::day_end_impl( time_point day ){
	using duration = typename Clock::duration;

	std::vector<std::string> assets;
	for(auto i:map ) assets.push_back( i.first );
	std::sort(assets.begin(), assets.end());

	auto fd = h5::create(file_path, H5F_ACC_TRUNC );
	h5::write(fd, asset_path, assets);
	if (H5Lexists(fd, rts_path.data(), H5P_DEFAULT) <= 0) {

		auto start_ = utils::string2duration<duration>(day_begin_,"%H:%M:%S");
		auto stop_  = utils::string2duration<duration>(day_end_, "%H:%M:%S");
		auto interval_ = std::chrono::duration_cast<duration>( std::chrono::seconds(interval));
		std::vector<std::string> rts;
		for(const auto& index: utils::sequence(start_, interval_, stop_))
			rts.push_back(utils::duration2string(index));
		h5::write(fd, rts_path, rts);
	}	
}
#endif

