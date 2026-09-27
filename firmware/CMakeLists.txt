# KynoWave firmware: ESP-IDF, communication through BTPDevice.
cmake_minimum_required(VERSION 3.16)

# BTPDevice and its two dependencies come from sibling checkouts of this
# repository (c:/git/BTPDevice, c:/git/BTP, c:/git/TinyShell) while the
# library has no release yet.
foreach(_dep BTPDevice BTP TinyShell)
    get_filename_component(_dir "${CMAKE_CURRENT_LIST_DIR}/../../${_dep}" ABSOLUTE)
    if(NOT EXISTS "${_dir}/CMakeLists.txt")
        message(FATAL_ERROR "KynoWave expects ${_dep} checked out next to this repository, at ${_dir}")
    endif()
    list(APPEND EXTRA_COMPONENT_DIRS "${_dir}")
endforeach()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(kynowave)
