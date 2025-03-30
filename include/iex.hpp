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
#include <cstdint>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

namespace iex {
    static constexpr uint16_t IEX_DEEPS_v105 = 0x8004; //!< as defined in deeps spec
    static constexpr uint16_t IEX_TOPS_v156  = 0x8002; //!< defined in tops spec
    static constexpr uint16_t IEX_TOPS_v163  = 0x8003; //!< defined in tops spec
    static constexpr uint16_t IEX_TOPS_v164  = 0x8003; //!< same as v163
    namespace protocol {
        struct header{
            uint8_t type;
            uint8_t flag;
            int64_t time;
            int64_t symbol;
        }__attribute__((packed));
        static_assert( sizeof(header) == 18, "not aligned to byte!!!");
}}
   
// START V105
namespace iex::deeps {
	using header = iex::protocol::header;
}
namespace iex::deeps::v105 {
	/** The System Event Message is used to indicate events that apply to the market or the data feed.
	 * There will be a single message disseminated per channel for each System Event type within a given trading session
	 */
	   	// 'S' 0x53 10 bytes
		// 'O' - start of msg,    'S' start_sys_hours,     'R' - start regular market 
		// 'C' - end of messages, 'E' - end system hours,  'M' - end regular market	
		// The time of the update event as set by the IEX Trading System lo
	using system = iex::protocol::header;
	//static_assert( sizeof(system) + sizeof(header) == 10, "not aligned to byte!!!");
	/** IEX disseminates a full pre-market spin of Security Directory Messages 
	 * for all IEX-listed securities. After the pre-market spin, IEX will use 
	 * the Security Directory Message to relay changes for an individual security.
	 */ 
	struct security_directory { // 'D' 0x44 31 bytes
		uint32_t lot_size; //!< Number of shares that represent a round lot
		/* The corporate action adjusted previous official closing price for the security (e.g., stock split, dividend, rights offering),
		where the decimal portion is zero filled on the right. The decimal point is implied by position and does not explicitly
		appear in the field. For example, 123400 = $12.34. When no corporate action has occurred, the Adjusted POC Price will
		be populated with the previous official close price. For new issues (e.g., an IPO), this field will be the issue price.
		*/
		int64_t poc_price; //!< Corporate action adjusted previous official closing price
		uint8_t luld; 	   //!< Indicates which Limit Up-Limit Down price band calculation parameter is to be used
	} __attribute__((packed));

	struct luld {
		uint8_t na 	   : 1,
				tier_1 : 1,
				tier_2 : 1;
	}; // 1 byte
		// 'H' - halted across US equity, 
		// 'O' - halt released into Order Acceptance Period on IEX
		// 'P' - Trading paused and Order Acceptance Period on IEX  
		// 'T' - Trading on IEX
	struct trading_status{ // 'H' 0x48 22 bytes
		// Treading Halt reasons: 
		// 		T1: halt News Pending, IPO1: IPO/New Issue Not Yet Trading
		// 		IPOD: IPO/New Issue Deferred, MCB3: Market Wide Cicuit Breaker Level 3 - Breached
		// 		NA: Reason Not Available
		// Order Acceptance Period Reasons:
		// 		T2: Halt News Dissemination, IPO2: IPO/News Issue Order Acceptance Period
		// 		IPO3: IPO Pre-Launch Period
		// 		MCB1: Market Wide Curcuit Breaker Level 1 - breached
		// 		MCB2: Market Wide Curcuit Breaker Level 2 - breached
		uint8_t reason[4];
	}__attribute__((packed)); // 22  bytes

	struct operational_halt_status { // 'O' 0x4f 18 bytes
		int zero[0];
	};

	struct short_sale_price_test_status { // 'P' 0x50 19 bytes  
		uint8_t detail;
	}__attribute__((packed));

	struct security_event { // 'E' 0x45 18 bytes
		int zero[0]; // this may not work: zero length class hack
	}__attribute__((packed));

	struct price_level_update { // '8' 0x38, '5' 0x35 30 bytes
		uint32_t size;
		uint64_t price;
	}__attribute__((packed));

	struct trade_report { // 'T' 0x54 38 bytes
		uint32_t size;
		uint64_t price;
		uint64_t id;
	}__attribute__((packed));

	struct official_price { // 'X' 0x58 26 bytes
		uint64_t price;
	}__attribute__((packed));

	struct trade_break { // 'B' 0x42 38 bytes
		uint64_t price;
	}__attribute__((packed));

	struct auction_information { //'A' 80 bytes
		uint32_t paired_shares;
		uint64_t reference_price;
		uint64_t indicative_clearing_price;
		uint32_t imbalance_shares;
		uint8_t imbalance_side;
		uint8_t extension_number;
		uint32_t scheduled_auciton_time;
		uint64_t auction_book_clearing_price;
		uint64_t collar_reference_price;
		uint64_t lower_auction_collar;
		uint64_t upper_auction_collar;
	}__attribute__((packed));

	struct message {
		iex::deeps::header hdr;
		union {
			security_directory sd;
			trading_status ts;
			operational_halt_status ohs;
			short_sale_price_test_status sts;
			security_event se;
			price_level_update plu;
			trade_report tr;
			official_price op;
			trade_break tb;
			auction_information ai;
		} __attribute__((packed));
	}__attribute__((packed));
}
// END V105
// BEGIN V156
namespace iex::tops {
	/**
	 * price: 8byte int fixed 4 digit decimal
	 * fields are little endian or intel order
	 * int 4byte
	 * long 8byte
	 */
	static constexpr int protocol_id = 0x8002;
	static constexpr int channel_id  = 1;

	using header = iex::protocol::header;
}

namespace iex::tops::v156 { // udpated May 09, 2017
		/** 42 byte length, in intel order
		 */
		struct quote_update { //'Q' (0x51) 42 bytes
			uint32_t bid_size; //!< bid 
			int64_t bid_price; //!< price
			int64_t ask_price; //!< price 
			uint32_t ask_size; //!< ask
		} __attribute__((packed));
		static_assert( sizeof(quote_update) + sizeof(header) == 42, "not aligned to byte!!!");

		struct trade_report { //'T' (ox54) 42 bytes 
			uint32_t size;
			int64_t price; 	  //!< fixed last 4 digit is fractional
			int64_t trade_id; //!< iex generated id, may reference to trade break message
			int32_t reserved;
		} __attribute__((packed));
		using trade_break = trade_report;
		static_assert( sizeof(trade_report) + sizeof(header) == 42, "not aligned to byte!!!");
		static_assert( sizeof(trade_break)  + sizeof(header) == 42, "not aligned to byte!!!");

		struct quote_update_flag {
			uint8_t symbol_halt 	: 1,
					market_session  : 1;
		};
		struct trade_break_flag {
				uint8_t 
					intermarket_sweep 		: 1, //!< 0 - non intermarket sweep order, 1 - ISO
					extended_hours 			: 1,
					odd_lot 				: 1,
					trade_through_exempt 	: 1 ;
		};

	struct message {
		iex::tops::header hdr;
		union {
			trade_report tr;
			quote_update qu;
			trade_break  tb;
		}__attribute__((packed));
	}__attribute__((packed));
}

namespace iex::tops::v164 { // udpated Fab 27, 2018
	static constexpr int protocol_id = 0x8003;
	static constexpr int channel_id  = 1;
}
// END V156
// BEGIN V163
namespace iex { namespace tops {
	using header = iex::protocol::header;
}}


namespace iex::tops::v163 {
/* ADMINISTRATIVE MESSAGE FORMATS */

		/** IEX disseminates a full pre-market spin of Security Directory Messages 
		 * for all IEX-listed securities. After the pre-market spin, IEX will use 
		 * the Security Directory Message to relay changes for an individual security.
		 */ 
		struct security_directory { // 'D' 0x44 31 bytes
			uint32_t lot_size; //!< Number of shares that represent a round lot
			/* The corporate action adjusted previous official closing price for the security (e.g., stock split, dividend, rights offering),
			where the decimal portion is zero filled on the right. The decimal point is implied by position and does not explicitly
			appear in the field. For example, 123400 = $12.34. When no corporate action has occurred, the Adjusted POC Price will
			be populated with the previous official close price. For new issues (e.g., an IPO), this field will be the issue price.
			*/
			int64_t poc_price; //!< Corporate action adjusted previous official closing price
			uint8_t luld; 	   //!< Indicates which Limit Up-Limit Down price band calculation parameter is to be used
		}__attribute__((packed)); // 31 bytes
		static_assert( sizeof(security_directory) + sizeof(header) == 31, "not aligned to byte!!!");

		/** 'H' - halted across US equity, 
		    'O' - halt released into Order Acceptance Period on IEX
		    'P' - Trading paused and Order Acceptance Period on IEX  
			'T' - Trading on IEX
		*/
		struct trading_status { // 'H' 0x48 22 bytes
			// Treading Halt reasons: 
			// 		T1: halt News Pending, IPO1: IPO/New Issue Not Yet Trading
			// 		IPOD: IPO/New Issue Deferred, MCB3: Market Wide Cicuit Breaker Level 3 - Breached
			// 		NA: Reason Not Available
			// Order Acceptance Period Reasons:
			// 		T2: Halt News Dissemination, IPO2: IPO/News Issue Order Acceptance Period
			// 		IPO3: IPO Pre-Launch Period
			// 		MCB1: Market Wide Curcuit Breaker Level 1 - breached
			// 		MCB2: Market Wide Curcuit Breaker Level 2 - breached
			uint8_t reason[4];
		}__attribute__((packed)); // 22  bytes
		static_assert( sizeof(trading_status) + sizeof(header) == 22, "not aligned to byte!!!");

		// 'O' IEX specific halt, 'N' - not operationally halted
		struct operational_halt_status { // 'O' 0x4f 18 bytes
			int arr[0];
		}__attribute__((packed));
		static_assert( sizeof(operational_halt_status) + sizeof(header) == 18, "zero size class (hack) didn't work: please find me in source code!!!");

		struct short_sale_price_test_status{ // 'P' 0x50 19 bytes  
			uint8_t detail;
		}__attribute__((packed));
		static_assert( sizeof(short_sale_price_test_status) + sizeof(header) == 19, "not aligned to byte!!!");

/* TRADING MESSAGE FORMATS */
		/** 42 byte length, in intel order
		 */
		struct quote_update { //'Q' (0x51) 42 bytes
			int32_t bid_size; //!< bid 
			int64_t bid_price;//!< price
			int64_t ask_price;//!< price 
			int32_t ask_size; //!< ask
		}__attribute__((packed));
		static_assert( sizeof(quote_update) + sizeof(header) == 42, "not aligned to byte!!!");

		struct trade_report { //'T' (ox54) 38 bytes 
			uint32_t size;
			int64_t price; 		//!< fixed last 4 digit is frational
			int64_t trade_id;   //!< iex generated id, may reference to trade break message
		}__attribute__((packed));
		static_assert( sizeof(trade_report) + sizeof(header) == 38, "not aligned to byte!!!");

		struct official_price{ // 'X' 0x58 26 bytes
			uint64_t price;
		}__attribute__((packed));
		static_assert( sizeof(official_price) + sizeof(header) == 26, "not aligned to byte!!!");

		using trade_break = trade_report;
		static_assert( sizeof(trade_break) + sizeof(header) == 38, "not aligned to byte!!!");

		struct auction_information { // 80 bytes
			uint32_t paired_shares;
			uint64_t reference_price;
			uint64_t indicative_clearing_price;
			uint32_t imbalance_shares;
			uint8_t imbalance_side;
			uint8_t extension_number;
			uint32_t scheduled_auciton_time;
			uint64_t auction_book_clearing_price;
			uint64_t collar_reference_price;
			uint64_t lower_auction_collar;
			uint64_t upper_auction_collar;
		}__attribute__((packed));
		static_assert( sizeof(auction_information) + sizeof(header) == 80, "not aligned to byte!!!");

		struct quote_update_flag {
			uint8_t symbol_halt 	: 1,
					market_session  : 1;
		};
		struct trade_break_flag {
				uint8_t 
					intermarket_sweep : 1, //!< 0 - non intermarket sweep order, 1 - ISO
					extended_hours : 1,
					odd_lot : 1,
					trade_through_exempt : 1 ;
		};
		struct message {
			iex::tops::header hdr;
			union {
				security_directory sd;
				trading_status ts;
				operational_halt_status ohs;
				short_sale_price_test_status sps;
				quote_update qu;
				trade_report tr;
				official_price op;
				trade_break tb;
				auction_information aui;
			}__attribute__((packed));
		}__attribute__((packed));
}
// END V163


// BEGIN V125
namespace iex::transport {
    // iex->message_count == 0 && iex->payload_length == 0 => heart_beat

    /** Outbound Segments are sent by sources to listeners. Each Outbound Segment contains 
     * zero or more messages sent to listeners, described by the following IEX-TP Header.
     * Each Outbound Segment consists of a header and a payload that carries the actual data 
     * stream represented as a series of Message Blocks.
     */ 
    struct header { 				// 40 bytes
        uint8_t  version; 					//!< 0x1 Version of transport specification
        uint8_t  res; 						//!< reserved byte
        uint16_t protocol_id; 				//!< IEX_DEEPS_PROTOCOL_ID | IEX_TOPS_PROTOCOL_ID
        uint32_t channel_id; 				//!< Identifies the stream of bytes/sequenced messages
        uint32_t session_id; 				//!< Identifies the session
        uint16_t payload_length; 			//!< Byte length of the payload
        uint16_t message_count; 			//!< Number of messages in the payload
        int64_t  stream_offset; 			//!< Byte offset of the data stream
        int64_t  message_sequence; 			//!< Sequence of the first message in the segment
        int64_t  time; 						//!< Send time of segment
    }__attribute__((packed));
    static_assert( sizeof(header) == 40, "not aligned to byte!!!");
}
// END V125

namespace iex::protocol {
    struct block {
        /** The Message Length is an unsigned binary count representing the number of bytes in a message following the Message
         * Length field. The Message Length field value does not include the two bytes occupied by the Message Length field. The
         * total size of the Message Block is the value of the Message Length field plus two.
         */
        uint16_t length;
        header      hdr;
    }__attribute__((packed));
    static_assert( sizeof(block) == 20, "not aligned to byte!!!");
}
 