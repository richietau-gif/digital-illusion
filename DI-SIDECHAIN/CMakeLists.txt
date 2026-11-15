cmake_minimum_required(VERSION 3.22)
project(PRECISION_SIDECHAIN VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
include(FetchContent)
FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)
juce_add_plugin(PrecisionSidechain COMPANY_NAME "Digital Illusion" BUNDLE_ID "com.digitalillusion.precisionsidechain" PLUGIN_MANUFACTURER_CODE DIll PLUGIN_CODE PrSc FORMATS VST3 PRODUCT_NAME "PRECISION SIDECHAIN" IS_SYNTH FALSE NEEDS_MIDI_INPUT TRUE IS_MIDI_EFFECT FALSE COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(PrecisionSidechain)
target_sources(PrecisionSidechain PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(PrecisionSidechain PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(PrecisionSidechain PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra)

include(CTest)
if(BUILD_TESTING)
 add_executable(precision_sidechain_smoke Tests/PrecisionSidechainSmoke.cpp)
 target_compile_features(precision_sidechain_smoke PRIVATE cxx_std_17)
 add_test(NAME precision_sidechain_smoke COMMAND precision_sidechain_smoke)
 add_executable(precision_timing_regression Tests/TimingRegression.cpp)
 target_compile_features(precision_timing_regression PRIVATE cxx_std_17)
 add_test(NAME precision_timing_regression COMMAND precision_timing_regression)

 # Genuine DSP tests link the actual processor and editor sources rather than
 # re-implementing a simplified imitation of the algorithm.
 juce_add_console_app(precision_dsp_integration PRODUCT_NAME "PRECISION DSP Integration Tests")
 juce_generate_juce_header(precision_dsp_integration)
 target_sources(precision_dsp_integration PRIVATE
  Tests/PrecisionDSPIntegration.cpp
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)
 target_compile_features(precision_dsp_integration PRIVATE cxx_std_17)
 target_compile_definitions(precision_dsp_integration PRIVATE
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
 target_link_libraries(precision_dsp_integration PRIVATE
  juce::juce_audio_utils juce::juce_dsp
  juce::juce_audio_processors juce::juce_gui_extra)
 add_test(NAME precision_dsp_integration COMMAND precision_dsp_integration)
endif()
