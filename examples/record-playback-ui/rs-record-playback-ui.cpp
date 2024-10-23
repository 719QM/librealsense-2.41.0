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
    wchar_t file_name[MAX_PATH] = {0};  // ʹ�ÿ��ַ�

    OPENFILENAMEW ofn;  // ʹ�ÿ��ַ��Ľṹ��OPENFILENAMEW
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"All Files\0*.*\0";  // ʹ�ÿ��ַ�������
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;  // �������Ѵ��ڵ��ļ�
    ofn.lpstrDefExt = L"";  // Ĭ����չ��

    std::wstring filePath;

    // �򿪱����ļ��Ի���
    if (GetSaveFileNameW(&ofn))  // ʹ�ÿ��ַ���API
    {
        filePath = file_name;  // ��ȡ�û�ѡ���·�����ļ���
    }
    std::wcout << L"File path: " << filePath << std::endl;

    return filePath;
}
void SaveFileDialog_noopen(const std::string& filepath)
{
    // ת�� std::string �� std::wstring
    std::wstring wfilepath(filepath.begin(), filepath.end());

    // ���ļ��Ա㱣��
    std::ofstream file(wfilepath, std::ios::binary);
}

std::wstring OpenFileDialog()
{
    wchar_t file_name[MAX_PATH] = {0};  // ʹ�ÿ��ַ�

    OPENFILENAMEW ofn;  // ʹ�ÿ��ַ��Ľṹ��OPENFILENAMEW
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"All Files\0*.*\0";  // ʹ�ÿ��ַ�������
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"";

    std::wstring openfilePath;

    if (GetOpenFileNameW(&ofn))  // ʹ�ÿ��ַ���API
    {
        openfilePath = file_name;
    }

    return openfilePath;
}
// ��std::wstringת��Ϊstd::string
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
        // Flags for displaying ImGui window �������ã�����ʾ������������ʾ������������ʾ�������ã�����ʾ�����������ɵ�����С�������ƶ�
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
            // If the device is sreaming live and not from a file ����豸����ʵʱ�����Ҳ��Ǵ��ļ��ж�ȡ��
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

            // Set options for the ImGui buttons ���ð�ť����ʽ��ѡ��ʱ�ı�����ɫ����ť�ı�����ɫ�������ͣʱ�ı�����ɫ����ť����ʱ�ı�����ɫ����ť��Բ��
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1, 1, 1, 1 });
            ImGui::PushStyleColor(ImGuiCol_Button, { 36 / 255.f, 44 / 255.f, 51 / 255.f, 1 });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 40 / 255.f, 170 / 255.f, 90 / 255.f, 1 });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 36 / 255.f, 44 / 255.f, 51 / 255.f, 1 });
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);

            if (!device.as<rs2::playback>()) // Disable recording while device is playing ����Ƿ��ڲ���¼�Ƶ��ļ�������������¼�ư�ť
            {
                ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 5 + 90});
                ImGui::Text("Click 'record' to start recording");
                ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 5 + 110 });
                if (ImGui::Button("record\nbag", { 50, 50 }))  // ����һ����ť����ť�Ĵ�СΪ50*50�������ť������򷵻�true
                {
                    // If it is the start of a new recording (device is not a recorder yet)
                    if (!device.as<rs2::recorder>())  //  ���û����¼��
                    {
                        std::wstring sourceFilePath;
                        // ���ļ�ѡ��Ի��򣬻�ȡԴ�ļ�·��
                        if(filePath.empty()){
                            sourceFilePath = SaveFileDialog();
                            if(!sourceFilePath.empty())
                            {
                                pipe->stop(); // Stop the pipeline with the default configuration
                                pipe = std::make_shared<rs2::pipeline>();  //  ����һ���µ�pipeline��make_shared��һ��ģ�庯�������ڴ���һ��shared_ptr����
                                rs2::config cfg; // Declare a new configuration
                                filePath = WStringToString(sourceFilePath);  // ת��·��
                                cfg.enable_record_to_file(filePath);  //  �������ݼ�¼���ļ���
                                pipe->start(cfg); //File will be opened at this point
                                device = pipe->get_active_profile().get_device();  //  ��ȡ��ǰ�豸
                            }
                            else{
                                std::wcout << L"No file path selected." << std::endl;  // ��ʾ�û�δѡ���ļ�·��
                            }
                        }
                    } 
                    else
                    { // If the recording is resumed after a pause, there's no need to reset the shared pointer
                        device.as<rs2::recorder>().resume(); // rs2::recorder allows access to 'resume' function  �������ͣ��ָ�¼�ƣ����������ù���ָ��
                    }
                    recording = true;
                    record_bag = true;
                    recordmp4 = false;
                }
                ImGui::SetCursorPos({ app.width() / 2, 3 * app.height() / 5 + 110 });
                if (ImGui::Button("record\nmp4", { 50, 50 }))  // ����һ����ť����ť�Ĵ�СΪ50*50�������ť������򷵻�true
                {
                    std::wstring sourceFilePath;
                    if(filePath.empty())  //  ���·���ǿյģ�Ҳ���ǵ�һ��д��ʱ
                    {
                        sourceFilePath = SaveFileDialog();
                        filePath = WStringToString(sourceFilePath);  // ת��·��
                        if (video.isOpened()) {
                            video.release();  // �ͷŵ�ǰ�� VideoWriter
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
                if (ImGui::Button("record\nlong\nvideo", { 50, 50 }))  // ����һ����ť����ť�Ĵ�СΪ50*50�������ť������򷵻�true
                {
                    std::wstring sourceFilePath;
                    if(filePath.empty())  //  ���·���ǿյģ�Ҳ���ǵ�һ��д��ʱ
                    {
                        sourceFilePath = SaveFileDialog();
                        filePath = WStringToString(sourceFilePath);  // ת��·��
                        if (video.isOpened()) {
                            video.release();  // �ͷŵ�ǰ�� VideoWriter
                        }
                        std::string baseFilePath = filePath;
                        std::size_t dotPos = baseFilePath.find_last_of('.');  // �������һ�� '.' ��λ��

                        // ����ҵ�����չ��
                        if (dotPos != std::string::npos) {
                        std::stringstream ss;
                        ss << baseFilePath.substr(0, dotPos) << "_" << video_index << baseFilePath.substr(dotPos);  // ����չ��֮ǰ������
                        numberedFilePath = ss.str();
                        // ת�� std::string �� std::wstring
                        std::wstring wfilepath(numberedFilePath.begin(), numberedFilePath.end());
                        // ���ļ��Ա㱣��
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
                            // �رյ�ǰ��Ƶ�ļ�
                            if (video.isOpened()) {
                                video.release();
                            }
                        std::string baseFilePath = filePath;
                        std::size_t dotPos = baseFilePath.find_last_of('.');  // �������һ�� '.' ��λ��

                        // ����ҵ�����չ��
                        if (dotPos != std::string::npos) {
                        std::stringstream ss;
                        ss << baseFilePath.substr(0, dotPos) << "_" << video_index << baseFilePath.substr(dotPos);  // ����չ��֮ǰ������
                        numberedFilePath = ss.str();
                        // ת�� std::string �� std::wstring
                        std::wstring wfilepath(numberedFilePath.begin(), numberedFilePath.end());
                        // ���ļ��Ա㱣��
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
                        // ת�� BGR �� RGB
                        cv::Mat image_rgb;
                        cv::cvtColor(image, image_rgb, cv::COLOR_BGR2RGB);
                        video.write(image_rgb);
                        frameCout ++;
                        // std::cout << "write frame" << std::endl;
                        std::cout <<frameCout << ":  "<< "Frame written to video: " << image_rgb.size() << std::endl;
                    }
                    else if(depth_frame)
                    {
                        // ���� depth_frame �����ͼ��
                        rs2::depth_frame depth_video = depth.as<rs2::depth_frame>();
                        
                        // �������ͼ��� cv::Mat
                        cv::Mat depth_image(cv::Size(depth_video.get_width(), depth_video.get_height()), CV_16UC1, (void*)depth.get_data(), cv::Mat::AUTO_STEP);
                        
                        // ת�����ͼ��Ϊα��ɫͼ��
                        cv::Mat depth_colored;
                        cv::normalize(depth_image, depth_image, 0, 255, cv::NORM_MINMAX, CV_16UC1); // ��һ�����ͼ��
                        cv::applyColorMap(depth_image, depth_colored, cv::COLORMAP_JET); // Ӧ��α��ɫӳ��

                        // ��α��ɫͼ��д����Ƶ
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
                    if (ImGui::Button("pause\nrecord", { 50, 50 }))  //  pause֮��device��Ȼ�����ļ�������record֮������¼��
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
                    openfilePath = WStringToString(openFilePathw);  // ת��·��
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
            // rgb_image.render(rgb, { app.width() * 0.25f, app.height() * 0.25f, app.width() * 0.5f, app.height() * 0.75f }); // Render RGB image ��ȾRGBͼ���ڴ��ڵ��м�λ�ã���СΪ���ڵ�һ��
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

// ���ƽ�����
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
