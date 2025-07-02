/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <cstdint>
#include <string>
#include <cstdio>
#include <utility>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <future>

#include <patterns.hpp>
#include <producers.hpp>
#include <utils.hpp>

namespace io {
         * which encapsulate deeps and tops messages
         */
        void transport_handler( const iex::transport::header* segment ){
            using namespace std;
            using namespace date;
        
            if( !count ) today = date::floor<date::days>( time_point(duration( segment->time) ) );
            auto now = time_point(duration( segment->time) );
        
            // trigger opening market event
            if( now > today + this->start && !is_market_opened )
                is_market_opened = true,this->day_begin( now );
        
            char* cursor = (char*)(segment + 1); // the first message
            if( is_market_opened && !is_market_closed)
                // a segment may contain multiple messages, we are to iterate through them
                for( int i=0; i < segment->message_count; i++ ){
                    // make sure to trigger this timer event before processing the current
                    // HFT event, so the current state of client will not contain the event that tripped
                    // timer
                    if( now - last_time >= this->heart_beat_interval ){
                            last_time = date::floor<std::chrono::seconds>( now );
        
                            if( !is_first_beat ) 
                                this->heart_beat( last_time );
                            else
                                is_first_beat = false;
                    }
                    const block_t* block =  (block_t*) cursor;
                    switch( segment[i].protocol_id ) {
                        case IEX_DEEPS_v105: deeps_v105( (iex::deeps::v105::message*) &block->hdr ); break;
                        case IEX_TOPS_v156: tops_v156( (iex::tops::v156::message*)  &block->hdr ); break;
                        case IEX_TOPS_v163: tops_v163( (iex::tops::v163::message*)  &block->hdr ); break;
                        default: ;
                    }
                    cursor += (block->length+sizeof(block_t::length)); // move cursor to next block,
                }
            //closing market
            if( now > today + this->stop && is_market_opened && !is_market_closed )
                is_market_closed = true, this->day_end( now );
            count++;
        }
        // TODO: convert to CRTP
        virtual void run_impl() = 0;


    template < typename consumer_t, typename... args_t>
    requires io::consumer_concept<consumer_t> && requires(args_t&&... args) { consumer_t(std::forward<args_t>(args)...); }
    std::function<void()> task(std::string path, std::string start, std::string interval, std::string stop, args_t&&... args) {
        return [=, ... args_captured = std::forward<args_t>(args)]() mutable {
            using duration = typename consumer_t::duration;
            using gzip =  iex::pcap::producer_t<stream::gzip_t, consumer_t>;
            using pcap =  iex::pcap::producer_t<stream::file_t, consumer_t>;

            FILE* fd = (path == "-") ? stdin : std::fopen(path.c_str(), "rb");
            if (!fd)
                THROW_RUNTIME_ERROR("unable to open " + path);
            INFO << "processing " << path << std::endl;
            auto [start_, interval_, stop_] = utils::strings_to_duration<duration>(start, interval, stop);
            consumer_t consumer(args_captured...);
            try {
                if (utils::is_gzip(fd))
                    gzip(fd, interval_).run(consumer, start_, stop_);
                else pcap(fd, interval_).run(consumer, start_, stop_);
            } catch (...) {
                if (fd != stdin) std::fclose(fd);
                throw;
            }
    
            if (fd != stdin) std::fclose(fd);
        };
    }

            const v163::quote_update* qu = &msg->qu;
            const v163::trade_report* tr = &msg->tr;
            const v163::trade_break*  tb = &msg->tb;
        
            switch(msg->hdr.type) {
                case 'Q': // quote update
                    if( qu->ask_size ) this->ask(tp,  msg->hdr.symbol, conv_scalar * qu->ask_price, qu->ask_size, msg->hdr.flag);
                    if( qu->bid_size ) this->bid(tp,  msg->hdr.symbol, conv_scalar * qu->bid_price, qu->bid_size, msg->hdr.flag);
                    break;
                case 'T': // trade report
                    this->trade_report(tp, msg->hdr.symbol, conv_scalar * tr->price, tr->size, msg->hdr.flag);
                    break;
                case 'B': // trade break
                    this->trade_break(tp, msg->hdr.symbol, conv_scalar * tb->price, tb->size, msg->hdr.flag);
                    break;
            }                
        }

        void deeps_v105(const iex::deeps::v105::message * msg) {
            using namespace deeps;
            time_point tp = time_point(duration(msg->hdr.time));
            const v105::trade_report* tr = &msg->tr;
            const v105::trade_break*  tb = &msg->tb;
            // frequent: H,T,P,8   none: D,X,B  rare: O,E,A 
            switch(msg->hdr.type) {
                case '8': // bid: price level update buy size or bids
                    this->bid(tp, msg->hdr.symbol, 1e-4 * tr->price, tr->size, msg->hdr.flag);
                    break;
    template<typename consumer_t, typename pool_t, typename tuple_t>
    std::future<void> submit_task(pool_t& pool, tuple_t&& args) {
        return std::apply(
            [&](auto&&... unpacked_args) {
                return pool.submit_task(io::task<consumer_t>(
                    std::forward<decltype(unpacked_args)>(unpacked_args)...));
            },
            std::forward<tuple_t>(args)
        );
    }
    
} // namespace io
