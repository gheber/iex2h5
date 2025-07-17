/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca */

 #pragma once

#include <string>
#include <iostream>
#include <string_view>
#include <chrono>
#include <thread>
#include <exception>
#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>
#include <date/date.h>
#include <date/tz.h> 
#include "compat.hpp"
namespace  {
    inline std::string basename__(const std::string& path) {
        size_t pos = path.find_last_of('/');
        return (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }
    std::string thread_id__(){
        std::stringstream ss; 
        ss << std::this_thread::get_id();
        return ss.str();
    }
    std::string time_stamp__(){
        using namespace std::chrono;
        return  date::format("%H:%M:%S",
            date::floor<milliseconds>(system_clock::now()));
    }
}
namespace sigma::syslog {
    struct logger_t {
        logger_t(int priority) : priority(priority){}

        template<class T>
        logger_t& operator<<(T value){
            ss << value;
            return *this;
        }

        void operator<<(std::ostream&(*io_manip)(std::ostream&)){
            if (io_manip == static_cast<std::ostream&(*)(std::ostream&)>(std::endl)) 
                ::syslog(priority, "%s", ss.str().data());
            else ss << io_manip;
        }             
        private:
            int priority;
            std::stringstream ss;
    };
}

#ifdef DEBUG 
    #define SIGMA_LOGGER_PREAMBLE \
        iex::compat::format("{:40}", iex::compat::format("[{:12} {:5}:{:<5} {:8}/{:<8} {:>15} #{:05}] ", \
        time_stamp__(), getgid(), getuid(), getpid(), gettid(), basename__(__FILE__),  __LINE__ ))
#else
    #define SIGMA_LOGGER_PREAMBLE \
        iex::compat::format("{:40}", iex::compat::format("[{} {:>15} #{:05}] ", time_stamp__(),  basename__(__FILE__),  __LINE__ ))
#endif

#ifdef DEBUG 
    #define TRACE std::cerr <<  "[trace]  " << SIGMA_LOGGER_PREAMBLE
    #define INFO std::cerr <<   "[info]   " << SIGMA_LOGGER_PREAMBLE 
    #define WARNING std::cerr <<"[warning]" << SIGMA_LOGGER_PREAMBLE 
    #define ERROR std::cerr <<  "[error]  " << SIGMA_LOGGER_PREAMBLE 
    #define FATAL std::cerr <<  "[fatal]  " << SIGMA_LOGGER_PREAMBLE 
#else 
    #define TRACE sigma::syslog::logger_t(LOG_DAEMON | LOG_DEBUG ) <<  "[trace]  " << SIGMA_LOGGER_PREAMBLE
    #define INFO sigma::syslog::logger_t(LOG_DAEMON | LOG_INFO )  <<   "[info]   " << SIGMA_LOGGER_PREAMBLE
    #define WARNING sigma::syslog::logger_t(LOG_DAEMON | LOG_WARNING ) << "[warning]"  << SIGMA_LOGGER_PREAMBLE
    #define ERROR sigma::syslog::logger_t(LOG_DAEMON | LOG_ERR )  <<  "[error]  " << SIGMA_LOGGER_PREAMBLE
    #define FATAL sigma::syslog::logger_t(LOG_DAEMON | LOG_CRIT ) <<  "[fatal]  " << SIGMA_LOGGER_PREAMBLE
#endif
// redefining H5CPP error logger to match local environment: 
#ifdef H5CPP_ERROR_MSG
    #undef H5CPP_ERROR_MSG
#endif
#define H5CPP_ERROR_MSG( msg ) iex::compat::format("{} #{:05} {}", basename__(__FILE__),  __LINE__ , msg)
#ifndef RUNTIME_ERROR 
    #define RUNTIME_ERROR(msg) std::runtime_error( \
        iex::compat::format("{} {}", static_cast<std::string>(SIGMA_LOGGER_PREAMBLE), static_cast<std::string>(msg)))
#endif
#ifndef THROW_RUNTIME_ERROR 
    #define THROW_RUNTIME_ERROR(msg) throw RUNTIME_ERROR(msg)
#endif
