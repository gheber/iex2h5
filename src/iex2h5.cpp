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
#include <sigma/error.hpp>
#include <vector>
#include <iostream>
#include <argparse/all>

using namespace std;
// declarations from *.cpp files
void initialise( const std::string input, const std::string output,
		  const std::string trading_days_path, const std::string instruments_path,const std::string rts_path,
		  const std::string day_begin, const std::string day_end,  unsigned long interval );

void generate_irts( const std::string input, const std::string output,
		  const std::string trading_days_path, const std::string instruments_path, const std::string rts_path,
		  const std::string day_begin, const std::string day_end,  unsigned long interval );

void generate_rts( const std::string  input, const std::string output,
		  const std::string trading_days_path, const std::string instruments_path,const std::string rts_path,
		  const std::string day_begin, const std::string day_end,  unsigned long interval );

void generate_index( const std::string input, const std::string output,
		  const std::string trading_days_path, const std::string instruments_path,const std::string rts_path,
		  const std::string day_begin, const std::string day_end,  unsigned long interval );

using command = void(const std::string, const std::string, const std::string, const std::string,
	const std::string, const std::string, const std::string,  unsigned long);

int main(int argc, char **argv) {

	std::string output, stream,
		rts_path, instruments_path, trading_days_path, days, day_begin, day_end, cmd;
    unsigned time_interval, gzip;
	argparse::ArgumentParser program(argv[0], "1.0.1", argparse::default_arguments::none);

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
		cout << "   unpigz -c tops.pcap.gz | " << argv[0] << " -g 9 --time-interval 10 --command init" << endl;
		cout << "   for file in repo/*.pcap.gz; do unpigz -c ${file} | iex2h5 -g 9 --command rts -o ${HOME}/iex.h5;" << endl << endl;

		cout << "Copyright © <2017-2025> Varga Consulting, Toronto, ON, info@vargaconsulting.ca" << endl << endl;
		std::exit(0);
	  })
	  .default_value(false)
	  .help("shows help message")
	  .implicit_value(true)
	  .nargs(0);

	std::map<std::string,std::function<command>> dispatch;
	dispatch["init"] 	= initialise;
	dispatch["irts"] 	= generate_irts;
	dispatch["rts"] 	= generate_rts;
	dispatch["index"] 	= generate_index;

	program.add_argument("--time-interval").default_value(static_cast<unsigned>(10)).scan<'u', unsigned>().help("temporal interval in seconds, irts stream is converted into");
	program.add_argument("--start").default_value(std::string("14:30:00")).help("lower bound in UTC, considers events only after");
	program.add_argument("--stop").default_value(std::string("21:00:00")).help("upper bound in UTC, considers events only before");
	
	program.add_argument("-o", "--output").default_value(std::string("./iex.h5")).help("path to the HDF5 container");
	
	program.add_argument("--rts-path").default_value(std::string("/time.txt")).help("hdf5-group/directory for regular time interval index");
	program.add_argument("--instruments-path").default_value(std::string("/instruments.txt")).help("hdf5-group/directory for listed [symbols|assets|financial] instruments");
	program.add_argument("--trading-days-path").default_value(std::string("/trading_days.txt")).help("hdf5-group/directory for active trading days");
	
	program.add_argument("-g", "--gzip").default_value(static_cast<unsigned>(0)).scan<'u', unsigned>().help("0-9 0 for no compression, 9 for highest");
	program.add_argument("-c", "--command").default_value(std::string("rts")).help(
		"init  - intitialises hdf5 container with retrieved symbols from irts/stream\n"
		"irts  - saves captured events as irts stream\n"
		"rts   - converts irts to rts\n"
		"index - scans and rebuilds trading day index\n"
		"\n");
	try {
		program.parse_args(argc, argv);
	} catch (const std::exception& err){
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		return 1;
	}

    try {
		std::tie(time_interval, day_begin, day_end, output, trading_days_path, instruments_path, rts_path, gzip, cmd) = std::make_tuple(
			program.get<unsigned>("--time-interval"), program.get<std::string>("--start"), program.get<std::string>("--stop"),
			program.get<std::string>("--output"),
			program.get<std::string>("trading-days-path"), program.get<std::string>("--instruments-path"), program.get<std::string>("--rts-path"),
			program.get<unsigned>("--gzip"), program.get<std::string>("--command"));
		dispatch[cmd]("", output, trading_days_path, instruments_path, rts_path, day_begin, day_end, time_interval );
	} catch( const std::exception& err ) {
		cout << err.what() << endl;
        cout << program << endl;
		return 1;
    }
    return 0;
}
