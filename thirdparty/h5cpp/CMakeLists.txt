# This file is part of the IEX2H5 project and is licensed under the MIT License.
# 
# Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
# Contact: info@vargaconsulting.ca 

set(name h5cpp)

set(libname lib${name})
project(${libname} LANGUAGES CXX)

set(current_version_dir "v1.10.8")
add_library(${libname} INTERFACE)
target_include_directories(${libname} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}/include)

