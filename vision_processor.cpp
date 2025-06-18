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

// Removed captureWindowScreenshot - unused
// Removed findButtons - unused
// Removed findTextFields - unused
// Removed findTemplateMatches - unused

std::string VisionProcessor::captureScreenshot()
{
    auto now = std::chrono::system_clock::now();
    auto time_point = std::chrono::system_clock::to_time_t(now);
    std::tm tm_struct = *std::localtime(&time_point); // Use std::tm for formatting

    std::stringstream ss_filename;
    // Format timestamp for filename to avoid issues with special characters in default std::time_t string
    ss_filename << temp_directory << "/screenshot_"
                << std::put_time(&tm_struct, "%Y%m%d_%H%M%S")
                << ".png"; // Default to PNG
    std::string filename = ss_filename.str();

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    if (opencv_available) {
        HDC hScreenDC = GetDC(NULL);
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

        BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

        BITMAPINFOHEADER bi;
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = -height; // Negative height to indicate top-down DIB
        bi.biPlanes = 1;
        bi.biBitCount = 32; // 32-bit for easier OpenCV processing with alpha
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        cv::Mat mat(height, width, CV_8UC4); // 4 channels for BGRA

        GetDIBits(hScreenDC, hBitmap, 0, (UINT)height, mat.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

        try {
            if (cv::imwrite(filename, mat)) {
                std::cout << "📸 Screenshot saved as PNG: " << filename << std::endl;
            } else {
                std::cerr << "❌ Failed to save screenshot as PNG using OpenCV: " << filename << std::endl;
                // Potentially fallback to BMP if PNG fails, or just return empty
                filename = "";
            }
        } catch (const cv::Exception& ex) {
            std::cerr << "❌ OpenCV exception when saving PNG: " << ex.what() << std::endl;
            filename = "";
        }

        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);

        return filename;

    } else {
        // Fallback to BMP saving if OpenCV is not available
        // This is a basic GDI capture.
        std::stringstream ss_bmp_filename;
        ss_bmp_filename << temp_directory << "/screenshot_"
                        << std::put_time(&tm_struct, "%Y%m%d_%H%M%S")
                        << ".bmp";
        filename = ss_bmp_filename.str();
        std::cout << "📸 OpenCV not available, attempting to save screenshot as BMP: " << filename << std::endl;

        HDC hScreenDC = GetDC(NULL);
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
        BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

        BITMAPFILEHEADER bmfHeader;
        BITMAPINFOHEADER bi;
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = height;
        bi.biPlanes = 1;
        bi.biBitCount = 24; // BMP typically 24-bit
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0; // Can be 0 for BI_RGB
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        DWORD dwBmpSize = ((width * bi.biBitCount + 31) / 32) * 4 * height;
        HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
        if (!hDIB) {
             std::cerr << "❌ GlobalAlloc failed for BMP DIB" << std::endl;
             DeleteDC(hMemoryDC); ReleaseDC(NULL, hScreenDC); return "";
        }
        char *lpbitmap = (char *)GlobalLock(hDIB);
        if (!lpbitmap) {
            std::cerr << "❌ GlobalLock failed for BMP DIB" << std::endl;
            GlobalFree(hDIB); DeleteDC(hMemoryDC); ReleaseDC(NULL, hScreenDC); return "";
        }

        GetDIBits(hScreenDC, hBitmap, 0, (UINT)height, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

        std::ofstream file(filename, std::ios::binary);
        if (file.is_open()) {
            DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
            bmfHeader.bfSize = dwSizeofDIB;
            bmfHeader.bfType = 0x4D42; // 'BM'

            file.write((char *)&bmfHeader, sizeof(BITMAPFILEHEADER));
            file.write((char *)&bi, sizeof(BITMAPINFOHEADER));
            file.write(lpbitmap, dwBmpSize);
            file.close();
            std::cout << "📸 Screenshot saved as BMP: " << filename << std::endl;
        } else {
            std::cerr << "❌ Failed to open file to save BMP: " << filename << std::endl;
            filename = "";
        }

        GlobalUnlock(hDIB);
        GlobalFree(hDIB);
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);

        return filename;
    }
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

    // TODO: Implement advanced UI element detection using OpenCV (e.g., template matching, feature detection, or an ML model).
    // The current generic contour detection was removed as it's not specific enough for UI elements and can be noisy.
    // The function now primarily relies on addCommonWindowsElements or future, more targeted detection methods.
    try
    {
        if (opencv_available) {
            cv::Mat image = cv::imread(image_path);
            if (image.empty())
            {
                std::cerr << "Could not load image for UI element detection: " << image_path << std::endl;
                // Fall through to addCommonWindowsElements
            }
            else
            {
                // Placeholder for where more advanced OpenCV detection would go.
                // For example, if you had templates for common icons:
                // std::vector<UIElement> found_templates = findTemplateMatches(image_path, "path_to_template.png");
                // elements.insert(elements.end(), found_templates.begin(), found_templates.end());
                std::cout << "ℹ️ detectUIElements: OpenCV is available, but no specific advanced detection is implemented yet. Image loaded, path: " << image_path << std::endl;
            }
        } else {
            std::cout << "ℹ️ detectUIElements: OpenCV not available. Skipping image-based detection." << std::endl;
        }
    }
    catch (const cv::Exception &e) // Catch specific OpenCV exceptions
    {
        std::cerr << "❌ OpenCV exception in detectUIElements: " << e.what() << std::endl;
    }
    catch (const std::exception &e) // Catch other standard exceptions
    {
        std::cerr << "❌ Standard exception in detectUIElements: " << e.what() << std::endl;
    }

    // Add common Windows UI elements as a baseline or fallback.
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
    // TODO: Placeholder Implementation: Implement robust OCR using a library like Tesseract.
    // The current version is a basic simulation and does not perform actual OCR.
    std::string extracted_text = "OCR not implemented.";

    if (opencv_available) {
        try
        {
            cv::Mat image = cv::imread(image_path, cv::IMREAD_GRAYSCALE);
            if (image.empty())
            {
                std::cerr << "❌ Could not load image for OCR: " << image_path << std::endl;
                return "OCR Error: Could not load image.";
            }

            // The morphological operations below are NOT OCR. They were a very basic simulation.
            // Commenting them out as per the plan to remove non-functional "trash code".
            /*
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
            cv::Mat opened;
            cv::morphologyEx(image, opened, cv::MORPH_OPEN, kernel);

            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(opened, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            extracted_text = "Simulated text regions detected: " + std::to_string(contours.size());
            */
            std::cout << "ℹ️ extractTextFromImage: OpenCV available, image loaded. Actual OCR needs implementation (e.g., Tesseract)." << std::endl;

        }
        catch (const cv::Exception &e)
        {
            std::cerr << "❌ OpenCV exception in extractTextFromImage: " << e.what() << std::endl;
            extracted_text = "OCR Error: OpenCV exception.";
        }
        catch (const std::exception &e)
        {
            std::cerr << "❌ Standard exception in extractTextFromImage: " << e.what() << std::endl;
            extracted_text = "OCR Error: Standard exception.";
        }
    } else {
        std::cout << "ℹ️ extractTextFromImage: OpenCV not available. Cannot perform even simulated OCR." << std::endl;
        extracted_text = "OCR Error: OpenCV not available.";
    }

    return extracted_text; // Returns "OCR not implemented." or an error string.
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

// Removed scrollToElement - unused
// Removed compareScreenshots - unused
// Removed createElementsJson - unused
