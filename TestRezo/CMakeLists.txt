cmake_minimum_required(VERSION 3.15)

project(TestRezo)

set(CMAKE_CXX_STANDARD 17)

# Define paths
set(EOS_SDK_PATH "${CMAKE_SOURCE_DIR}/SDK")
set(EOS_DLL "${EOS_SDK_PATH}/Bin/EOSSDK-Win64-Shipping.dll")
set(EOS_LIB "${EOS_SDK_PATH}/Lib/EOSSDK-Win64-Shipping.lib")

# Grab source files
file(GLOB_RECURSE SOURCES "Source/*.cpp")

# Add main executable
add_executable(TestRezo ${SOURCES})

# Add Include path
target_include_directories(TestRezo PRIVATE "${EOS_SDK_PATH}/Include")

# Link library
target_link_libraries(TestRezo PRIVATE "${EOS_LIB}")

# Copy EOS DLL to build output
add_custom_command(TARGET TestRezo POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${EOS_DLL}"
      $<TARGET_FILE_DIR:TestRezo>
)