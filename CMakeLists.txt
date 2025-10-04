cmake_minimum_required(VERSION 3.10)

project(SelfDrivingCar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_NAME "self_driving_car")

# Project include directory
include_directories(include)

set(SFML_STATIC_LIBRARIES ON)
# Find SFML 3+ package and its components
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

# List all source files explicitly
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
    src/Game.cpp
)

# Create the executable target
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Link SFML libraries using modern imported targets
target_link_libraries(${EXECUTABLE_NAME} PUBLIC SFML::Graphics SFML::Window SFML::System)

# --- Copy assets directory to build directory ---
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ASSETS_DESTINATION "${CMAKE_BINARY_DIR}/assets")

if(EXISTS ${ASSETS_DIR})
    file(GLOB ASSET_FILES "${ASSETS_DIR}/*.*")
    if(ASSET_FILES)
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

# --- Copy backups directory to build directory ---
set(BACKUPS_DIR "${CMAKE_SOURCE_DIR}/backups")
set(BACKUPS_DESTINATION "${CMAKE_BINARY_DIR}/backups")

if(EXISTS ${BACKUPS_DIR})
    file(GLOB BACKUP_FILES "${BACKUPS_DIR}/*.*")
    if(BACKUP_FILES)
        add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${BACKUPS_DESTINATION}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BACKUP_FILES} ${BACKUPS_DESTINATION}
            COMMENT "Copying BACKUPS..."
        )
    else()
        message(WARNING "BACKUPS directory exists at ${BACKUPS_DIR}, but no files matching '*.*' were found. Skipping backup copying.")
    endif()
else()
    message(WARNING "BACKUPS directory not found at ${BACKUPS_DIR}. Skipping backup copying.")
endif()
