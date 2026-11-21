cmake_minimum_required(VERSION 3.22)
project(Siren VERSION 1.1.0)
set(CMAKE_CXX_STANDARD 17)
# JUCE: local path (-DJUCE_PATH=...) or automatic download
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND FORMATS AU)
endif()

juce_add_plugin(Siren
  COMPANY_NAME "Dani Koenig"
  BUNDLE_ID "ch.danikoenig.siren"
  PLUGIN_MANUFACTURER_CODE Dkng
  PLUGIN_CODE Sir1
  FORMATS ${FORMATS}
  PRODUCT_NAME "Siren"
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  VST3_CATEGORIES Instrument Synth
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Siren PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/Look.cpp)
target_compile_definitions(Siren PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Siren PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
