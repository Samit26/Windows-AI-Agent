#include "vision_processor.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <psapi.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

VisionProcessor::VisionProcessor() : temp_directory("temp/vision"), opencv_available(true)
{
    std::filesystem::create_directories(temp_directory);
    std::cout << "✅ OpenCV available for advanced vision processing" << std::endl;
}

VisionProcessor::~VisionProcessor()
{
    // Cleanup if needed
}

std::string VisionProcessor::captureScreenshot()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << temp_directory << "/screenshot_" << time_t << ".bmp";
    std::string filename = ss.str();

    // Get the device context of the entire screen
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    // Save bitmap to file
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((width * bi.biBitCount + 31) / 32) * 4 * height;

    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char *lpbitmap = (char *)GlobalLock(hDIB);

    GetDIBits(hScreenDC, hBitmap, 0, (UINT)height, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    std::ofstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
        bmfHeader.bfSize = dwSizeofDIB;
        bmfHeader.bfType = 0x4D42; // BM

        file.write((char *)&bmfHeader, sizeof(BITMAPFILEHEADER));
        file.write((char *)&bi, sizeof(BITMAPINFOHEADER));
        file.write(lpbitmap, dwBmpSize);
        file.close();
    }

    // Cleanup
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return filename;
}

ScreenAnalysis VisionProcessor::analyzeCurrentScreen()
{
    ScreenAnalysis analysis;

    // Capture screenshot
    analysis.screenshot_path = captureScreenshot();

    // Get active window info
    HWND activeWindow = getActiveWindow();
    analysis.window_title = getWindowTitle(activeWindow);
    analysis.application_name = getApplicationName(activeWindow);

    // Detect UI elements
    analysis.elements = detectUIElements(analysis.screenshot_path);

    // Generate overall description
    analysis.overall_description = generateScreenDescription(analysis);

    // Add metadata
    analysis.metadata = {
        {"timestamp", std::time(nullptr)},
        {"window_handle", reinterpret_cast<uintptr_t>(activeWindow)},
        {"screen_resolution", {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)}},
        {"element_count", analysis.elements.size()}};

    return analysis;
}

std::vector<UIElement> VisionProcessor::detectUIElements(const std::string &image_path)
{
    std::vector<UIElement> elements;

    try
    {
        cv::Mat image = cv::imread(image_path);
        if (image.empty())
        {
            std::cerr << "Could not load image: " << image_path << std::endl;
            return elements;
        }

        // Convert to grayscale
        cv::Mat gray;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

        // Detect buttons using edge detection and contours
        cv::Mat edges;
        cv::Canny(gray, edges, 50, 150);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (size_t i = 0; i < contours.size(); i++)
        {
            cv::Rect rect = cv::boundingRect(contours[i]);

            // Filter by size (reasonable button/element sizes)
            if (rect.width > 20 && rect.height > 10 && rect.width < 500 && rect.height < 200)
            {
                UIElement element;
                element.x = rect.x;
                element.y = rect.y;
                element.width = rect.width;
                element.height = rect.height;
                element.type = "button_candidate";
                element.confidence = 0.6;
                element.id = "element_" + std::to_string(elements.size());
                // Extract text from this region (basic OCR simulation)
                cv::Mat roi = gray(rect);
                element.text = "text_region_" + std::to_string(i);

                elements.push_back(element);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "OpenCV error in detectUIElements: " << e.what() << std::endl;
    }

    // Add common Windows UI elements as fallback
    addCommonWindowsElements(elements);

    return elements;
}

void VisionProcessor::addCommonWindowsElements(std::vector<UIElement> &elements)
{
    // Add Start button (bottom-left corner)
    UIElement start_button;
    start_button.x = 0;
    start_button.y = GetSystemMetrics(SM_CYSCREEN) - 40;
    start_button.width = 50;
    start_button.height = 40;
    start_button.type = "button";
    start_button.text = "Start Button";
    start_button.description = "Windows Start Menu Button";
    start_button.confidence = 0.9;
    start_button.id = "start_button";
    elements.push_back(start_button);

    // Add Search box (next to start button)
    UIElement search_box;
    search_box.x = 60;
    search_box.y = GetSystemMetrics(SM_CYSCREEN) - 35;
    search_box.width = 300;
    search_box.height = 30;
    search_box.type = "text_field";
    search_box.text = "Search Box";
    search_box.description = "Windows Search Box";
    search_box.confidence = 0.9;
    search_box.id = "search_box";
    elements.push_back(search_box);

    // Add entire taskbar
    UIElement taskbar;
    taskbar.x = 0;
    taskbar.y = GetSystemMetrics(SM_CYSCREEN) - 40;
    taskbar.width = GetSystemMetrics(SM_CXSCREEN);
    taskbar.height = 40;
    taskbar.type = "container";
    taskbar.text = "Taskbar";
    taskbar.description = "Windows Taskbar";
    taskbar.confidence = 0.95;
    taskbar.id = "taskbar";
    elements.push_back(taskbar);
}

std::string VisionProcessor::extractTextFromImage(const std::string &image_path)
{
    // Basic OCR simulation - in real implementation would use Tesseract
    std::string extracted_text = "";

    try
    {
        cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
        if (!image.empty())
        {
            // Simple text detection using morphological operations
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
            cv::Mat opened;
            cv::morphologyEx(image, opened, cv::MORPH_OPEN, kernel);

            // Find text regions (this is very basic)
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(opened, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            extracted_text = "Text regions detected: " + std::to_string(contours.size());
        }
    }
    catch (const std::exception &e)
    {
        extracted_text = "OCR error: " + std::string(e.what());
    }

    return extracted_text;
}

UIElement VisionProcessor::findElementByText(const std::string &text, const ScreenAnalysis &analysis)
{
    for (const auto &element : analysis.elements)
    {
        if (element.text.find(text) != std::string::npos)
        {
            return element;
        }
    }

    // Return empty element if not found
    UIElement empty_element;
    empty_element.confidence = 0.0;
    return empty_element;
}

UIElement VisionProcessor::findElementByType(const std::string &type, const ScreenAnalysis &analysis)
{
    for (const auto &element : analysis.elements)
    {
        if (element.type == type)
        {
            return element;
        }
    }

    // Return empty element if not found
    UIElement empty_element;
    empty_element.confidence = 0.0;
    return empty_element;
}

bool VisionProcessor::clickElement(const UIElement &element)
{
    if (element.confidence == 0.0)
    {
        return false;
    }

    // Calculate center point
    int centerX = element.x + element.width / 2;
    int centerY = element.y + element.height / 2;

    // Move cursor and click
    SetCursorPos(centerX, centerY);
    Sleep(100); // Small delay

    // Simulate mouse click
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    Sleep(50);

    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool VisionProcessor::typeAtElement(const UIElement &element, const std::string &text)
{
    // First click on the element to focus it
    if (!clickElement(element))
    {
        return false;
    }

    Sleep(200); // Wait for focus

    // Type the text
    for (char c : text)
    {
        SHORT vk = VkKeyScanA(c);
        if (vk != -1)
        {
            INPUT input[2] = {0};

            // Key down
            input[0].type = INPUT_KEYBOARD;
            input[0].ki.wVk = vk & 0xFF;

            // Key up
            input[1].type = INPUT_KEYBOARD;
            input[1].ki.wVk = vk & 0xFF;
            input[1].ki.dwFlags = KEYEVENTF_KEYUP;

            SendInput(2, input, sizeof(INPUT));
            Sleep(30); // Small delay between keystrokes
        }
    }

    return true;
}

HWND VisionProcessor::getActiveWindow()
{
    return GetForegroundWindow();
}

std::string VisionProcessor::getWindowTitle(HWND window)
{
    if (!window)
        return "";

    char buffer[256];
    GetWindowTextA(window, buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string VisionProcessor::getApplicationName(HWND window)
{
    if (!window)
        return "";

    DWORD processId;
    GetWindowThreadProcessId(window, &processId);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess)
    {
        char buffer[MAX_PATH];
        DWORD bufferSize = MAX_PATH;
        if (QueryFullProcessImageNameA(hProcess, 0, buffer, &bufferSize))
        {
            std::string fullPath(buffer);
            // Extract just the filename
            size_t lastSlash = fullPath.find_last_of("\\/");
            if (lastSlash != std::string::npos)
            {
                CloseHandle(hProcess);
                return fullPath.substr(lastSlash + 1);
            }
            CloseHandle(hProcess);
            return fullPath;
        }
        CloseHandle(hProcess);
    }

    return "Unknown";
}

std::string VisionProcessor::generateScreenDescription(const ScreenAnalysis &analysis)
{
    std::stringstream description;

    description << "Screen Analysis Summary:\n";
    description << "Application: " << analysis.application_name << "\n";
    description << "Window Title: " << analysis.window_title << "\n";
    description << "UI Elements Found: " << analysis.elements.size() << "\n\n";

    description << "Detected Elements:\n";
    for (size_t i = 0; i < analysis.elements.size() && i < 10; ++i)
    {
        const auto &element = analysis.elements[i];
        description << "- " << element.type << " at (" << element.x << "," << element.y
                    << ") size " << element.width << "x" << element.height;
        if (!element.text.empty())
        {
            description << " text: \"" << element.text << "\"";
        }
        description << "\n";
    }

    if (analysis.elements.size() > 10)
    {
        description << "... and " << (analysis.elements.size() - 10) << " more elements\n";
    }

    return description.str();
}

void VisionProcessor::setTempDirectory(const std::string &path)
{
    temp_directory = path;
    std::filesystem::create_directories(temp_directory);
}

bool VisionProcessor::isOpenCVAvailable() const
{
    return opencv_available;
}

std::string VisionProcessor::saveScreenshot(const std::string &filename)
{
    if (filename.empty())
    {
        return captureScreenshot();
    }
    else
    {
        std::string fullPath = temp_directory + "/" + filename;
        // Implementation would copy current screenshot to named file
        return fullPath;
    }
}

std::vector<UIElement> VisionProcessor::findElementsContaining(const std::string &text, const ScreenAnalysis &analysis)
{
    std::vector<UIElement> matching_elements;

    for (const auto &element : analysis.elements)
    {
        if (element.text.find(text) != std::string::npos)
        {
            matching_elements.push_back(element);
        }
    }

    return matching_elements;
}
