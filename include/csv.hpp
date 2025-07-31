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
        using base::I, base::T, base::contracts, base::rts, base::CONTRACT_ID_MASK;

        consumer_t(std::string dir, std::string asset_path, std::string tradingdays_path, bool is_irts_enabled, bool is_rts_enabled) : base(is_irts_enabled, is_rts_enabled),
            dir(dir), asset_path(asset_path), tradingdays_path(tradingdays_path) {
            namespace fs = std::filesystem;
            if (fs::exists(dir) && fs::is_directory(dir)) {
                INFO << "directory exists..." << std::endl;
            } else fs::create_directories(dir);

            std::vector<std::string> instruments;
            fs::path path = fs::path(dir + "/" + asset_path);
        
            if (fs::exists(path)) {
                std::ifstream file(path);
                if (!file) THROW_RUNTIME_ERROR("Failed to open asset file for reading: " + path.string());
        
                std::string line;
                while (std::getline(file, line))
                    if (!line.empty()) instruments.push_back(std::move(line));
                if (!instruments.empty()) batch_insert(instruments);
            }            
        }

        void on_day_begin(time_point day) {
            using namespace std::chrono;
            namespace fs = std::filesystem;
            auto start_time = floor<seconds>(day);
        
            try {
                std::string today = date::format("%F", floor<days>(day));
                std::string filename = dir + "/irts/" + today + ".csv";
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
            global::state::event_count++;          
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

        void on_session_end() {
            namespace fs = std::filesystem;
            if (ofs.is_open()) ofs.close();
            try {
                fs::path path(dir + "/" + asset_path);
                if (fs::exists(path)) fs::remove(path);
                std::ofstream fd(path);

                if (!fd) THROW_RUNTIME_ERROR("Failed to open asset file for writing: " + path.string());
                std::ranges::sort(flatmap, [](uint64_t a, uint64_t b) {
                    return  (a & CONTRACT_ID_MASK) < (b & CONTRACT_ID_MASK);
                });
                for (const auto& contract : flatmap)
                    fd << utils::base64::decode(contract).first << std::endl;
                fd.close();
            } catch (const std::exception& e) {
                ERROR << "Failed to write asset file: " << e.what() << '\n';
            }
            std::set<std::string> trading_days;
            for (const auto& entry : fs::directory_iterator(dir + "/irts")) {
                if (!entry.is_regular_file()) continue;

                auto name = entry.path().filename().string();
                if (name.size() == 14 && name.ends_with(".csv")) {
                    std::string date = name.substr(0, 10);  // "YYYY-MM-DD"
                    trading_days.insert(date);
                }
            }
            if (!trading_days.empty()) {
                fs::path path(dir + "/" + tradingdays_path);
                std::ofstream fd(path);
                if (!fd) THROW_RUNTIME_ERROR("Failed to write trading days index: " + path.string());
    
                for (const auto& day : trading_days)
                    fd << day << std::endl;
            }
        }

        uint64_t slot, max_slot, counter = 0;
    private:
        std::ofstream ofs;
        std::string asset_path, tradingdays_path, today, dir, filename;
        std::vector<std::string> start, stop;
    };
}
