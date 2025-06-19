# This file is part of the IEX2H5 project and is licensed under the MIT License.
# 
# Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
# Contact: info@vargaconsulting.ca 

set(name fmt)

set(libname lib${name}_static)
project(${libname} LANGUAGES CXX)

set(current_version_dir "v11.2.0")
file(GLOB_RECURSE all_source_files "${current_version_dir}/src/format.cc")
file(GLOB_RECURSE all_header_files "${current_version_dir}/include/*.h")

add_library(${libname} STATIC ${all_source_files} ${all_header_files})
set_target_properties(${libname} PROPERTIES OUTPUT_NAME ${name})
target_include_directories(${libname} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}/include)
