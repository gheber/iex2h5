/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#include <sstream>
#include <string>
#include <map>
#include <functional>
#include <iostream>
#include <ranges>
#include <string>
#include <filesystem>

#include <argparse>
#include <error.hpp>
#include <utils.hpp>
#include <patterns.hpp>
#include <producers.hpp>
#include <consumers.hpp>
#include <base64.hpp>
#include <threadpool.hpp>
#include <io.hpp>

#ifndef IEX_MAX_SYMBOLS
	#define IEX_MAX_SYMBOLS 1 << 16
#endif
using namespace std;

int main(int argc, char **argv) {
	namespace fs = std::filesystem;
	namespace ch = std::chrono;
	std::string hdf5_path, rts_path, instruments_path, trading_days_path, days, interval, start, stop, convert,
		copyright = "Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada   info@vargaconsulting.ca";

    unsigned compression_level;
	std::string version( "\033[1m" IEX2H5_SOFTWARE_VERSION "\033[0m" " commit: "  IEX2H5_SOFTWARE_COMMIT_HASH);
	argparse::ArgumentParser program(argv[0], version, argparse::default_arguments::none);
	program.add_argument("-h", "--help")
	.action([&](const std::string& s) {
		cout << "\033[1m" "IEX2H5 converts IEX TOPS Datasets to HDF5 Format" "\033[0m" << endl << endl;
		cout << "iex2h5 is a specialized tool for importing IEX TOPS datasets into the HDF5 data format," << endl;
		cout << "enabling efficient  storage and analysis of large  financial datasets. HDF5 is a widely" << endl;
		cout << "used file format for handling large, complex, and hierarchical data, supported by major" << endl;
		cout << "programming languages including Julia, Python, MATLAB, C, C++, and Node.js." << endl;
		cout << endl;	
		cout << "This application allows users to convert captured packet data streams (e.g., DEEP/TOPS)" << endl;
		cout << "into structured HDF5 datasets for advanced analytics and seamless integration into" << endl;
		cout << "scientific, engineering, and financial workflows." << endl << endl;
		
		cout << program << endl << endl;
		
		cout << "\033[1m" "example:" "\033[0m" <<endl;
		cout << "   " << argv[0] << " --time-interval 10 -o ~/iex.h5  ~/data/**/*.pcap.gz" << endl;
		cout << endl;
		cout << "This program uses the HDF5 library via dynamic linking. HDF5 is © The HDF Group and licensed under a BSD-style license." << endl;
		cout << copyright << endl << endl;
		std::exit(0);
	})
	.default_value(false)
	.help("shows help message")
	.implicit_value(true)
	.nargs(0);
	
	program.add_argument("--version").help("Print version information").default_value(false).implicit_value(true)
	.nargs(0).action([&](const std::string&) {
        std::cout << "\n\033[1m" << argv[0] << "\033[0m"
		<< " " << version << "\n\n"
		<< "IEX2H5: High-performance IEX market data importer\n"
		<< "Converts DEEP/TOPS pcap captures into HDF5 datasets\n\n"
		<< "This program uses the HDF5 library via dynamic linking. HDF5 is © The HDF Group and licensed under a BSD-style license.\n"
		<< "Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada   info@vargaconsulting.ca\n"
		<< "All rights reserved. info@vargaconsulting.ca\n"
		<< "Licensed under the MIT License.\n"

		<< std::endl;
        std::exit(0);
    });
	program.add_argument("--time-interval").default_value(std::string("00:01:00")).help("temporal interval in hh::mm::ss format, irts stream is converted into");
	program.add_argument("--start").default_value(std::string("14:30:00")).help("lower bound in UTC, considers events only after");
	program.add_argument("--stop").default_value(std::string("21:00:00")).help("upper bound in UTC, considers events only before");
	
	program.add_argument("-o", "--output").default_value(std::string("./iex.h5")).help("path to the HDF5 container");
	
	program.add_argument("--rts-path").default_value(std::string("/time.txt")).help("hdf5-group/directory for regular time interval index");
	program.add_argument("--instruments-path").default_value(std::string("/instruments.txt")).help("hdf5-group/directory for listed [symbols|assets|financial] instruments");
	program.add_argument("--trading-days-path").default_value(std::string("/trading_days.txt")).help("hdf5-group/directory for active trading days");
	program.add_argument("-g", "--gzip").default_value(static_cast<unsigned>(1)).scan<'u', unsigned>().help("0-9 0 for no compression, 9 for highest");
	
	program.add_argument("-c", "--convert").default_value(std::string("all")).choices("rts", "irts", "all", "none").help("Which conversion pipeline to run: rts | irts | all (default)");
	program.add_argument("files").remaining();

	try {
		program.parse_args(argc, argv);
	} catch (const std::exception& err){
		std::cerr << err.what() << std::endl;
		return 1;
	}
		
	h5::mute();
	
    try {
		std::tie(interval, start, stop, hdf5_path, rts_path, instruments_path, trading_days_path, compression_level, convert) = std::make_tuple(
			program.get<std::string>("--time-interval"), program.get<std::string>("--start"), program.get<std::string>("--stop"),
			program.get<std::string>("--output"),
			program.get<std::string>("--rts-path"), program.get<std::string>("--instruments-path"), program.get<std::string>("trading-days-path"),
			program.get<unsigned>("--gzip"), program.get<std::string>("--convert"));

		bool is_irts_enabled = (convert == "all" | convert =="irts"),
			is_rts_enabled = (convert == "all" | convert =="rts");
		using consumer = io::rts::consumer_t;
		using duration = typename consumer::duration;

		h5::fd_t fd;
		h5::ds_t ds;
		h5::dcpl_t dcpl = (compression_level != 0) ?  h5::gzip{compression_level} : h5::default_dcpl;

		try {
			fd = h5::open(hdf5_path, H5F_ACC_RDWR);
		} catch (const h5::error::any& err){
			fd = h5::create(hdf5_path, H5F_ACC_TRUNC);
		}

		std::vector<std::string> instruments, rts;
		if (H5Lexists(fd, instruments_path.data(), H5P_DEFAULT) > 0) {
			ds = h5::open(fd, instruments_path);
			instruments = h5::read<std::vector<std::string>>(fd, instruments_path);
		} else ds = h5::create<std::string>(fd, instruments_path, h5::current_dims{0}, h5::max_dims{IEX_MAX_SYMBOLS}, h5::chunk{512}| h5::gzip{9}); 
		io::base::consumer_t<consumer>::batch_insert(instruments);

		auto load_or_create_rts = [&]() -> std::vector<std::string> {
			h5::ds_t ds = H5Lexists(fd, rts_path.data(), H5P_DEFAULT) <= 0
				? h5::write(fd, rts_path, utils::sequence<ch::seconds>(start, interval, stop))
				: h5::open(fd, rts_path);
			return h5::read<std::vector<std::string>>(ds);
		};
		if (is_rts_enabled)
			rts = load_or_create_rts();

		std::vector<std::string> files = utils::resolve_input_paths(program.get<std::vector<std::string>>("files"));
		if(files.size()) {
			std::cout << "\033[1m[iex2h5]\033[0m Converting " << files.size()
			<< " file" << (files.size() > 1 ? "s" : "") 
			<< " into HDF5: using 1 thread — © Varga Consulting, 2017–2025\n"
			<< "\033[1m[iex2h5]\033[0m Visit \033[4mhttps://vargaconsulting.github.io/iex2h5/\033[0m — Star it, Share it, Support Open Tools ⭐️\n";

			for(std::string path: files) try {
				io::task<consumer>(path, start, interval, stop, fd, dcpl, rts, is_irts_enabled, is_rts_enabled)();
			} catch (const std::exception& ex) {
				std::cerr << "[error] task threw exception: " << ex.what() << '\n';
			} catch (...) {
				std::cerr << "[error] task threw unknown exception\n";
			}
					
			// condionally update trading days, given there has been RTS data processed
			if (H5Lexists(fd, "stats", H5P_DEFAULT) > 0) try {
				std::vector<std::string> active_days = h5::ls(fd, "stats");
				h5::ds_t ds;
				if (H5Lexists(fd, trading_days_path.data(), H5P_DEFAULT) > 0)
					ds = h5::open(fd, trading_days_path);
				else ds = h5::create<std::string>(fd, trading_days_path, h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{512}| h5::gzip{9});
				h5::set_extent(ds, h5::current_dims{active_days.size()});
				h5::write(ds, active_days, h5::offset{0}, h5::count{active_days.size()});
			} catch(const h5::error::any& err) {}

			const auto& all_contracts = global::state::flat_map;
			std::vector<std::string> asset_names(all_contracts.size());
			TRACE << "instruments: " << all_contracts.size() << std::endl;
			for(uint64_t contract: all_contracts) {
				auto[symbol, index] = utils::base64::decode(contract);
				if(index >= asset_names.size())
					throw std::runtime_error("Decoded index out of bounds.");
				asset_names[index] = utils::trim(symbol);
			}
			TRACE << "asset decoding has been completed" << std::endl;
			if (H5Fflush(fd, H5F_SCOPE_GLOBAL) < 0)
				THROW_RUNTIME_ERROR("hdf5 flush has failed...");
			if(instruments.size() != all_contracts.size()) try {
				h5::set_extent(ds, h5::current_dims{asset_names.size()});
				h5::write(fd, instruments_path, asset_names, h5::offset{0}, h5::count{asset_names.size()});
			} catch(const h5::error::any& err) {
				ERROR << err.what() << std::endl;
			} else INFO << "symbol/contract table has not changed, total: " << all_contracts.size() << std::endl;
			std::cout << "\033[1m[iex2h5]\033[0m Conversion complete — all files processed successfully, total contracts:  " << all_contracts.size() << "\n"
			"\033[1m[iex2h5]\033[0m This software uses the HDF5 library — © The HDF Group — BSD-licensed\n"
			"\033[1m[iex2h5]\033[0m Market data © IEX — Investors Exchange. Attribution required. See https://iextrading.com" << std::endl;			
		}
	} catch( const std::exception& err ) {
		cout << err.what() << endl;
        cout << program << endl;
		return 1;
    }
	h5::unmute();
    return 0;
}
