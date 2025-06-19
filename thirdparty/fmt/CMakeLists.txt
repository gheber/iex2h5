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
