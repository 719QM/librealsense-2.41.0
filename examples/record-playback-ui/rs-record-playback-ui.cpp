// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <chrono>

#include <imgui.h>
#include "imgui_impl_glfw.h"

// Includes for time display
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include <windows.h>
#include <commdlg.h>  // For file dialog
#include <string>


// Helper function for dispaying time conveniently
std::string pretty_time(std::chrono::nanoseconds duration);
// Helper function for rendering a seek bar
void draw_seek_bar(rs2::playback& playback, int* seek_pos, float2& location, float width);

std::wstring SaveFileDialog()
{
    wchar_t file_name[MAX_PATH] = {0};  // 使用宽字符

    OPENFILENAMEW ofn;  // 使用宽字符的结构体OPENFILENAMEW
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"All Files\0*.*\0";  // 使用宽字符过滤器
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;  // 允许覆盖已存在的文件
    ofn.lpstrDefExt = L"";  // 默认扩展名

    std::wstring filePath;

    // 打开保存文件对话框
    if (GetSaveFileNameW(&ofn))  // 使用宽字符版API
    {
        filePath = file_name;  // 获取用户选择的路径和文件名
    }
    std::wcout << L"File path: " << filePath << std::endl;

    return filePath;
}
void SaveFileDialog_noopen(const std::string& filepath)
{
    // 转换 std::string 到 std::wstring
    std::wstring wfilepath(filepath.begin(), filepath.end());

    // 打开文件以便保存
    std::ofstream file(wfilepath, std::ios::binary);
}

std::wstring OpenFileDialog()
{
    wchar_t file_name[MAX_PATH] = {0};  // 使用宽字符

    OPENFILENAMEW ofn;  // 使用宽字符的结构体OPENFILENAMEW
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"All Files\0*.*\0";  // 使用宽字符过滤器
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"";

    std::wstring openfilePath;

    if (GetOpenFileNameW(&ofn))  // 使用宽字符版API
    {
        openfilePath = file_name;
    }

    return openfilePath;
}
// 将std::wstring转换为std::string
std::string WStringToString(const std::wstring& wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed, NULL, NULL);
    return str;
}
int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Record and Playback (UI)");
    ImGui_ImplGlfw_Init(app, false);

    // Create booleans to control GUI (recorded - allow play button, recording - show 'recording to file' text)
    bool recorded = false;
    bool recording = false;
    bool rgb_frame = false;
    bool depth_frame = false;
    bool both_frames = false;
    bool button_visible = true;
    bool record_bag =false;
    bool recordmp4 = false;
    bool long_video = false;

    // Declare a texture for the depth image on the GPU
    texture depth_image;
    texture rgb_image;

    // Declare frameset and frames which will hold the data from the camera
    rs2::frameset frames;
    rs2::frame depth;
    rs2::frame rgb;

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    int camera_fps;
    int video_fps;
    std::cout << "Please enter the camera fps: ";
    std::cin >> camera_fps;
    std::cout << "Please enter the video fps: ";
    std::cin >> video_fps; 


    // Create a shared pointer to a pipeline
    auto pipe = std::make_shared<rs2::pipeline>();

    rs2::config initial_cfg;
    initial_cfg.enable_stream(RS2_STREAM_COLOR, 1280, 720, RS2_FORMAT_RGB8, camera_fps);
    // initial_cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
    // Start streaming with default configuration
    pipe->start(initial_cfg);

    // Initialize a shared pointer to a device with the current device on the pipeline
    rs2::device device = pipe->get_active_profile().get_device();

    // Create a variable to control the seek bar
    int seek_pos;

    // Create a video writer to save the frames
    int fps = 6;

    std::string filePath;
    std::string openfilePath;
    cv::VideoWriter video;
    // for long video
    int frameCout = 0;
    int video_index = 0;
    std::string numberedFilePath;
    std::ofstream file;

    // While application is running
    while(app) {
        // Flags for displaying ImGui window 窗口设置，不显示标题栏，不显示滚动条，不显示保存设置，不显示标题栏，不可调整大小，不可移动
        static const int flags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove;

        ImGui_ImplGlfw_NewFrame(1);
        ImGui::SetNextWindowSize({ app.width(), app.height() }); 
        ImGui::Begin("app", nullptr, flags);
        ImGui::SetCursorPos({ app.width() / 2 - 300, 3 * app.height() / 5 + 110 });
        if(button_visible)
        {
            ImGui::SetCursorPos({ app.width() / 2-100, 3 * app.height() / 5 + 110 });
            if (ImGui::Button("RGB", { 50, 50 }))
            {
                rgb_frame = true;
                depth_frame = false;
                both_frames = false;
                button_visible = false;
            }
            ImGui::SetCursorPos({ app.width() / 2 , 3 * app.height() / 5 + 110 });
            if (ImGui::Button("Depth", { 50, 50 }))
            {
                rgb_frame = false;
                depth_frame = true;
                both_frames = false;
                button_visible = false;
            }
            ImGui::SetCursorPos({ app.width() / 2 + 100, 3 * app.height() / 5 + 110 });
            if (ImGui::Button("Both", { 50, 50 }))
            {
                rgb_frame = false;
                depth_frame = false;
                both_frames = true;
                button_visible = false; 
            }
        }

        if(rgb_frame || depth_frame || both_frames)
        {
            // OpenFileDialog();
            ImGui::SetCursorPos({ app.width() / 2 - 200, 3 * app.height() / 5 + 110 });
            if (ImGui::Button(" return", { 50, 50 }))
            {
                rgb_frame = false;
                depth_frame = false;
                both_frames = false;
                button_visible = true;
                long_video  = false;
            }
            // If the device is sreaming live and not from a file 如果设备正在实时流并且不是从文件中读取的
            if (!device.as<rs2::playback>())
            {
                frames = pipe->wait_for_frames(); // wait for next set of frames from the camera
                if(rgb_frame)
                {
                    rgb = frames.get_color_frame(); // Find the color data
                }
                else if(depth_frame)
                {
                    depth = color_map.process(frames.get_depth_frame()); // Find and colorize the depth data
                }
                else if(both_frames)
                {
                depth = color_map.process(frames.get_depth_frame()); // Find and colorize the depth data
                rgb = frames.get_color_frame(); // Find the color data
                }
                
            }

            // Set options for the ImGui buttons 设置按钮的样式：选中时的背景颜色，按钮的背景颜色，鼠标悬停时的背景颜色，按钮按下时的背景颜色，按钮的圆角
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1, 1, 1, 1 });
            ImGui::PushStyleColor(ImGuiCol_Button, { 36 / 255.f, 44 / 255.f, 51 / 255.f, 1 });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 40 / 255.f, 170 / 255.f, 90 / 255.f, 1 });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 36 / 255.f, 44 / 255.f, 51 / 255.f, 1 });
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);

            if (!device.as<rs2::playback>()) // Disable recording while device is playing 检查是否在播放录制的文件，如果是则禁用录制按钮
            {
                ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 5 + 90});
                ImGui::Text("Click 'record' to start recording");
                ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 5 + 110 });
                if (ImGui::Button("record\nbag", { 50, 50 }))  // 创建一个按钮，按钮的大小为50*50，如果按钮被点击则返回true
                {
                    // If it is the start of a new recording (device is not a recorder yet)
                    if (!device.as<rs2::recorder>())  //  如果没有在录制
                    {
                        std::wstring sourceFilePath;
                        // 打开文件选择对话框，获取源文件路径
                        if(filePath.empty()){
                            sourceFilePath = SaveFileDialog();
                            if(!sourceFilePath.empty())
                            {
                                pipe->stop(); // Stop the pipeline with the default configuration
                                pipe = std::make_shared<rs2::pipeline>();  //  创建一个新的pipeline，make_shared是一个模板函数，用于创建一个shared_ptr对象
                                rs2::config cfg; // Declare a new configuration
                                filePath = WStringToString(sourceFilePath);  // 转换路径
                                cfg.enable_record_to_file(filePath);  //  允许将数据记录到文件中
                                pipe->start(cfg); //File will be opened at this point
                                device = pipe->get_active_profile().get_device();  //  获取当前设备
                            }
                            else{
                                std::wcout << L"No file path selected." << std::endl;  // 提示用户未选择文件路径
                            }
                        }
                    } 
                    else
                    { // If the recording is resumed after a pause, there's no need to reset the shared pointer
                        device.as<rs2::recorder>().resume(); // rs2::recorder allows access to 'resume' function  如果是暂停后恢复录制，则无需重置共享指针
                    }
                    recording = true;
                    record_bag = true;
                    recordmp4 = false;
                }
                ImGui::SetCursorPos({ app.width() / 2, 3 * app.height() / 5 + 110 });
                if (ImGui::Button("record\nmp4", { 50, 50 }))  // 创建一个按钮，按钮的大小为50*50，如果按钮被点击则返回true
                {
                    std::wstring sourceFilePath;
                    if(filePath.empty())  //  如果路径是空的，也就是第一次写入时
                    {
                        sourceFilePath = SaveFileDialog();
                        filePath = WStringToString(sourceFilePath);  // 转换路径
                        if (video.isOpened()) {
                            video.release();  // 释放当前的 VideoWriter
                        }
                        if(rgb_frame)
                        {
                            video.open(filePath, cv::VideoWriter::fourcc('H', '2', '6', '4'), video_fps, cv::Size(1280, 720));
                        }
                        else if(depth_frame)
                        {
                            video.open(filePath, cv::VideoWriter::fourcc('F', 'F', 'V', '1'), video_fps, cv::Size(640, 480));
                        }
                        // cv::VideoWriter video(filePath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(1280, 720));
                    }
                    recording = true;
                    recordmp4 = true;
                    record_bag = false;
                }
                ImGui::SetCursorPos({ app.width() / 2, 3 * app.height() / 5 + 160 });
                if (ImGui::Button("record\nlong\nvideo", { 50, 50 }))  // 创建一个按钮，按钮的大小为50*50，如果按钮被点击则返回true
                {
                    std::wstring sourceFilePath;
                    if(filePath.empty())  //  如果路径是空的，也就是第一次写入时
                    {
                        sourceFilePath = SaveFileDialog();
                        filePath = WStringToString(sourceFilePath);  // 转换路径
                        if (video.isOpened()) {
                            video.release();  // 释放当前的 VideoWriter
                        }
                        std::string baseFilePath = filePath;
                        std::size_t dotPos = baseFilePath.find_last_of('.');  // 查找最后一个 '.' 的位置

                        // 如果找到了扩展名
                        if (dotPos != std::string::npos) {
                        std::stringstream ss;
                        ss << baseFilePath.substr(0, dotPos) << "_" << video_index << baseFilePath.substr(dotPos);  // 在扩展名之前插入编号
                        numberedFilePath = ss.str();
                        // 转换 std::string 到 std::wstring
                        std::wstring wfilepath(numberedFilePath.begin(), numberedFilePath.end());
                        // 打开文件以便保存
                        file.open(wfilepath, std::ios::binary);
                        video.open(numberedFilePath, cv::VideoWriter::fourcc('H', '2', '6', '4'), video_fps, cv::Size(1280, 720));
                        std::cout << "open mp4 video"<< numberedFilePath << std::endl;
                        // cv::VideoWriter video(filePath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(1280, 720));
                        }

                    }
                    recording = true;
                    recordmp4 = true;
                    record_bag = false;
                    long_video = true;
                }

                if(recording && recordmp4)
                {
                    if(long_video)
                    {
                        if(frameCout!=0 && frameCout % (60 * video_fps) == 0)
                        {
                            video_index++;
                            file.close();  
                            // 关闭当前视频文件
                            if (video.isOpened()) {
                                video.release();
                            }
                        std::string baseFilePath = filePath;
                        std::size_t dotPos = baseFilePath.find_last_of('.');  // 查找最后一个 '.' 的位置

                        // 如果找到了扩展名
                        if (dotPos != std::string::npos) {
                        std::stringstream ss;
                        ss << baseFilePath.substr(0, dotPos) << "_" << video_index << baseFilePath.substr(dotPos);  // 在扩展名之前插入编号
                        numberedFilePath = ss.str();
                        // 转换 std::string 到 std::wstring
                        std::wstring wfilepath(numberedFilePath.begin(), numberedFilePath.end());
                        // 打开文件以便保存
                        file.open(wfilepath, std::ios::binary);
                        video.open(numberedFilePath, cv::VideoWriter::fourcc('H', '2', '6', '4'), video_fps, cv::Size(1280, 720));
                        std::cout << "open mp4 video"<< numberedFilePath << std::endl;
                        // cv::VideoWriter video(filePath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(1280, 720));
                        }
                        }
                    }
                    if(rgb_frame)
                    {
                        rs2::video_frame rgb_video = rgb.as<rs2::video_frame>();
                        cv::Mat image(cv::Size(rgb_video.get_width(), rgb_video.get_height()), CV_8UC3, (void*)rgb.get_data(), cv::Mat::AUTO_STEP);
                        // 转换 BGR 到 RGB
                        cv::Mat image_rgb;
                        cv::cvtColor(image, image_rgb, cv::COLOR_BGR2RGB);
                        video.write(image_rgb);
                        frameCout ++;
                        // std::cout << "write frame" << std::endl;
                        std::cout <<frameCout << ":  "<< "Frame written to video: " << image_rgb.size() << std::endl;
                    }
                    else if(depth_frame)
                    {
                        // 假设 depth_frame 是深度图像
                        rs2::depth_frame depth_video = depth.as<rs2::depth_frame>();
                        
                        // 创建深度图像的 cv::Mat
                        cv::Mat depth_image(cv::Size(depth_video.get_width(), depth_video.get_height()), CV_16UC1, (void*)depth.get_data(), cv::Mat::AUTO_STEP);
                        
                        // 转换深度图像为伪彩色图像
                        cv::Mat depth_colored;
                        cv::normalize(depth_image, depth_image, 0, 255, cv::NORM_MINMAX, CV_16UC1); // 归一化深度图像
                        cv::applyColorMap(depth_image, depth_colored, cv::COLORMAP_JET); // 应用伪彩色映射

                        // 将伪彩色图像写入视频
                        video.write(depth_colored);
                        
                        std::cout << "Depth frame written to video: " << depth_colored.size() << std::endl;

                    }
                    
                }

                /*
                When pausing, device still holds the file.
                */
                if (device.as<rs2::recorder>() || (recording && recordmp4))  
                {
                    if (recording && record_bag)
                    {
                        ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 5 + 60 });
                        ImGui::TextColored({ 255 / 255.f, 64 / 255.f, 54 / 255.f, 1 }, "Recording to file '%s'",filePath.c_str());
                    }
                    if(recording && recordmp4)
                    {
                        ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 5 + 60 });
                        if(!long_video)
                        {
                            ImGui::TextColored({ 255 / 255.f, 64 / 255.f, 54 / 255.f, 1 }, "Recording to file '%s'",filePath.c_str());
                        }
                        else
                        {
                            ImGui::TextColored({ 255 / 255.f, 64 / 255.f, 54 / 255.f, 1 }, "Recording to file '%s'",numberedFilePath.c_str());
                        }
                    }
                    // Pause the playback if button is clicked
                    ImGui::SetCursorPos({ app.width() / 2 + 100, 3 * app.height() / 5 + 110 });
                    if (ImGui::Button("pause\nrecord", { 50, 50 }))  //  pause之后，device仍然持有文件，按下record之后会继续录制
                    {
                        if(record_bag)
                        {
                            device.as<rs2::recorder>().pause();
                        }
                        recording = false;
                    }

                    ImGui::SetCursorPos({ app.width() / 2 + 200, 3 * app.height() / 5 + 110 });
                    if (ImGui::Button(" stop\nrecord", { 50, 50 }))  
                    {
                        if(record_bag)
                        {
                        pipe->stop(); // Stop the pipeline that holds the file and the recorder
                        pipe = std::make_shared<rs2::pipeline>(); //Reset the shared pointer with a new pipeline
                        pipe->start(); // Resume streaming with default configuration
                        device = pipe->get_active_profile().get_device();
                        }
                        if(recordmp4)
                        {
                            video.release();
                            std::cout << "release mp4 video" << std::endl;
                            if(long_video)
                            {
                                file.close();
                            }
                        }
                        recorded = true; // Now we can run the file
                        recording = false;
                        long_video = false;
                        filePath.clear();
                    }
                }
            }

            // After a recording is done, we can play it
            ImGui::SetCursorPos({ app.width() / 2 -300, 3 * app.height() / 5 + 110 });
            if(ImGui::Button("play\nbag", { 50, 50 }))
            {
                record_bag = true;
                std::wstring openFilePathw;
                if(openfilePath.empty())
                {
                    openFilePathw = OpenFileDialog();
                    openfilePath = WStringToString(openFilePathw);  // 转换路径
                }
                if (!device.as<rs2::playback>())
                {
                    pipe->stop(); // Stop streaming with default configuration
                    pipe = std::make_shared<rs2::pipeline>();
                    rs2::config cfg;
                    cfg.enable_device_from_file(openfilePath);
                    pipe->start(cfg); //File will be opened in read mode at this point
                    device = pipe->get_active_profile().get_device();
                }
                else
                {
                    device.as<rs2::playback>().resume();
                }
            }
            if (recorded)
            {
                ImGui::SetCursorPos({ app.width() / 2 - 100, 4 * app.height() / 5 + 30 });
                ImGui::Text("Click 'play' to start playing");
                ImGui::SetCursorPos({ app.width() / 2 - 100, 4 * app.height() / 5 + 50});
                if (ImGui::Button("play", { 50, 50 }))
                {
                    if(record_bag)
                    {
                        if (!device.as<rs2::playback>())
                        {
                            pipe->stop(); // Stop streaming with default configuration
                            pipe = std::make_shared<rs2::pipeline>();
                            rs2::config cfg;
                            cfg.enable_device_from_file(filePath);
                            pipe->start(cfg); //File will be opened in read mode at this point
                            device = pipe->get_active_profile().get_device();
                        }
                        else
                        {
                            device.as<rs2::playback>().resume();
                        }
                    }
                }
            }

            // If device is playing a recording, we allow pause and stop
            if (device.as<rs2::playback>())
            {
                if(record_bag)
                {
                    rs2::playback playback = device.as<rs2::playback>();
                    if (pipe->poll_for_frames(&frames)) // Check if new frames are ready
                    {
                        depth = color_map.process(frames.get_depth_frame()); // Find and colorize the depth data for rendering
                        rgb = frames.get_color_frame(); // Find the color data
                    }

                    // Render a seek bar for the player
                    float2 location = { app.width() / 4, 4 * app.height() / 5 + 110 };
                    draw_seek_bar(playback , &seek_pos, location, app.width() / 2);

                    ImGui::SetCursorPos({ app.width() / 2, 4 * app.height() / 5 + 50 });
                    if (ImGui::Button(" pause\nplaying", { 50, 50 }))
                    {
                        playback.pause();
                    }

                    ImGui::SetCursorPos({ app.width() / 2 + 100, 4 * app.height() / 5 + 50 });
                    if (ImGui::Button("  stop\nplaying", { 50, 50 }))
                    {
                        pipe->stop();
                        pipe = std::make_shared<rs2::pipeline>();
                        pipe->start();
                        device = pipe->get_active_profile().get_device();
                    }
                }
            }

            ImGui::PopStyleColor(4);  
            ImGui::PopStyleVar();

            // Render depth frames from the default configuration, the recorder or the playback
            if(depth_frame)
            {
                depth_image.render(depth, { app.width() * 0.25f, app.height() * 0.25f, app.width() * 0.5f, app.height() * 0.75f  });
            }
            // depth_image.render(depth, { app.width() * 0.25f, app.height() * 0.25f, app.width() * 0.5f, app.height() * 0.75f  });
            if(rgb_frame)
            {
                rgb_image.render(rgb, { app.width() * 0.25f, app.height() * 0.25f, app.width() * 0.5f, app.height() * 0.75f  });
            }
            if(both_frames)
            {
                depth_image.render(depth, { app.width() * 0.25f, app.height() * 0.5f, app.width() * 0.25f, app.height() * 0.375f  });
                rgb_image.render(rgb, { app.width() * 0.5f, app.height() * 0.5f, app.width() * 0.25f, app.height() * 0.375f  });
            }
            // rgb_image.render(rgb, { app.width() * 0.25f, app.height() * 0.25f, app.width() * 0.5f, app.height() * 0.75f }); // Render RGB image 渲染RGB图像，在窗口的中间位置，大小为窗口的一半
        }
        ImGui::End();
        ImGui::Render();
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cout << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}


std::string pretty_time(std::chrono::nanoseconds duration)
{
    using namespace std::chrono;
    auto hhh = duration_cast<hours>(duration);
    duration -= hhh;
    auto mm = duration_cast<minutes>(duration);
    duration -= mm;
    auto ss = duration_cast<seconds>(duration);
    duration -= ss;
    auto ms = duration_cast<milliseconds>(duration);

    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(hhh.count() >= 10 ? 2 : 1) << hhh.count() << ':' <<
        std::setfill('0') << std::setw(2) << mm.count() << ':' <<
        std::setfill('0') << std::setw(2) << ss.count();
    return stream.str();
}

// 绘制进度条
void draw_seek_bar(rs2::playback& playback, int* seek_pos, float2& location, float width)
{
    int64_t playback_total_duration = playback.get_duration().count();
    auto progress = playback.get_position();
    double part = (1.0 * progress) / playback_total_duration;
    *seek_pos = static_cast<int>(std::max(0.0, std::min(part, 1.0)) * 100);
    auto playback_status = playback.current_status();
    ImGui::PushItemWidth(width);
    ImGui::SetCursorPos({ location.x, location.y });
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
    if (ImGui::SliderInt("##seek bar", seek_pos, 0, 100, "", true))
    {
        //Seek was dragged
        if (playback_status != RS2_PLAYBACK_STATUS_STOPPED) //Ignore seek when playback is stopped
        {
            auto duration_db = std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(playback.get_duration());
            auto single_percent = duration_db.count() / 100;
            auto seek_time = std::chrono::duration<double, std::nano>((*seek_pos) * single_percent);
            playback.seek(std::chrono::duration_cast<std::chrono::nanoseconds>(seek_time));
        }
    }
    std::string time_elapsed = pretty_time(std::chrono::nanoseconds(progress));
    ImGui::SetCursorPos({ location.x + width + 10, location.y });
    ImGui::Text("%s", time_elapsed.c_str());
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
}
