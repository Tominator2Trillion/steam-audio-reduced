# Copyright 2017-2023 Valve Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.17)

project(Phonon VERSION 4.8.1)

set(CMAKE_CXX_STANDARD 14)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build")


#
# SYSTEM INTROSPECTION
#

# This minimal build only targets Windows.
if (NOT WIN32)
    message(FATAL_ERROR "This minimal build only supports Windows. For other platforms, use the full Steam Audio SDK.")
endif()

set(IPL_OS_WINDOWS TRUE)

# CPU architecture detection
if (CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
    set(IPL_CPU_ARMV8 TRUE)
elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IPL_CPU_X64 TRUE)
else()
    set(IPL_CPU_X86 TRUE)
endif()


#
# OPTIONS
#

option(BUILD_SHARED_LIBS "Build shared libraries instead of static." ON)
option(STEAMAUDIO_STATIC_RUNTIME "Statically link against the C/C++ runtime library." ON)


#
# COMPILER FLAGS
#

add_compile_definitions(
    WIN32
    WINDOWS
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _DISABLE_EXTENDED_ALIGNED_STORAGE
    $<IF:$<CONFIG:Debug>,_DEBUG,NDEBUG>
)
add_compile_options(
    /EHsc /MP /W3 /Zi
    $<$<AND:$<CONFIG:Debug>,$<BOOL:${STEAMAUDIO_STATIC_RUNTIME}>>:/MTd>
    $<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:${STEAMAUDIO_STATIC_RUNTIME}>>>:/MDd>
    $<$<AND:$<NOT:$<CONFIG:Debug>>,$<BOOL:${STEAMAUDIO_STATIC_RUNTIME}>>:/MT>
    $<$<AND:$<NOT:$<CONFIG:Debug>>,$<NOT:$<BOOL:${STEAMAUDIO_STATIC_RUNTIME}>>>:/MD>
    $<$<CONFIG:Debug>:/RTC1>
    $<$<NOT:$<CONFIG:Debug>>:/Ox>
    $<$<CONFIG:Release>:/GL>
)
add_link_options(
    /DEBUG
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
    $<$<CONFIG:Release>:/LTCG>
    $<$<CONFIG:Release>:/MANIFEST:NO>
    $<IF:$<CONFIG:Release>,/INCREMENTAL:NO,/INCREMENTAL>
    $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>
)


#
# DEPENDENCIES
#

find_package(PFFFT REQUIRED)
set(FFT_LIBRARY PFFFT)

find_package(MySOFA REQUIRED)
find_package(FlatBuffers REQUIRED)

include(CMakeListsInternal.txt OPTIONAL RESULT_VARIABLE STEAMAUDIO_INTERNAL_MODE)
if (NOT STEAMAUDIO_INTERNAL_MODE)
    set(STEAMAUDIO_PROFILING_TOOLS "None;Generic")
    set(STEAMAUDIO_PROFILING_TOOL "None" CACHE STRING "${STEAMAUDIO_PROFILING_TOOLS}")

    if(NOT STEAMAUDIO_PROFILING_TOOL IN_LIST STEAMAUDIO_PROFILING_TOOLS)
        message(FATAL_ERROR "Invalid option selected: ${STEAMAUDIO_PROFILING_TOOL}. Must be one of: ${STEAMAUDIO_PROFILING_TOOLS}.")
    endif()
endif()


#
# TARGETS
#

add_subdirectory(src/core)


#
# PACKAGING
#

install(FILES ${CMAKE_HOME_DIRECTORY}/THIRDPARTY.md DESTINATION root)

set(CPACK_PACKAGE_VENDOR "Valve Corporation")
set(CPACK_PACKAGE_FILE_NAME steamaudio)
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_HOME_DIRECTORY}/dist)
set(CPACK_INSTALL_CMAKE_PROJECTS "")

include(CPack)
