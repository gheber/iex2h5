# This file is part of the IEX2H5 project and is licensed under the MIT License.
# 
# Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
# Contact: info@vargaconsulting.ca 

set(name rediscpp)

set(libname lib${name}_static)
project(${libname} LANGUAGES CXX C)

set(current_version_dir "v1.3.14")
set(all_source_files command.cpp command_options.cpp connection.cpp connection_pool.cpp crc16.cpp
    errors.cpp pipeline.cpp redis.cpp redis_cluster.cpp redis_uri.cpp reply.cpp sentinel.cpp
    shards.cpp shards_pool.cpp subscriber.cpp transaction.cpp)
list(TRANSFORM all_source_files PREPEND "${current_version_dir}/src/sw/redis++/")
set(hiredis_root "${CMAKE_CURRENT_SOURCE_DIR}/../hiredis")

add_library(${libname} STATIC ${all_source_files})
set_target_properties(${libname} PROPERTIES OUTPUT_NAME ${name})
target_include_directories(${libname} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}/src/sw/redis++/cxx17
    ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}/src/sw/redis++/no_tls)
target_link_libraries(${libname} PRIVATE libhiredis_static)
