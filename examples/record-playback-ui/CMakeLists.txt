# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesRecord-Playback-UI)
set(OpenCV_DIR "D:/Realsense_L515/opencv/build/x64/vc16/lib")
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message("OpenCV Found")
    include_directories("D:/Realsense_L515/opencv/build/include" "D:/Realsense_L515/opencv/build/include/opencv2") 
    add_executable(opencvtest main.cpp)
    target_link_libraries(opencvtest ${OpenCV_LIBS})
else()
    message(FATAL_ERROR "OpenCV Not Found")
endif()

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-record-playback-ui rs-record-playback-ui.cpp ../example.hpp ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp )
    set_property(TARGET rs-record-playback-ui PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-record-playback-ui ${DEPENDENCIES} ${OpenCV_LIBS})
    include_directories(rs-record-playback-ui ../ ../../third-party/tclap/include ../../third-party/imgui) 
    set_target_properties (rs-record-playback-ui PROPERTIES FOLDER "Examples")
    install(TARGETS rs-record-playback-ui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
