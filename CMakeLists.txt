cmake_minimum_required(VERSION 3.21)

project(GravityEQ VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JUCE_DIR "${CMAKE_SOURCE_DIR}/third_party/JUCE")
if (NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE not found at third_party/JUCE.\n"
        "Add JUCE as a submodule:\n"
        "  git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE\n"
        "  git submodule update --init --recursive\n")
endif()

add_subdirectory(third_party/JUCE)

juce_add_plugin(GravityEQ
    COMPANY_NAME "EQ Infinity"
    BUNDLE_ID "com.eqinfinity.GravityEQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE

    PLUGIN_MANUFACTURER_CODE EqIn
    PLUGIN_CODE GrEQ

    FORMATS AU VST3 Standalone
    PRODUCT_NAME "GravityEQ"
)

target_sources(GravityEQ PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/util/Params.cpp
    src/util/Params.h
)

target_include_directories(GravityEQ PRIVATE
    src
)

target_compile_definitions(GravityEQ PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GravityEQ PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp

    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_generate_juce_header(GravityEQ)
