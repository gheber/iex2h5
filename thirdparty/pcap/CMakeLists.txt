# This file is part of the IEX2H5 project and is licensed under the MIT License.
# 
# Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
# Contact: info@vargaconsulting.ca 

set(name pcap)
set(libname lib${name}_static)
project(${libname} LANGUAGES C)

set(current_version_dir "v1.10.4")

# Source files — glob just what we need, filtered to core and Linux-specific files
set(all_source_files
   bpf_dump.c bpf_filter.c bpf_image.c charconv.c etherent.c fad-getad.c fad-gifc.c fmtutils.c
   gencode_stub.c nametoaddr.c optimize.c pcap.c pcap-common.c pcap-linux.c pcap-netfilter-linux.c
   pcap-usb-linux.c pcap-usb-linux-common.c pcap-util.c savefile.c sf-pcap.c sf-pcapng.c sockutils.c strlcpy.c
)
list(TRANSFORM all_source_files PREPEND "${current_version_dir}/")

# Create the static library target
add_library(${libname} STATIC ${all_source_files})
set_target_properties(${libname} PROPERTIES OUTPUT_NAME pcap)

# Include public and internal headers
target_include_directories(${libname} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}
    ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir}/pcap
)

# Optional: link additional libs if needed
if(UNIX AND NOT APPLE)
    target_link_libraries(${libname} PRIVATE dl)
endif()

# Compile flags for pcap (disable unused warnings, define HAVE_CONFIG_H if needed)
target_compile_definitions(${libname} PRIVATE
    HAVE_CONFIG_H )
