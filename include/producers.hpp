/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include "tick.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <array>
#include <stdexcept>
#include <cstring>
#include <span>
#include <string>
#include <sys/time.h>
#include <bit>
#include <error.hpp>
#include <utils.hpp>
#include <iex.hpp>
#include <vector>
#include <zlib-ng.h>
#include <algorithm>
#include <h5cpp/all>
namespace utils::pcap {
	enum class link_type : uint16_t {
		NULL_LINKTYPE = 0, ETHERNET = 1, TOKEN_RING = 6, ARCNET = 7, SLIP = 8, PPP = 9, FDDI = 10, PPPoE = 50, 
		CISCO_HDLC = 51, ATM_RFC1483 = 100, RAW_IP = 101, IEEE_802_11 = 105, FRAME_RELAY = 113, BLUETOOTH_HCI_H4 = 117,
		USB_LINUX = 119, IEEE_802_15_4 = 122, BLUETOOTH_HCI_H4_WITH_PHDR = 127, LINUX_SLL = 147, LOCALTALK = 148,
		BLUETOOTH_MONITOR = 201, IPV4 = 229, IPV6 = 230, IEEE_802_15_4_NOFCS = 239, DVB_CI = 240, MUX27010 = 241,
		BLUETOOTH_LE_LL = 245, Z_WAVE = 247, IEEE_802_15_9 = 257, BLUETOOTH_MESH = 276, IEEE_1905_1 = 278, DSA_TAG_BRCM = 279,
		IEEE_802_11_RADIOTAP = 280, OPENVSWITCH_DATAPATH = 283, USBPCAP = 284, RTPS = 286, NFC = 287, RTIC = 288,
		LORA = 289, SIGFOX = 291, WI_SUN = 292, DASH7 = 293, NRF_802_15_4 = 294, NR_5G_RRC = 295, NB_IOT = 296, GTP_U = 297,
		E1 = 298, RTP_RTCP = 299, GTPV2_C = 300, PFCP = 301, IEEE_802_1QCP = 302, TSN = 303, TT_ETHERNET = 304, H248 = 305,
		NBMA = 306, G709 = 307, MPLS = 308, LISP = 309, ETNET = 310, IEX_DEEP_v105 = 320, IEX_TOPS_v156 = 321 };

	inline const std::map<link_type, std::string> link_names = {
		{link_type::NULL_LINKTYPE,"Null / No link-layer"},{link_type::ETHERNET,"Ethernet (IEEE 802.3)"},
		{link_type::TOKEN_RING,"Token Ring (IEEE 802.5)"},{link_type::ARCNET,"ARCnet"},{link_type::SLIP,"SLIP"},
		{link_type::PPP,"PPP"},{link_type::FDDI,"FDDI"},{link_type::PPPoE,"PPP over Ethernet"},{link_type::CISCO_HDLC,"Cisco HDLC"},
		{link_type::ATM_RFC1483,"ATM Classical IP"},{link_type::RAW_IP,"Raw IP"},{link_type::IEEE_802_11,"IEEE 802.11"},
		{link_type::FRAME_RELAY,"Frame Relay"},{link_type::BLUETOOTH_HCI_H4,"Bluetooth HCI H4"},{link_type::USB_LINUX,"USB Linux"},
		{link_type::IEEE_802_15_4,"IEEE 802.15.4"},{link_type::BLUETOOTH_HCI_H4_WITH_PHDR,"Bluetooth HCI H4 (w/ pseudo-header)"},
		{link_type::LINUX_SLL,"Linux Cooked Capture"},{link_type::LOCALTALK,"LocalTalk"},{link_type::BLUETOOTH_MONITOR,"Bluetooth Linux Monitor"},
		{link_type::IPV4,"IPv4"},{link_type::IPV6,"IPv6"},{link_type::IEEE_802_15_4_NOFCS,"IEEE 802.15.4 (no FCS)"},
		{link_type::DVB_CI,"DVB-CI"},{link_type::MUX27010,"MUX27010"},{link_type::BLUETOOTH_LE_LL,"Bluetooth LE LL"},
		{link_type::Z_WAVE,"Z-Wave"},{link_type::IEEE_802_15_9,"IEEE 802.15.9"},{link_type::BLUETOOTH_MESH,"Bluetooth Mesh"},
		{link_type::IEEE_1905_1,"IEEE 1905.1"},{link_type::DSA_TAG_BRCM,"Broadcom DSA Tag"},{link_type::IEEE_802_11_RADIOTAP,"802.11 RadioTap"},
		{link_type::OPENVSWITCH_DATAPATH,"Open vSwitch Datapath"},{link_type::USBPCAP,"USBPcap"},{link_type::RTPS,"RTPS"},
		{link_type::NFC,"NFC"},{link_type::RTIC,"RTIC"},{link_type::LORA,"LoRa"},{link_type::SIGFOX,"Sigfox"},
		{link_type::WI_SUN,"Wi-SUN"},{link_type::DASH7,"Dash7"},{link_type::NRF_802_15_4,"NRF 802.15.4"},
		{link_type::NR_5G_RRC,"5G NR RRC"},{link_type::NB_IOT,"NB-IoT"},{link_type::GTP_U,"GTP-U"},{link_type::E1,"E1"},
		{link_type::RTP_RTCP,"RTP / RTCP"},{link_type::GTPV2_C,"GTPv2-C"},{link_type::PFCP,"PFCP"},{link_type::IEEE_802_1QCP,"IEEE 802.1Qcp"},
		{link_type::TSN,"Time-Sensitive Networking"},{link_type::TT_ETHERNET,"TTEthernet"},{link_type::H248,"H.248"},
		{link_type::NBMA,"NBMA"},{link_type::G709,"ITU-T G.709"},{link_type::MPLS,"MPLS"},{link_type::LISP,"LISP"},
		{link_type::ETNET,"DetNet"},{link_type::IEX_DEEP_v105,"IEX DEEP v1.05"},{link_type::IEX_TOPS_v156,"IEX TOPS v1.56"}
	};
}

namespace io::stream {
	struct file_t {
		explicit file_t(FILE* fd) : fd(fd) {}

		[[nodiscard]] size_t pull(uint8_t* dst, size_t max) {
			return std::fread(dst, 1, max, fd);
		}
		[[nodiscard]] size_t peek(uint8_t* dst, size_t len) {
			if (!fd || !dst || len == 0)
				return 0;

			fpos_t pos;
			if (fgetpos(fd, &pos) != 0)
				return 0;

			size_t n = std::fread(dst, 1, len, fd);
			fsetpos(fd, &pos);
			return n;
		}

		static uint32_t peek(FILE* fd) {
			fpos_t pos;
			uint32_t magic = 0;
			if (!fd) THROW_RUNTIME_ERROR("null FILE* passed to file_t::peek");
			if (fgetpos(fd, &pos) != 0)
				THROW_RUNTIME_ERROR("fgetpos failed in file_t::peek");
			if (std::fread(&magic, sizeof(magic), 1, fd) != 1)
				THROW_RUNTIME_ERROR("fread failed in file_t::peek");
			if (fsetpos(fd, &pos) != 0)
				THROW_RUNTIME_ERROR("fsetpos failed in file_t::peek");

			return magic;
		}

	private:
		FILE* fd; /*!< underlying file descriptor, not owned */
	};
	
	struct gzip_t {
		explicit gzip_t(FILE* fd) : fd(fd) {
			if (zng_inflateInit2(&strm, 31) != Z_OK)
				THROW_RUNTIME_ERROR("zng_inflateInit2 failed");
		}

		~gzip_t() {
			zng_inflateEnd(&strm);  // wraps zng_inflateEnd for compatibility
		}

		[[nodiscard]] size_t pull(uint8_t* dst, size_t len) {
			size_t total = 0;
			while (total < len) {
				if (decompressed_pos == decompressed_end)
					if (!replenish()) break;

				size_t available = decompressed_end - decompressed_pos;
				size_t n = std::min(len - total, available);
				std::memcpy(dst + total, decompressed_buffer.data() + decompressed_pos, n);
				decompressed_pos += n;
				total += n;
			}
			return total;
		}
		[[nodiscard]] size_t peek(uint8_t* dst, size_t len) {
			if (decompressed_pos == decompressed_end && !replenish())
				return 0;

			size_t available = decompressed_end - decompressed_pos;
			size_t n = std::min(len, available);
			std::memcpy(dst, decompressed_buffer.data() + decompressed_pos, n);
			std::rewind(fd);
			return n;
		}

		static uint32_t peek(FILE* fd) {
			if (!fd) THROW_RUNTIME_ERROR("null FILE* in gzip_t::peek");

			stream::gzip_t gz(fd);  // temp gzip stream
			uint32_t magic = 0;
			if (gz.peek(reinterpret_cast<uint8_t*>(&magic), sizeof(magic)) != sizeof(magic))
				THROW_RUNTIME_ERROR("failed to read magic from gzip");

			return magic;
		}

		bool replenish() {
			if (stream_ended) return false;

			// Pull compressed input
			strm.avail_in = std::fread(compressed_buffer.data(), 1, compressed_buffer.size(), fd);
			if (ferror(fd))
				THROW_RUNTIME_ERROR("fread failed while decompressing gzip");
			if (strm.avail_in == 0) {
				stream_ended = true;
				return false;
			}
			strm.next_in = compressed_buffer.data();

			strm.avail_out = decompressed_buffer.size();
			strm.next_out = decompressed_buffer.data();

			int ret = zng_inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_STREAM_END) stream_ended = true;
			else if (ret != Z_OK && ret != Z_BUF_ERROR)
				THROW_RUNTIME_ERROR("zng_inflate failed: " + std::to_string(ret));

			decompressed_pos = 0;
			decompressed_end = decompressed_buffer.size() - strm.avail_out;
			return decompressed_end > 0;
		}

	private:
		FILE* fd; /*!< backing compressed input stream (not owned) */
		bool stream_ended = false; /*!< true if end-of-stream was reached */
		std::array<uint8_t, 1 << 16> compressed_buffer;   /*!< input buffer (65 KiB) */
		std::array<uint8_t, 1 << 20> decompressed_buffer; /*!< output buffer (1 MiB) */
		size_t decompressed_pos = 0;  /*!< current read offset into decompressed buffer */
		size_t decompressed_end = 0;  /*!< end of valid decompressed data */
		zng_stream strm{}; /*!< zlib-ng decompression state */
	};
}

namespace iex::base {
	struct packet {
		uint8_t ethernet_frame[14];  /*!< Ethernet header: destination MAC, source MAC, EtherType */
		uint8_t ipv4[20];            /*!< IPv4 header (not parsed) */
		uint8_t udp[8];              /*!< UDP header (not parsed) */
	} __attribute__((packed));


	template <class stream, class consumer>
	struct producer_t : public stream, public transport_t<consumer> {
		using type = producer_t<stream, consumer>;
		using duration = typename consumer::duration;
	
	explicit producer_t(FILE* fd, duration heart_beat)
	: stream(fd) {
		this->heart_beat_interval = heart_beat;
	}

	protected:
		bool read_exact(uint8_t* dst, size_t len) {
			size_t total = 0;
			while (total < len) {
				size_t n = static_cast<stream*>(this)->pull(dst + total, len - total);
				if (n == 0) return false;
				total += n;
			}
			return true;
		}

		void check_compatibility(std::string protocol){
			INFO << protocol << " v" << version_major << "." << version_minor << " snaplen " << snap_length
				<< " linktype " << static_cast<uint16_t>(link_type) << " " << utils::pcap::link_names.at(link_type) 
				<< (is_little_endian ? " little-endian" : " big-endian") << std::endl;
			if(!is_little_endian)	THROW_RUNTIME_ERROR("only little endian is supported");
			if(link_type != utils::pcap::link_type::ETHERNET) THROW_RUNTIME_ERROR("this link type is not supported...");
		}

		bool needs_byte_swap = false;        /*!< true if host byte order differs from file byte order */
		bool is_little_endian = false;
		uint32_t version_minor = 0, version_major = 0, snap_length = 0, packet_count = 0;
		std::array<uint8_t, 16384> buffer;   /*!< scratch buffer for captured packet payload */
		utils::pcap::link_type link_type = utils::pcap::link_type::NULL_LINKTYPE;
	};
}

namespace iex::pcap {
	struct global_header_t {
		uint32_t magic_number;     /*!< Magic number used to detect byte order and timestamp resolution */
		uint16_t version_major;    /*!< Major version number (typically 2) */
		uint16_t version_minor;    /*!< Minor version number (typically 4) */
		int32_t thiszone;          /*!< GMT to local time correction (usually zero) */
		uint32_t sigfigs;          /*!< Accuracy of timestamps (not used) */
		uint32_t snaplen;          /*!< Max length of captured packets, in octets */
		uint32_t network;          /*!< Data link type (1 = Ethernet) */
	} __attribute__((packed));

	struct packet_header_t {
		uint32_t ts;        /**< Timestamp: seconds since Unix epoch */
		uint32_t ns;        /**< Timestamp: sub-second precision (micro or nanoseconds) */
		uint32_t captured;  /**< Number of bytes actually captured (≤ snaplen) */
		uint32_t original;  /**< Original length of the packet on the wire */
	} __attribute__((packed));

	template <class stream, class consumer>
	struct producer_t : public base::producer_t<stream, consumer> {
		using parent = base::producer_t<stream, consumer>;
		using duration = typename consumer::duration;
		using parent::needs_byte_swap, parent::read_exact, parent::buffer, parent::is_little_endian, parent::packet_count,
			parent::link_type, parent::version_major, parent::version_minor, parent::snap_length, parent::check_compatibility;

		explicit producer_t(FILE* fd, duration hb) : parent(fd, hb) {
			read_exact(reinterpret_cast<uint8_t*>(&global_header), sizeof(global_header));
			if (!utils::pcap::is_valid_magic(global_header.magic_number))
				THROW_RUNTIME_ERROR("Invalid PCAP magic number: " + std::to_string(global_header.magic_number));

			this->needs_byte_swap = utils::pcap::needs_byteswap(global_header.magic_number);
			if (this->needs_byte_swap) {
				global_header.version_major = std::byteswap(global_header.version_major);
				global_header.version_minor = std::byteswap(global_header.version_minor);
				global_header.thiszone      = std::byteswap(global_header.thiszone);
				global_header.sigfigs       = std::byteswap(global_header.sigfigs);
				global_header.snaplen       = std::byteswap(global_header.snaplen);
				global_header.network       = std::byteswap(global_header.network);
			}

			link_type = static_cast<utils::pcap::link_type>(global_header.network);
			version_major = global_header.version_major, version_minor = global_header.version_minor, 
			snap_length = global_header.snaplen, is_little_endian = utils::pcap::is_little_endian(global_header.magic_number);
			check_compatibility("pcap");
		}

		void run_impl() {
			while (read_exact(reinterpret_cast<uint8_t*>(&packet_header), sizeof(packet_header))) {
				if (packet_header.captured > buffer.size())
					THROW_RUNTIME_ERROR(
						"packet too large: " + std::to_string(packet_header.captured) +
						" buffer: " + std::to_string(buffer.size()));

				if (!read_exact(buffer.data(), packet_header.captured))
					break;  // EOF

				const iex::transport::header* segment = reinterpret_cast<const iex::transport::header*>(
					buffer.data() + sizeof(iex::base::packet));
				this->transport_handler(segment);
				packet_count++;
			}
			this->end();
		}

		global_header_t global_header{};     /*!< parsed PCAP global header */
		packet_header_t packet_header{};     /*!< current PCAP packet header */
	};
}  // namespace iex::pcap

namespace iex::pcapng {
	enum class block_type : uint32_t {
		SECTION_HEADER        = 0x0A0D0D0A, //!< Section Header Block (SHB)
		INTERFACE_DESCRIPTION = 0x00000001, //!< Interface Description Block (IDB)
		PACKET                = 0x00000002, //!< Obsolete: Simple Packet Block (SPB)
		NAME_RESOLUTION       = 0x00000004, //!< Name Resolution Block (NRB)
		INTERFACE_STATS       = 0x00000005, //!< Interface Statistics Block (ISB)
		ENHANCED_PACKET       = 0x00000006, //!< Enhanced Packet Block (EPB)
		UNKNOWN               = 0xFFFFFFFF  //!< Fallback or invalid block type
	};
	struct block_header_t {
		uint32_t block_type;
		uint32_t block_total_length;
	} __attribute__((packed));

	struct shb_t {
		uint32_t byte_order_magic;
		uint16_t version_major;
		uint16_t version_minor;
		int64_t  section_length;
	} __attribute__((packed));

	struct idb_t {
		uint16_t link_type;
		uint16_t reserved;
		uint32_t snaplen;
	} __attribute__((packed));

	struct epb_t {
		uint32_t interface_id;
		uint32_t ts_high;
		uint32_t ts_low;
		uint32_t captured_len;
		uint32_t original_len;
	} __attribute__((packed));

	template <class stream, class consumer>
	struct producer_t : public base::producer_t<stream, consumer> {
		using parent = base::producer_t<stream, consumer>;
		using duration = typename consumer::duration;
		using parent::needs_byte_swap, parent::read_exact, parent::buffer, parent::is_little_endian, parent::packet_count,
			parent::link_type, parent::version_major, parent::version_minor, parent::snap_length, parent::check_compatibility;

		explicit producer_t(FILE* fd, duration hb) : parent(fd, hb) {
		}

		void run_impl() {
			while (true) {
				if (!this->read_exact(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr))) break;
				if (!this->read_exact(buffer.data(), hdr.block_total_length - sizeof(hdr)))
					THROW_RUNTIME_ERROR("Failed to read complete block body");

				switch(static_cast<block_type>(hdr.block_type)) {
					case block_type::SECTION_HEADER:  // already verifies `magic`
						shb = reinterpret_cast<shb_t*>(buffer.data());
						needs_byte_swap = utils::pcapng::needs_byteswap(shb->byte_order_magic);
						version_major = shb->version_major, version_minor = shb->version_minor;
						is_little_endian = utils::pcapng::is_little_endian(shb->byte_order_magic);
					break;
					case block_type::INTERFACE_DESCRIPTION:
						idb = reinterpret_cast<idb_t*>(buffer.data()), snap_length = idb->snaplen,
						link_type = static_cast<utils::pcap::link_type>(idb->link_type);
						check_compatibility("pcap-ng");
					break;
					case block_type::ENHANCED_PACKET: {
						const epb_t* epb = reinterpret_cast<const epb_t*>(buffer.data());
						if(epb->captured_len != epb->original_len)
							TRACE << epb->captured_len << " " << epb->original_len << std::endl;
						const iex::transport::header* segment = reinterpret_cast<const iex::transport::header*>(
							buffer.data() + sizeof(epb_t) + sizeof(iex::base::packet));
						this->transport_handler(segment);
						break;
					}
					default: ;
				}
			}
			this->end();
		}

		uint32_t trailing_length = 0, trailer = 0;
		block_header_t hdr;
		shb_t* shb;
		idb_t* idb;
	};
} // namespace iex::pcapng


namespace h5 {
	template <class consumer_t> struct producer_t {
		using clock      = typename consumer_t::clock;
		using duration   = typename clock::duration;
		using time_point = typename clock::time_point;

		producer_t(std::string path, std::pair<std::string, std::string> date, duration interval) : heart_beat_interval(interval.count()) {
			INFO << date.first << " " << date.second << std::endl;
			auto start = ::utils::string_to_day(date.first), stop = ::utils::string_to_day(date.second);
			try {
				fd = h5::open(path, H5F_ACC_SWMR_READ);
				if ( H5Lexists(fd, "/irts", H5P_DEFAULT) > 0)
					for(std::string day: h5::ls(fd, "/irts")) {
						auto today = ::utils::string_to_day(day);
						if (today >= start && today <= stop) trading_days.push_back(day); 
					}
			} catch(h5::error::any err){
				ERROR << err.what() << std::endl;
			}
			INFO << "selected trading days: " <<  trading_days.size() << std::endl;
		}
		
		void run(consumer_t& consumer, duration start, duration stop) {
			using nanoseconds = std::chrono::nanoseconds;
			if ( H5Lexists(fd, instruments_path.data(), H5P_DEFAULT) > 0) {
				instruments = h5::read<std::vector<std::string>>(fd, instruments_path);
				consumer.batch_insert(instruments);
			}
			
			for(std::string day: trading_days) {
				const time_point today = ::utils::string_to_day(day),
					start_time = today + start, stop_time = today + stop;
				const uint64_t start_tick = ::utils::to_ns(start_time), stop_tick = ::utils::to_ns(stop_time);
				uint64_t hb = start_tick + heart_beat_interval;
				consumer.day_begin( start_time );
				h5::ds_t ds = h5::open(fd, "/irts/" + day);
				for(const iex::tick_t& tick: h5::view<iex::tick_t>(ds) ) {
					if(tick.time <= start_tick) continue;
					if(tick.time >= stop_tick) break;
					if(tick.time >= hb)
						consumer.heart_beat( time_point{nanoseconds{hb}} ),
						hb += heart_beat_interval;

					const time_point now = time_point{nanoseconds{tick.time}};
					switch(tick.flags){
						case IS_BID: consumer.on_bid(now, tick.contract_id, tick.price, tick.size, 0); break;
						case IS_TRADE: consumer.on_trade_report(now, tick.contract_id, tick.price, tick.size, 0); break;
						case IS_ASK: consumer.on_ask(now, tick.contract_id, tick.price, tick.size, 0); break;
						default: [[unlikely]]
							WARNING << "unexpected flag: " << tick.flags << std::endl;
						break;
					}
				}
				consumer.heart_beat( time_point{nanoseconds{hb}} );
				consumer.day_end( stop_time );
			}
		}
		
	private:
		const std::string instruments_path = "instruments.txt";
		const uint64_t heart_beat_interval;
		h5::fd_t fd;
		std::vector<std::string> trading_days, instruments;
		static constexpr uint16_t IS_BID = 1<<0, IS_TRADE = 1<<1, IS_ASK = 1<<2;
	};
}

