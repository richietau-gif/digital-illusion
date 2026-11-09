cmake_minimum_required(VERSION 3.22)
project(DI_SEQFX VERSION 5.4.0)
set(CMAKE_CXX_STANDARD 17)
include(FetchContent)
FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)
juce_add_plugin(DISeqfx COMPANY_NAME "Digital Illusion" BUNDLE_ID "com.digitalillusion.diseqfx" PLUGIN_MANUFACTURER_CODE DIll PLUGIN_CODE DiSq FORMATS VST3 PRODUCT_NAME "DI SEQFX" IS_SYNTH FALSE NEEDS_MIDI_INPUT TRUE IS_MIDI_EFFECT FALSE COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(DISeqfx)
target_sources(DISeqfx PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(DISeqfx PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(DISeqfx PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra)

include(CTest)
if(BUILD_TESTING)
 add_executable(di_trigger_logic_smoke Tests/TriggerLogicSmoke.cpp)
 target_compile_features(di_trigger_logic_smoke PRIVATE cxx_std_17)
 add_test(NAME di_trigger_logic_smoke COMMAND di_trigger_logic_smoke)
 add_executable(di_shape_sequencer_smoke Tests/ShapeSequencerSmoke.cpp)
 target_compile_features(di_shape_sequencer_smoke PRIVATE cxx_std_17)
 add_test(NAME di_shape_sequencer_smoke COMMAND di_shape_sequencer_smoke)
 add_executable(di_fx_waveshaper_smoke Tests/FxWaveshaperSmoke.cpp)
 target_compile_features(di_fx_waveshaper_smoke PRIVATE cxx_std_17)
 add_test(NAME di_fx_waveshaper_smoke COMMAND di_fx_waveshaper_smoke)
 add_executable(di_peak_detector_smoke Tests/PeakDetectorSmoke.cpp)
 target_compile_features(di_peak_detector_smoke PRIVATE cxx_std_17)
 add_test(NAME di_peak_detector_smoke COMMAND di_peak_detector_smoke)
 add_executable(di_oversampled_fx_smoke Tests/OversampledFx2xSmoke.cpp)
 target_compile_features(di_oversampled_fx_smoke PRIVATE cxx_std_17)
 add_test(NAME di_oversampled_fx_smoke COMMAND di_oversampled_fx_smoke)
 add_executable(di_filter_cutoff_smoke Tests/FilterCutoffSmoke.cpp)
 target_compile_features(di_filter_cutoff_smoke PRIVATE cxx_std_17)
 add_test(NAME di_filter_cutoff_smoke COMMAND di_filter_cutoff_smoke)
 add_executable(di_fx_combination_smoke Tests/FxCombinationSmoke.cpp)
 target_compile_features(di_fx_combination_smoke PRIVATE cxx_std_17)
 add_test(NAME di_fx_combination_smoke COMMAND di_fx_combination_smoke)
 add_executable(di_duck_track_grid_smoke Tests/DuckTrackGridSmoke.cpp)
 target_compile_features(di_duck_track_grid_smoke PRIVATE cxx_std_17)
 add_test(NAME di_duck_track_grid_smoke COMMAND di_duck_track_grid_smoke)
 add_executable(di_creative_fx_smoke Tests/CreativeFxSmoke.cpp)
 target_compile_features(di_creative_fx_smoke PRIVATE cxx_std_17)
 target_link_libraries(di_creative_fx_smoke PRIVATE juce::juce_audio_basics)
 add_test(NAME di_creative_fx_smoke COMMAND di_creative_fx_smoke)
 add_executable(di_duck_voice_smoke Tests/DuckVoiceSmoke.cpp)
 target_compile_features(di_duck_voice_smoke PRIVATE cxx_std_17)
 add_test(NAME di_duck_voice_smoke COMMAND di_duck_voice_smoke)
 add_executable(di_creative_distort_2x_smoke Tests/CreativeDistort2xSmoke.cpp)
 target_compile_features(di_creative_distort_2x_smoke PRIVATE cxx_std_17)
 add_test(NAME di_creative_distort_2x_smoke COMMAND di_creative_distort_2x_smoke)
 add_executable(di_curve_selection_smoke Tests/CurveSelectionSmoke.cpp)
 target_compile_features(di_curve_selection_smoke PRIVATE cxx_std_17)
 add_test(NAME di_curve_selection_smoke COMMAND di_curve_selection_smoke)
 add_executable(di_dynamic_curve_smoke Tests/DynamicCurveSmoke.cpp)
 target_compile_features(di_dynamic_curve_smoke PRIVATE cxx_std_17)
 add_test(NAME di_dynamic_curve_smoke COMMAND di_dynamic_curve_smoke)
 add_executable(di_creative_delay_smoke Tests/CreativeDelaySmoke.cpp)
 target_compile_features(di_creative_delay_smoke PRIVATE cxx_std_17)
 add_test(NAME di_creative_delay_smoke COMMAND di_creative_delay_smoke)
 add_executable(di_analog_ladder_smoke Tests/AnalogLadderSmoke.cpp)
 target_compile_features(di_analog_ladder_smoke PRIVATE cxx_std_17)
 add_test(NAME di_analog_ladder_smoke COMMAND di_analog_ladder_smoke)
 add_executable(di_seqfx_layout_smoke Tests/SeqfxLayoutSmoke.cpp)
 target_compile_features(di_seqfx_layout_smoke PRIVATE cxx_std_17)
 add_test(NAME di_seqfx_layout_smoke COMMAND di_seqfx_layout_smoke)
 # Treat warnings as errors for all standalone DSP regression executables,
 # matching the standalone Tests/CMakeLists.txt configuration.
 foreach(smoke_target IN ITEMS
   di_trigger_logic_smoke
   di_shape_sequencer_smoke
   di_fx_waveshaper_smoke
   di_peak_detector_smoke
   di_oversampled_fx_smoke
   di_filter_cutoff_smoke
   di_fx_combination_smoke
   di_duck_track_grid_smoke
   di_creative_fx_smoke
   di_duck_voice_smoke
   di_creative_distort_2x_smoke
   di_curve_selection_smoke
   di_dynamic_curve_smoke
   di_creative_delay_smoke
   di_analog_ladder_smoke
   di_seqfx_layout_smoke)
  if(MSVC)
   target_compile_options(${smoke_target} PRIVATE /W4 /WX)
  else()
   target_compile_options(${smoke_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
 endforeach()
endif()
