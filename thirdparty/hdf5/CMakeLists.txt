#
# Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
# Contact: info@vargaconsulting.ca

cmake_minimum_required(VERSION 3.10)

set(name hdf5)
set(libname lib${name}_static)
project(${libname} LANGUAGES C)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING  OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_HL_LIB OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HDF5_ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(TEST_SHELL_SCRIPTS OFF CACHE BOOL "" FORCE)

set(_saved_prefix "${CMAKE_INSTALL_PREFIX}")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/fake-install" CACHE PATH "Fake install prefix for HDF5" FORCE)
    set(current_version_dir "v1.14.3")
    add_subdirectory(${current_version_dir})
    add_library(${libname} ALIAS hdf5-static)
set(CMAKE_INSTALL_PREFIX "${_saved_prefix}" CACHE PATH "Restore install prefix" FORCE)
