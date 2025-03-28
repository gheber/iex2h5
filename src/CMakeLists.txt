#   ALL RIGHTS RESERVED.
#   _________________________________________________________________________________
#   NOTICE: All information contained  herein is, and remains the property  of  Varga
#   Consulting and  its suppliers, if  any. The intellectual and  technical  concepts
#   contained herein are proprietary to Varga Consulting and its suppliers and may be
#   covered  by  Canadian and  Foreign Patents, patents in process, and are protected
#   by  trade secret or copyright law. Dissemination of this information or reproduc-
#   tion  of  this  material is strictly forbidden unless prior written permission is
#   obtained from Varga Consulting.
#
#   Copyright © <2011-2024> Varga Consulting, Toronto, On     info@vargaconsulting.ca
#   _________________________________________________________________________________

cmake_minimum_required(VERSION 3.14)
project(iex2h5 LANGUAGES C CXX)
set(PROGRAM_NAME ${PROJECT_NAME})

file(GLOB_RECURSE all_source_files CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE all_header_files CONFIGURE_DEPENDS "*.hpp")

add_executable(${PROGRAM_NAME} ${all_source_files})

# Install target
install(TARGETS ${PROGRAM_NAME} DESTINATION bin)

# Includes, libraries, compiler options
target_include_directories(${PROGRAM_NAME} PRIVATE ${INCLUDE_DIRS})
target_compile_options(${PROGRAM_NAME} PRIVATE -w)
target_compile_definitions(${PROGRAM_NAME} PRIVATE IEX_PROGRAM_NAME="${PROJECT_NAME}")
target_link_libraries(${PROGRAM_NAME} PRIVATE ${LIBS})

# Custom commands to mimic Makefile logic
if(CMAKE_BUILD_TYPE MATCHES "Debug")

    set(IEX_H5 "$ENV{HOME}/iex.h5")
    set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")
    set(DATA_PCAP "${DATA_DIR}/TOPS-2017-01-06.pcap.gz")

    # init-iexh5 target: creates $HOME/iex.h5 using one known pcap file
    add_custom_command(
        OUTPUT ${IEX_H5}
        COMMAND ${CMAKE_COMMAND} -E echo "Generating ${IEX_H5} from ${INIT_PCAP}"
        COMMAND unpigz -c ${DATA_PCAP} | $<TARGET_FILE:${PROGRAM_NAME}> --command init -o ${IEX_H5}
        DEPENDS ${PROGRAM_NAME}
        COMMENT "Generating ${IEX_H5} from TOPS data"
        VERBATIM
    )
    add_custom_target(init-iexh5 DEPENDS ${IEX_H5})

    # rts-iexh5 target: runs rts step on one additional file
    add_custom_command(
        OUTPUT ${IEX_H5}.rts
        COMMAND ${CMAKE_COMMAND} -E echo "Running RTS on ${RTS_PCAP}"
        COMMAND unpigz -c ${DATA_PCAP} | $<TARGET_FILE:${PROGRAM_NAME}> -g 9 --command rts -o ${IEX_H5}
        COMMAND ${CMAKE_COMMAND} -E touch ${IEX_H5}.rts
        DEPENDS ${PROGRAM_NAME} init-iexh5
        COMMENT "Converting IR/TS stream to RTS in ${IEX_H5}"
        VERBATIM
    )
    add_custom_target(rts-iexh5 DEPENDS ${IEX_H5}.rts)

endif()
