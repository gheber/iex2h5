# This file is part of the IEX2H5 project and is licensed under the MIT License.
# 
# Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
# Contact: info@vargaconsulting.ca 

set(name hiredis)

set(libname lib${name}_static)
project(${libname} LANGUAGES CXX C)

set(current_version_dir "v1.2.0")
set(all_source_files alloc.c async.c hiredis.c net.c read.c sds.c sockcompat.c)
list(TRANSFORM all_source_files PREPEND "${current_version_dir}/src/")

add_library(${libname} STATIC ${all_source_files})
set_target_properties(${libname} PROPERTIES OUTPUT_NAME ${name})

target_include_directories(${libname} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}/include/hiredis)
