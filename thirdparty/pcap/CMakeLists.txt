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
#   Copyright © <2011-2025> Varga Consulting, Toronto, On     info@vargaconsulting.ca
#   _________________________________________________________________________________

set(name pcap)
set(libname lib${name}_static)
project(${libname} LANGUAGES C)

set(current_version_dir "v1.10.4")

# Source files — glob just what we need, filtered to core and Linux-specific files
set(all_source_files
    ${current_version_dir}/bpf_dump.c
    ${current_version_dir}/bpf_filter.c
    ${current_version_dir}/bpf_image.c
    ${current_version_dir}/charconv.c
    ${current_version_dir}/etherent.c
    ${current_version_dir}/fad-getad.c
    ${current_version_dir}/fad-gifc.c
    ${current_version_dir}/fmtutils.c
    ${current_version_dir}/gencode_stub.c
    ${current_version_dir}/nametoaddr.c
    ${current_version_dir}/optimize.c
    ${current_version_dir}/pcap.c
    ${current_version_dir}/pcap-common.c
    ${current_version_dir}/pcap-linux.c
    ${current_version_dir}/pcap-netfilter-linux.c
    ${current_version_dir}/pcap-usb-linux.c
    ${current_version_dir}/pcap-usb-linux-common.c
    ${current_version_dir}/pcap-util.c
    ${current_version_dir}/savefile.c
    ${current_version_dir}/sf-pcap.c
    ${current_version_dir}/sf-pcapng.c
    ${current_version_dir}/sockutils.c
)
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
