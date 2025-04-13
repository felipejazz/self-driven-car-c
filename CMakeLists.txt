cmake_minimum_required(VERSION 3.10)

project(SelfDrivingCar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set executable name
set(EXECUTABLE_NAME "self_driving_car")

# Include directories for your own project headers
include_directories(include)

# Find SFML package - Searches system paths automatically
# Make sure you request all components you actually use
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

# Add source files
# Using GLOB is convenient but may require rerunning cmake if files are added/removed.
# Listing files explicitly is often more robust:
# set(SOURCES src/main.cpp src/other.cpp ...)
file(GLOB SOURCES "src/*.cpp")

# Add executable using the gathered sources
set(SOURCES 
    main.cpp
    src/Car.cpp
    src/Controls.cpp
    src/Network.cpp
    src/Road.cpp
    src/Sensor.cpp
    src/Utils.cpp
    src/Visualizer.cpp
    src/Obstacle.cpp
)
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Link SFML libraries using modern imported targets
# This automatically handles include paths and dependencies for SFML.
target_link_libraries(${EXECUTABLE_NAME} PUBLIC SFML::Graphics SFML::Window SFML::System)

# --- Optional: Copy assets to build directory ---
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ASSETS_DESTINATION "${CMAKE_BINARY_DIR}/assets") # Destination within the build folder

if(EXISTS ${ASSETS_DIR})
    file(GLOB ASSET_FILES "${ASSETS_DIR}/*.*")
    if(ASSET_FILES) # Check if GLOB found any files
        add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ASSETS_DESTINATION}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSET_FILES} ${ASSETS_DESTINATION}
            COMMENT "Copying assets..."
        )
    else()
        message(WARNING "Assets directory exists at ${ASSETS_DIR}, but no files matching '*.*' were found. Skipping asset copying.")
    endif()
else()
    message(WARNING "Assets directory not found at ${ASSETS_DIR}. Skipping asset copying.")
endif()

# --- RPATH Section (Likely unnecessary for system-installed SFML) ---
# If you remove this, the system linker will find SFML libs in /usr/lib.
# If you keep it, it tells the executable to also look in its own directory for .so files.
# set(CMAKE_INSTALL_RPATH "$ORIGIN") # Affects installation
# set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) # Affects installation
# target_link_libraries(${EXECUTABLE_NAME} PRIVATE "-Wl,-rpath,'$ORIGIN'") # Affects build tree executable

# Optional: Set output directory for the executable (default is CMAKE_BINARY_DIR)
# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})


