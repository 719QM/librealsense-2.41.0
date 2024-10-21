# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesRecord-Playback-UI)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-record-playback-ui rs-record-playback-ui.cpp ../example.hpp ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp)
    set_property(TARGET rs-record-playback-ui PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-record-playback-ui ${DEPENDENCIES})
    include_directories(rs-record-playback-ui ../ ../../third-party/tclap/include ../../third-party/imgui)
    set_target_properties (rs-record-playback-ui PROPERTIES FOLDER "Examples")
    install(TARGETS rs-record-playback-ui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
