/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once

#include <cstdint>
#include <functional>
#include <array>
#include <stdexcept>
#include <cstring>
#include <span>
#include <sys/time.h>
#include <bit>
#include <error.hpp>
#include <utils.hpp>
#include <iex.hpp>
#include <zlib-ng.h>

namespace io::stream {

	struct file_t {
		explicit file_t(FILE* fd) : fd(fd) {}

		[[nodiscard]] size_t pull(uint8_t* dst, size_t max) {
			return std::fread(dst, 1, max, fd);
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

namespace iex::pcap {
	struct packet {
		uint8_t ethernet_frame[14];  /*!< Ethernet header: destination MAC, source MAC, EtherType */
		uint8_t ipv4[20];            /*!< IPv4 header (not parsed) */
		uint8_t udp[8];              /*!< UDP header (not parsed) */
	} __attribute__((packed));

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
	struct producer_t : public stream, public transport_t<consumer> {
		using type = producer_t<stream, consumer>;
		using duration = typename consumer::duration;
		using callback_t = std::function<size_t(uint8_t*, size_t)>;

		explicit producer_t(FILE* fd, duration heart_beat)
		: stream(fd) {
			this->heart_beat_interval = heart_beat;

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

			if (global_header.network != 1)
				THROW_RUNTIME_ERROR("unsupported PCAP network type (expected Ethernet)");

			const char* ts_precision = (global_header.magic_number == 0x4d3cb2a1 || global_header.magic_number == 0xa1b23c4d)
				? "nanosecond" : "microsecond";

			INFO << "pcap v" << global_header.version_major << "." << global_header.version_minor
				<< " snaplen " << global_header.snaplen
				<< " linktype " << global_header.network << " (Ethernet) "
				<< (utils::pcap::is_little_endian(global_header.magic_number) ? "little-endian" : "big-endian")
				<< " " << ts_precision << " resolution" << std::endl;
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
					buffer.data() + sizeof(packet));
				this->transport_handler(segment);
			}
			this->end();
		}

	private:

		bool read_exact(uint8_t* dst, size_t len) {
			size_t total = 0;
			while (total < len) {
				size_t n = static_cast<stream*>(this)->pull(dst + total, len - total);
				if (n == 0) return false;
				total += n;
			}
			return true;
		}

		bool needs_byte_swap = false;        /*!< true if host byte order differs from file byte order */
		global_header_t global_header{};     /*!< parsed PCAP global header */
		packet_header_t packet_header{};     /*!< current PCAP packet header */
		std::array<uint8_t, 16384> buffer;   /*!< scratch buffer for captured packet payload */
	};
}  // namespace iex::pcap
