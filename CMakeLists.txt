cmake_minimum_required(VERSION 3.22)
project(RangeMasterLab VERSION 1.0.2 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JUCE_PATH "" CACHE PATH "Local JUCE 8.0.12 source directory")
if(JUCE_PATH)
  add_subdirectory("${JUCE_PATH}" juce)
else()
  include(FetchContent)
  set(juce_archive https://codeload.github.com/juce-framework/JUCE/zip/refs/tags/8.0.12)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/JUCE-8.0.12.zip")
    set(juce_archive "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/JUCE-8.0.12.zip")
  endif()
  FetchContent_Declare(JUCE URL "${juce_archive}"
    URL_HASH SHA256=91ceb4b54f9cc763884f6285fcfee4372fbe72d33061a339255cee2957e34ad8)
  FetchContent_MakeAvailable(JUCE)
endif()
set(formats VST3 Standalone)
if(APPLE)
  list(APPEND formats AU)
endif()
juce_add_plugin(RangeMasterLab
  COMPANY_NAME "c.d.r.c plugins" BUNDLE_ID "com.cdrcplugins.rangemasterlab"
  PLUGIN_MANUFACTURER_CODE CdrC PLUGIN_CODE RmLb
  FORMATS ${formats} PRODUCT_NAME "c.d.r.c plugins - RangeMaster"
  DESCRIPTION "Circuit-modelled germanium and silicon treble booster"
  IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Fx Distortion
  AU_MAIN_TYPE kAudioUnitType_Effect)
target_sources(RangeMasterLab PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(RangeMasterLab PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(RangeMasterLab PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags)
juce_add_console_app(RangeMasterTests PRODUCT_NAME "RangeMasterTests")
target_sources(RangeMasterTests PRIVATE tests/Tests.cpp)
target_link_libraries(RangeMasterTests PRIVATE RangeMasterLab juce::juce_audio_utils juce::juce_dsp)
target_compile_definitions(RangeMasterTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_MODAL_LOOPS_PERMITTED=1)
juce_add_console_app(RangeMasterHostCheck PRODUCT_NAME "RangeMasterHostCheck")
target_sources(RangeMasterHostCheck PRIVATE tests/HostSmoke.cpp)
target_link_libraries(RangeMasterHostCheck PRIVATE juce::juce_audio_utils)
target_compile_definitions(RangeMasterHostCheck PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_PLUGINHOST_VST3=1 JUCE_VST3_CAN_REPLACE_VST2=0)
enable_testing()
add_test(NAME circuit_and_processor COMMAND RangeMasterTests)
juce_add_console_app(RangeMasterControlAudit PRODUCT_NAME "RangeMasterControlAudit")
target_sources(RangeMasterControlAudit PRIVATE tests/ControlAudit.cpp)
target_link_libraries(RangeMasterControlAudit PRIVATE juce::juce_audio_utils)
target_compile_definitions(RangeMasterControlAudit PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_PLUGINHOST_VST3=1 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_MODAL_LOOPS_PERMITTED=1)
add_executable(RangeMasterCircuitProbe tests/CircuitProbe.cpp)
