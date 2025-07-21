/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

#pragma once
#include "consumers.hpp"
#include <fstream>
#include <iomanip>
#include <filesystem>

namespace io::csv {
    struct consumer_t : public io::base::consumer_t<consumer_t> {
        using base = io::base::consumer_t<consumer_t>;
        using typename base::clock, typename base::duration, typename base::time_point, typename base::contract_t;
        using base::I, base::T, base::contracts, base::rts;

        consumer_t(std::string dir, std::string asset_path, bool is_irts_enabled, bool is_rts_enabled) : base(is_irts_enabled, is_rts_enabled),
            dir(dir), asset_path(asset_path) {
            namespace fs = std::filesystem;
            if (fs::exists(dir) && fs::is_directory(dir)) {
                INFO << "directory exists..." << std::endl;
            } else fs::create_directories(dir);
        }

        void on_day_begin(time_point day) {
            using namespace std::chrono;
            namespace fs = std::filesystem;
            auto start_time = floor<seconds>(day);
        
            try {
                std::string today = date::format("%F", floor<days>(day));
                std::string filename = dir + "/" + today + ".csv";
                fs::path filepath(filename);
        
                if (!fs::exists(filepath)) {
                    fs::create_directories(filepath.parent_path());
                    ofs.open(filepath);
                    if (!ofs) THROW_RUNTIME_ERROR("Failed to create CSV output: " + filename);
                    status = iex::compat::format("▫ {}", start_time);
                } else {
                    ofs.open(filepath, std::ios::trunc);
                    if (!ofs) THROW_RUNTIME_ERROR("Failed to overwrite CSV output: " + filename);
                    status = iex::compat::format("▪ {}", start_time);
                }
                ofs << "time,contract_id,price,size,is_bid,is_trade,is_ask\n";
            } catch (const h5::error::any& err) {
                ERROR << err.what() << std::endl;
                status = iex::compat::format("⯑ {}", start_time);
            }
        
            std::cout << status << std::flush;
        }
        
        void append(time_point now, contract_t contract, float price, uint32_t size, bool is_bid, bool is_trade, bool is_ask) {
            auto ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            ofs << ns_since_epoch << ',' << contract << ',' << price << ',' << size << ','
                << is_bid << ',' << is_trade << ',' << is_ask << '\n';
            ++counter;            
        }
        
        void on_trade_report(time_point time, contract_t id, float price, uint32_t size, uint8_t ) {
            if(is_irts_enabled) append(time, id, price, size, false, true, false); 
        }

        void on_ask(time_point time, contract_t id, float price, uint32_t size, uint8_t flag) {
            if(is_irts_enabled) append(time, id, price, size, false, false, true); 
        }
        void on_bid(time_point time, contract_t id, float price, uint32_t size, uint8_t flag) {
            if(is_irts_enabled) append(time, id, price, size, true, false, false); 
        }
        void on_heart_beat(time_point time) {
            auto tp = date::format("%H:%M:%S", date::floor<std::chrono::seconds>(time));
            std::cout << clear << status << " " << tp << std::flush;
        }        
        void on_day_end(time_point day) try {
            if (ofs.is_open()) ofs.close();
            std::cout << " ✓" << std::endl;
        } catch(const h5::error::any& err){
            ERROR << err.what() << std::endl;
            std::cout << " ✗" << std::endl;
        }

        void on_session_end(){
            if (ofs.is_open()) ofs.close();
        }

        uint64_t slot, max_slot, counter = 0;
    private:
        std::ofstream ofs;
        std::string asset_path, today, dir, filename;
        std::vector<std::string> start, stop;
    };
}
