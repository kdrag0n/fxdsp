cmake_minimum_required(VERSION 3.18.1)

project("fxdsp")
set(CMAKE_CXX_STANDARD 20)

set(COMMON_FLAGS
        -Wall
        -fexceptions -fno-rtti
        -DBOOST_MATH_STANDALONE=1
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug
    add_compile_options(${COMMON_FLAGS} -Og -g)
else()
    # Release
    add_compile_options(${COMMON_FLAGS} -O3)
endif()

# kissfft
set(kissfft_SOURCES
        external/kissfft/kiss_fftnd.c
        external/kissfft/kiss_fft.c
        external/kissfft/kiss_fftndr.c
        external/kissfft/kiss_fftr.c
        external/kissfft/kfc.c
)
set_source_files_properties(${kissfft_SOURCES} PROPERTIES COMPILE_FLAGS
        "-Dkiss_fft_scalar=float -fomit-frame-pointer"
)

# Boost math
include_directories(
        .
        external/boost/libs/math/include
)

set(fxdsp_SOURCES
        effects/convolver.cpp
        effects/gain.cpp
        effects/geq_common.cpp
        effects/graphic_eq_fir.cpp
        effects/graphic_eq_iir.cpp
        effects/noise.cpp
        effects/parametric_eq.cpp
        effects/silence.cpp
        filters/biquad.cpp
        filters/fir_design.cpp
        sinks/collecting_float.cpp
        sinks/collecting_s16.cpp
        util/amplitude.cpp
        util/debug.cpp
        util/fft.cpp
        util/graph.cpp
        util/window.cpp
        dsp.cpp
        log.cpp
        pcm.cpp
        sink.cpp
        wave.cpp
        ${kissfft_SOURCES}
)

if(ANDROID)
    set(fxdsp_SOURCES ${fxdsp_SOURCES}
            jni.cpp
            sinks/oboe.cpp
    )
endif()

add_library(fxdsp SHARED ${fxdsp_SOURCES})

if(ANDROID)
    # Log
    find_library(log-lib log)
    find_package(boringssl REQUIRED CONFIG)

    # Audio I/O
    # Set the path to the Oboe directory.
    set(OBOE_DIR external/oboe)

    # Add the Oboe library as a subdirectory in your project.
    # add_subdirectory tells CMake to look in this directory to
    # compile oboe source files using oboe's CMake file.
    # ./oboe specifies where the compiled binaries will be stored
    add_subdirectory(${OBOE_DIR} ./oboe)

    # Specify the path to the Oboe header files.=
    # This allows targets compiled with this CMake (application code)
    # to see public Oboe headers, in order to access its API.
    include_directories(${OBOE_DIR}/include)

    target_link_libraries(fxdsp ${log-lib} oboe OpenSLES boringssl::crypto_static)
else()
    # Only build executables for desktop testing. -fvisibility=hidden breaks their linking
    add_executable(fxdsp-filter-test
            cli/filter_test.cpp)
    target_link_libraries(fxdsp-filter-test fxdsp)

    add_executable(fxdsp-gen-fr-test-combined
            cli/gen_fr_test_combined.cpp)
    target_link_libraries(fxdsp-gen-fr-test-combined fxdsp)

    add_executable(fxdsp-gen-fr-test-sweep
            cli/gen_fr_test_sweep.cpp)
    target_link_libraries(fxdsp-gen-fr-test-sweep fxdsp)

    add_executable(fxdsp-filter-fr-sweep
            cli/filter_fr_sweep.cpp)
    target_link_libraries(fxdsp-filter-fr-sweep fxdsp)
endif()
