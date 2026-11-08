cmake_minimum_required(VERSION 3.22)
project(DI_SIDECHAIN_Standalone_Tests LANGUAGES CXX)
enable_testing()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
foreach(test IN ITEMS
    ShapeSequencerSmoke
    FxWaveshaperSmoke
    PeakDetectorSmoke
    OversampledFx2xSmoke
    TriggerLogicSmoke)
  add_executable(${test} ${test}.cpp)
  if(MSVC)
    target_compile_options(${test} PRIVATE /W4 /WX)
  else()
    target_compile_options(${test} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME ${test} COMMAND ${test})
endforeach()
