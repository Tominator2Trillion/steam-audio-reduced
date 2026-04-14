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

include(SteamAudioHelpers)

#
# VERSION STAMPING
#

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/phonon_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/phonon_version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/phonon.rc.in ${CMAKE_CURRENT_BINARY_DIR}/phonon.rc)


#
# HRTF DATA
#

add_library(hrtf OBJECT hrtf.cpp)
target_include_directories(hrtf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


#
# CORE OBJECT LIBRARY
#
# Minimal build: only Context, HRTF, BinauralEffect, PanningEffect, AudioBuffer.
#

add_library(core OBJECT
    # Infrastructure
    platform.h
    types.h
    error.h
    log.h
    log.cpp
    memory_allocator.h
    memory_allocator.cpp
    profiler.h
    profiler.cpp
    context.h
    context.cpp
    library.h
    library.cpp

    # Containers
    array.h
    containers.h
    stack.h
    triple_buffer.h
    serialized_object.h
    serialized_object.cpp

    # SIMD
    float4.h
    sse_float4.h
    neon_float4.h
    array_math.h
    array_math.cpp

    # Math
    math_functions.h
    math_functions.cpp
    vector.h
    polar_vector.h
    matrix.h
    quaternion.h
    coordinate_space.h
    box.h
    sphere.h
    material.h

    # DSP
    window_function.h
    window_function.cpp
    iir.h
    iir.cpp
    fft.h
    bands.h
    bands.cpp
    pffft_fft.cpp

    # Spherical Harmonics (required by hrtf_database)
    sh/spherical_harmonics.h
    sh/spherical_harmonics.cc
    sh.h
    sh.cpp

    # Audio
    speaker_layout.h
    speaker_layout.cpp
    audio_buffer.h
    audio_buffer.cpp

    # HRTF
    panning_effect.h
    panning_effect.cpp
    hrtf_map.h
    hrtf_map.cpp
    sofa_hrtf_map.h
    sofa_hrtf_map.cpp
    hrtf_database.h
    hrtf_database.cpp
    hrtf_map_factory.h
    hrtf_map_factory.cpp
    ambisonics_panning_effect.h
    ambisonics_panning_effect.cpp

    # Binaural
    overlap_add_convolution_effect.h
    overlap_add_convolution_effect.cpp
    binaural_effect.h
    binaural_effect.cpp

    # API
    phonon_version.h.in
    phonon.h
    docs.h
    util.h
    phonon_interfaces.h
    api_context.h
    api_context.cpp
    api_hrtf.h
    api_hrtf.cpp
    api_binaural_effect.h
    api_binaural_effect.cpp
    api_panning_effect.h
    api_panning_effect.cpp
    api_audio_buffer.cpp
    api_stubs.cpp
    api_validation_layer.cpp
)

if (STEAMAUDIO_PROFILING_TOOL STREQUAL "None")
    target_sources(core PRIVATE
        null_profiler.h
        null_profiler.cpp
    )
endif()

target_sources(core PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/phonon.rc
)

# Generated headers
target_include_directories(core PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Platform libraries
target_link_libraries(core PUBLIC delayimp)

# Dependencies
target_link_libraries(core PUBLIC FlatBuffers::FlatBuffers)
target_link_libraries(core PUBLIC PFFFT::PFFFT)
target_link_libraries(core PUBLIC MySOFA::MySOFA)

target_compile_definitions(core PRIVATE STEAMAUDIO_BUILDING_CORE)
target_precompile_headers(core PUBLIC pch.h)


#
# SHARED LIBRARY
#

if (BUILD_SHARED_LIBS)
    add_library(phonon SHARED pch.cpp)
else()
    add_library(phonon STATIC pch.cpp)
endif()

target_link_libraries(phonon PRIVATE core hrtf)
target_compile_definitions(phonon PRIVATE STEAMAUDIO_BUILDING_CORE)


#
# INSTALL
#

if (IPL_CPU_X86)
    set(IPL_LIB_SUBDIR windows-x86)
elseif (IPL_CPU_X64)
    set(IPL_LIB_SUBDIR windows-x64)
elseif (IPL_CPU_ARMV8)
    set(IPL_LIB_SUBDIR windows-arm64)
endif()

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/phonon.h ${CMAKE_CURRENT_BINARY_DIR}/phonon_version.h ${CMAKE_CURRENT_SOURCE_DIR}/phonon_interfaces.h
    DESTINATION include
)

install(
    TARGETS phonon
    RUNTIME DESTINATION lib/${IPL_LIB_SUBDIR}
    LIBRARY DESTINATION lib/${IPL_LIB_SUBDIR}
    ARCHIVE DESTINATION lib/${IPL_LIB_SUBDIR}
)
