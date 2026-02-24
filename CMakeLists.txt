cmake_minimum_required(VERSION 3.21)

project(EQInfinity VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Show JUCE module sources in IDE projects" ON)

if (WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif()

option(EQINF_COPY_PLUGIN_AFTER_BUILD "Copy plugin artifacts to system plugin directories after build" ON)
if (DEFINED ZL_JUCE_COPY_PLUGIN)
    set(EQINF_COPY_PLUGIN_AFTER_BUILD ${ZL_JUCE_COPY_PLUGIN})
endif()

if (APPLE)
    set(EQINF_DEFAULT_PLUGIN_FORMATS "AU;VST3;Standalone")
else()
    set(EQINF_DEFAULT_PLUGIN_FORMATS "VST3;Standalone")
endif()

if (DEFINED ZL_JUCE_FORMATS)
    set(EQINF_PLUGIN_FORMATS "${ZL_JUCE_FORMATS}")
else()
    set(EQINF_PLUGIN_FORMATS "${EQINF_DEFAULT_PLUGIN_FORMATS}" CACHE STRING
        "Semicolon-separated JUCE plugin formats to build")
endif()
message(STATUS "Formats are ${EQINF_PLUGIN_FORMATS}")

set(JUCE_DIR "${CMAKE_SOURCE_DIR}/third_party/JUCE")
if (NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE not found at third_party/JUCE.\n"
        "Add JUCE as a submodule:\n"
        "  git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE\n"
        "  git submodule update --init --recursive\n")
endif()

add_subdirectory(third_party/JUCE)

juce_add_plugin(EQInfinity
    COMPANY_NAME "EQ Infinity"
    BUNDLE_ID "com.eqinfinity.EQInfinity"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${EQINF_COPY_PLUGIN_AFTER_BUILD}

    PLUGIN_MANUFACTURER_CODE EqIn
    PLUGIN_CODE EqI8

    FORMATS ${EQINF_PLUGIN_FORMATS}
    PRODUCT_NAME "EQ Infinity"
)

target_sources(EQInfinity PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/util/Params.cpp
    src/util/Params.h
    src/dsp/EqBand.cpp
    src/dsp/EqBand.h
    src/dsp/EqEngine.cpp
    src/dsp/EqEngine.h
    src/dsp/ResponseCurve.cpp
    src/dsp/ResponseCurve.h
    src/ui/EqPlotComponent.cpp
    src/ui/EqPlotComponent.h
    src/ui/SpectrumAnalyzer.cpp
    src/ui/SpectrumAnalyzer.h
)

target_include_directories(EQInfinity PRIVATE
    src
)

target_compile_definitions(EQInfinity PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(EQInfinity PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp

    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_generate_juce_header(EQInfinity)

include(CTest)
set(CTEST_OUTPUT_ON_FAILURE ON)

if (BUILD_TESTING)
    add_executable(eq_infinity_tests
        tests/Milestone23Tests.cpp
        src/util/Params.cpp
        src/util/Params.h
        src/dsp/EqBand.cpp
        src/dsp/EqBand.h
        src/dsp/EqEngine.cpp
        src/dsp/EqEngine.h
    )

    target_include_directories(eq_infinity_tests PRIVATE
        src
    )

    target_compile_definitions(eq_infinity_tests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(eq_infinity_tests PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp

        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )

    add_test(NAME milestone23_tests COMMAND eq_infinity_tests)
endif()
