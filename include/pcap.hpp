/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <string>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <pcap/pcap.h>

#include "error.hpp"
#include "patterns.hpp"
#include "io.hpp"
	
namespace iex::pcap {
	/** mock pcap packet to compute only the length, eventually 
	 * all fields are discarded
	 * @sa iex::transport::header iex::transport_t
	 */
	struct packet {
		uint8_t ethernet_frame[14];  //!< source,dest mac address + 2byte type 
		uint8_t ipv4[20]; 	         //!< IP address header, we don't care 
		uint8_t udp[8]; 	         //!< UDP frame: src,dst, len,chk sum 
	}__attribute__((packed));
		

	/** extracts data from pcap stream, then after disassembling packets delegates
	 * them to IexProtocolProducer for further processing  
	 * you must link against pcap library -lpcap
	 * \ingroup IEX
	 */
	template <class consumer> struct producer_t : public transport_t<consumer> {
		using type = producer_t<consumer>;
		using duration = typename consumer::duration;

		producer_t(FILE *fd, duration heart_beat) : fd(fd) {
			this->heart_beat_interval = heart_beat;
		}

		/**
		 * opens and reads input file, or exist with fatal ERROR if data stream is not Link Layer type 1
		 * Ethernet encapsulated UDP packets
		 * This method is usually called back by io::execute
		 */
		void run_impl(){
			pcap_t *fd_ = pcap_fopen_offline( fd, errbuf );

			if( fd_ == NULL ) 
				FATAL << "couldn't open file: " << input << " error: " << std::string(errbuf) << std::endl;
			int layer_type =   pcap_datalink( fd_ );
			if( layer_type != 1 )
				FATAL << "only ethernet frames are handled!!!" << std::endl;
			if(  pcap_loop( fd_, 0, pcap_handler, (uint8_t*)this ) < 0)
				FATAL << "pcap_loop() failed: " << pcap_geterr(fd_) << std::endl;
			pcap_close(fd_);
		}

	private:
		static void pcap_handler(uint8_t *ptr, const struct pcap_pkthdr* h, const uint8_t * start){
			type* producer =  static_cast<type*>((void*)ptr);
			const iex::transport::header* segment = static_cast<iex::transport::header*>(
						(void*)( start + sizeof(iex::pcap::packet)) );

			producer->transport_handler( segment );
		}

		char  errbuf[PCAP_ERRBUF_SIZE];
		const std::string input;
		std::FILE* fd;
};
}

