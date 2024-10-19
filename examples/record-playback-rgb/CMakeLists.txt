# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesRecord-Playback-RGB)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-record-playback-rgb rs-record-playback-rgb.cpp ../example.hpp ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp)
    set_property(TARGET rs-record-playback-rgb PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-record-playback-rgb ${DEPENDENCIES})
    include_directories(rs-record-playback-rgb ../ ../../third-party/tclap/include ../../third-party/imgui)
    set_target_properties (rs-record-playback-rgb PROPERTIES FOLDER "Examples")
    install(TARGETS rs-record-playback-rgb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
