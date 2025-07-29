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
#include <csignal>
#include <atomic>

#include <argparse>
#include <error.hpp>
#include <utils.hpp>
#include <patterns.hpp>
#include <producers.hpp>
#include <consumers.hpp>
#include <csv.hpp>
#include <base64.hpp>
#include <threadpool.hpp>
#include <io.hpp>
#include <licenses.hpp>

#ifndef IEX_MAX_SYMBOLS
	#define IEX_MAX_SYMBOLS 1 << 16
#endif
void signal_handler(int signal) {
	INFO << "received signal: " << signal << ", initiating shutdown..." << std::endl;
	global::state::shutdown_requested.store(true);
}

int main(int argc, char **argv) {
	namespace fs = std::filesystem;
	namespace ch = std::chrono;
	using std::cout, std::cerr, std::endl;

	std::string output_path_or_url, rts_path, instruments_path, trading_days_path, days, interval, start, stop, convert,
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
		cout << "\033[1m[iex2h5]\033[0m Market data © IEX — Investors Exchange. Attribution required. See https://iextrading.com" << std::endl;
		cout << copyright << endl << endl;
		std::exit(0);
	})
	.default_value(false)
	.help("shows help message")
	.implicit_value(true)
	.nargs(0);
	
	program.add_argument("--version").help("Print version information").default_value(false).implicit_value(true)
	.nargs(0).action([&](const std::string&) {
        cout << "\n\033[1m" << argv[0] << "\033[0m" << " " << version << "\n\n"
		<< "IEX2H5: High-performance IEX market data importer\n"
		<< "Converts DEEP/TOPS pcap captures into HDF5 datasets\n\n"
		<< "Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada   info@vargaconsulting.ca\n"
		<< "All rights reserved. info@vargaconsulting.ca\n"
		<< "Licensed under the MIT License.\n" << endl;
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
	
	program.add_argument("-c", "--convert").default_value(std::string("all")).choices("rts", "irts", "all", "none").help("Which conversion pipeline to run: rts | irts | none | all");
	program.add_argument("remaining").remaining();
	program.add_argument("--third-party-licenses").nargs('*').default_value(std::vector<std::string>{"all"}).implicit_value("all")
		.help("Print license(s) for a third-party library (or 'all | license 01 [, license 02, ...]')");

	try {
		program.parse_args(argc, argv);
	} catch (const std::exception& err){
		std::cerr << err.what() << std::endl;
		return 1;
	}
	if (program.is_used("--third-party-licenses")) {
		std::map<std::string, std::string> license_map;
		for (const auto& [name, _, content] : licenses::thirdparty)
			license_map[name] = content;
		if (program.is_used("remaining") ){
			std::vector remaining = program.get<std::vector<std::string>>("remaining");
			if(remaining.empty() || remaining.front() == "all") for (const auto& [name, text] : license_map)
				std::cout << "\n=== " << name << " ===\n" << text << "\n";
			else for(std::string key: remaining) {
				auto it = license_map.find(key);
				if (it != license_map.end())
					std::cout << "\n=== " << it->first << " ===\n" << it->second << "\n";
				else {
					std::cout << "Unknown license: " << key << "\n";
					std::cout << "Valid licenses:\n";
					for (const auto& [name, _] : license_map)
						std::cerr << "  - " << name << "\n";
					return 1;				
				}
			}
		} else {
			std::cout<< "Please choose one of the following: ";
			for (auto it = license_map.begin(); it != license_map.end(); ++it) {
				std::cout << it->first;
				if (std::next(it) != license_map.end())
					std::cout << ", ";
			}
			std::cout << std::endl << std::endl;
		}
		return 0;
	}

	std::signal(SIGINT, signal_handler); std::signal(SIGTERM, signal_handler);
	std::signal(SIGHUP, signal_handler); std::signal(SIGQUIT, signal_handler);
	
	h5::mute();
    try {
		std::tie(interval, start, stop, output_path_or_url, rts_path, instruments_path, trading_days_path, compression_level, convert) = std::make_tuple(
			program.get<std::string>("--time-interval"), program.get<std::string>("--start"), program.get<std::string>("--stop"),
			program.get<std::string>("--output"),
			program.get<std::string>("--rts-path"), program.get<std::string>("--instruments-path"), program.get<std::string>("trading-days-path"),
			program.get<unsigned>("--gzip"), program.get<std::string>("--convert"));

		bool is_irts_enabled = (convert == "all" || convert =="irts"),
			is_rts_enabled = (convert == "all" || convert =="rts");
		std::string dispatch = file::detect_format(output_path_or_url);
		std::vector<std::string> files = utils::resolve_input_paths(program.get<std::vector<std::string>>("remaining"));
		
		if(files.size()) {
			cout << "\033[1m[iex2h5]\033[0m Converting " << files.size()
			<< " file" << (files.size() > 1 ? "s" : "") 
			<< " using backend: " << dispatch << " — using 1 thread — © Varga Consulting, 2017–2025\n"
			<< "\033[1m[iex2h5]\033[0m Visit \033[4mhttps://vargaconsulting.github.io/iex2h5/\033[0m — Star it, Share it, Support Open Tools ⭐️\n";
			
			std::map<std::string, std::function<void()>> execute {
				{"hdf5", io::create<io::hdf5::consumer_t>(files, start, interval, stop, output_path_or_url, rts_path, instruments_path, trading_days_path, is_irts_enabled, is_rts_enabled, compression_level)},
				{"csv", io::create<io::csv::consumer_t>(files, start, interval, stop, output_path_or_url, instruments_path, trading_days_path, is_irts_enabled, is_rts_enabled)}					
			};
			if(!execute.contains(dispatch))
				std::cerr << "[iex2h5] error: unknown dispatch backend: " << dispatch << std::endl;
			else try {
				execute[dispatch]();
				cout << "\033[1m[iex2h5]\033[0m Conversion complete — all files processed successfully \n"
				"\033[1m[iex2h5]\033[0m Market data © IEX — Investors Exchange. Attribution required. See https://iextrading.com" << endl;
			} catch (const global::shutdown_exception& ex){
				cout << "\n\033[1m[iex2h5]\033[0m Conversion interrupted \n"
				"\033[1m[iex2h5]\033[0m Market data © IEX — Investors Exchange. Attribution required. See https://iextrading.com" << endl;				
			}
		}
	} catch( const std::exception& err ) {
		cerr << err.what() << endl;
        cerr << program << endl;
		return 1;
    }
	h5::unmute();
    return 0;
}
