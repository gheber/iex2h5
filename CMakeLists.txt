# This file is part of the IEX2H5 project and is licensed under the MIT License.
# 
# Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
# Contact: info@vargaconsulting.ca 


cmake_minimum_required(VERSION 3.22.0)

# Set the project name and specify the required C++ standard
project(iex2h5 LANGUAGES CXX C)

# Specify the required C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# automated versioning
execute_process( COMMAND git rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE )
execute_process( COMMAND git describe --tags --abbrev=0 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)

# Optionally, use the tag and hash to create a version string.
# If no tag is available, you can fallback to just using the hash.
# Configure the version file with the Git information
if(GIT_TAG)
  set(IEX2H5_VERSION_STRING ${GIT_TAG})
else()
  set(IEX2H5_VERSION_STRING ${GIT_COMMIT_HASH})
endif()
# Specify IEX2H5 system related constants, these value also can be (re)configured through env variables:
add_definitions(
  -DIEX2H5_SOFTWARE_VERSION="${IEX2H5_VERSION_STRING}"
  -DIEX2H5_SOFTWARE_COMMIT_HASH="${GIT_COMMIT_HASH}"
  -DARMA_DONT_USE_WRAPPER
  -DHOST_NAME_MAX=254
  -DVALUE_PRECISION=6
  -DSTAT_PRECISION=4
  -DIEX_MAX_SYMBOLS=20000)

## begin thirdparty library dependencies
  add_subdirectory(thirdparty)
## end library dependencies

include(FindHDF5)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED QUIET)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 11.4.0)
    add_compile_options(-w)
endif()

list(APPEND INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${HDF5_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} )
list(APPEND LIBS 
  ${HDF5_C_LIBRARIES} ${ZLIB_LIBRARIES} 
  libargparse libarmadillo libh5cpp libthreadpool libdate
  libfmt_static libhiredis_static librediscpp_static libzng_static
  )

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(IEX_PCAP_FILE "${CMAKE_SOURCE_DIR}/data/iex-packets.pcap")
  set(CMAKE_CXX_FLAGS "-fsanitize=address,undefined,bounds,float-divide-by-zero,float-cast-overflow,alignment -Wall -Wextra -Wpedantic -Werror")
  add_definitions(-DDEBUG) # -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC !!! will break AWS !!!
  enable_testing()
  ## usage: `add_test_case(file_name [LIBRARIES library-01 library-02 .. library-0n])`
  function(add_test_case file_name)
      set(options)
      set(one_value_args)
      set(multi_value_args LIBRARIES)
      cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

      set(test_name test-${PROJECT_NAME}-${file_name})
      add_executable(${test_name} test/${file_name}.cpp)
      target_include_directories(${test_name} PRIVATE ${INCLUDE_DIRS} src)
      target_compile_options(${test_name} PRIVATE -w)
      target_compile_definitions(${test_name} PRIVATE IEX_PCAP_FILE="${IEX_PCAP_FILE}")

      # Link libraries, excluding INTERFACE libraries
      target_link_libraries(${test_name} PRIVATE ${LIBS} libdoctest)
      foreach(lib IN LISTS ARG_LIBRARIES)
        target_link_libraries(${test_name} PRIVATE ${lib})
      endforeach()
      add_test(NAME ${test_name} COMMAND ${test_name})
  endfunction()

  add_test_case(example)
  add_test_case(utils)
  add_test_case(base76)
  add_test_case(filters)
  add_test_case(error)
  add_test_case(generics)
  add_test_case(producers)
endif()

add_subdirectory(src)
