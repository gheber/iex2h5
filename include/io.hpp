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

#include <chrono>
#include "iex.hpp"
#include "patterns.hpp"

namespace iex {
    /** @ingroup IEX
     * extracts data from pcap stream, then after disassembling packets delegates to io::consumer */
    template <typename consumer_t> struct transport_t :
    public io::producer_t<transport_t<consumer_t>,consumer_t> {
        using block_t = iex::protocol::block;
        using time_point = typename consumer_t::clock::time_point;
        using duration = typename consumer_t::clock::duration;

        /** IEX transport header parser to recover message blocks
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

        std::vector<std::string> symbols;
    private:
        void tops_v156(const iex::tops::v156::message* msg) {
            using namespace tops;
            time_point tp = time_point( duration( msg->hdr.time ));
            const v156::quote_update* qu = &msg->qu;
            const v156::trade_report* tr = &msg->tr;
            const v156::trade_break*  tb = &msg->tb;
        
            switch(msg->hdr.type) {
                case 'Q': // quote update
                    if(qu->ask_size) this->ask(tp, msg->hdr.symbol, 1e-4*qu->ask_price, qu->ask_size, 0);
                    if(qu->bid_size) this->bid(tp, msg->hdr.symbol, 1e-4*qu->bid_price, qu->bid_size, 0);
                    break;
                case 'T': // trade report
                    this->trade_report(tp, msg->hdr.symbol, 1e-4 * tr->price, tr->size, msg->hdr.flag);
                    break;
                case 'B': // trade break
                    this->trade_break(tp, msg->hdr.symbol, tb->price, tb->size, msg->hdr.flag);
                    break;
            }                
        }

        void tops_v163(const iex::tops::v163::message  * msg) {
            constexpr float conv_scalar = 1e-4;
            using namespace tops;
        
            time_point tp = time_point(duration(msg->hdr.time));
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
                case '5': // ask: price level update or sell side or offer 
                    this->ask(tp, msg->hdr.symbol, 1e-4 * tr->price, tr->size, msg->hdr.flag);
                    break;
                case 'T': // trade report
                    this->trade_report(tp, msg->hdr.symbol, 1e-4 * tr->price, tr->size, msg->hdr.flag);
                    break;
                case 'B': // trade break
                    this->trade_break(tp, msg->hdr.symbol, tb->price, 0,  msg->hdr.flag);
                    break;
            }
            // P -- not shortable, sort of important status info                
        }
        
        long count=0;
        bool is_market_opened=false, is_market_closed=false, is_first_beat=true;
        time_point today, last_time;
    };
}
