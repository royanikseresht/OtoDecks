cmake_minimum_required(VERSION 3.15)

project(FinalJuceVSCodeApp VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Path to your JUCE folder
set(JUCE_DIR "/Users/roya/Documents/JUCE")

# Add JUCE
add_subdirectory(${JUCE_DIR} JUCE)

# Define the JUCE app
juce_add_gui_app(FinalJuceVSCodeApp
    COMPANY_NAME "Roya"
    PRODUCT_NAME "FinalJuceVSCodeApp"
    MACOSX_BUNDLE
)

# Generate the AppConfig.h and JuceHeader.h files
juce_generate_juce_header(FinalJuceVSCodeApp)

# Add your source files
target_sources(FinalJuceVSCodeApp PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/CSVOperator.cpp
    Source/CSVOperator.h
    Source/DeckGUI.cpp
    Source/DeckGUI.h
    Source/DJAudioPlayer.cpp
    Source/DJAudioPlayer.h
    Source/LookAndFeel.cpp
    Source/LookAndFeel.h
    Source/PlaylistComponent.cpp
    Source/PlaylistComponent.h
    Source/TrackListComponent.cpp
    Source/TrackListComponent.h
    Source/WaveformDisplay.cpp
    Source/WaveformDisplay.h

)

# Disable unused JUCE modules
target_compile_definitions(FinalJuceVSCodeApp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

# Link JUCE modules
target_link_libraries(FinalJuceVSCodeApp PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
)

find_path(AUBIO_INCLUDE_DIR aubio/aubio.h
    PATHS /opt/homebrew/include /usr/local/include
)

find_library(AUBIO_LIB aubio
    PATHS /opt/homebrew/lib /usr/local/lib
)

if (AUBIO_INCLUDE_DIR AND AUBIO_LIB)
    message(STATUS "Found aubio in ${AUBIO_INCLUDE_DIR}")
    target_include_directories(FinalJuceVSCodeApp PRIVATE ${AUBIO_INCLUDE_DIR})
    target_link_libraries(FinalJuceVSCodeApp PRIVATE ${AUBIO_LIB})
else()
    message(FATAL_ERROR "aubio not found")
endif()

