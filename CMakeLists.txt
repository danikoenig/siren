cmake_minimum_required(VERSION 3.22)
project(Siren VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(libs/JUCE)

set(FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND FORMATS AU)
endif()

juce_add_plugin(Siren
  COMPANY_NAME "Dani Koenig"
  PLUGIN_MANUFACTURER_CODE Dkoe
  PLUGIN_CODE Sirn
  FORMATS ${FORMATS}
  PRODUCT_NAME "Siren"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  VST3_CATEGORIES Instrument Synth
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Siren PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(Siren PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Siren PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
