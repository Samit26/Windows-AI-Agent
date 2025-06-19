#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <psapi.h> // Windows API
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <curl/curl.h> // Added for network requests
#include <cstdlib>     // For std::getenv

#include "vision_processor.h" // Project-specific header

namespace { // Anonymous namespace for utility functions
    std::string base64_encode(const std::string& file_path) {
        std::ifstream image_file(file_path, std::ios::binary);
        if (!image_file.is_open()) {
            std::cerr << "Error opening image file for base64 encoding: " << file_path << std::endl;
            return "";
        }
        std::ostringstream os;
        os << image_file.rdbuf();
        std::string file_content = os.str();

        const std::string base64_chars =
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz"
                    "0123456789+/";

        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        for (char const& c : file_content) {
            char_array_3[i++] = c;
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; (i < 4); i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0'; // Use '\0' for padding

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            // char_array_4[3] should not be assigned if char_array_3[2] is padding.
            // The original code had char_array_4[3] = char_array_3[2] & 0x3f; which might be problematic with padding.
            // Let's adhere to the provided code for now, but this is a common spot for base64 bugs.

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

            while ((i++ < 3))
                ret += '=';
        }
        return ret;
    }

    // libcurl write callback function
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        try {
            s->append((char*)contents, newLength);
        } catch (std::bad_alloc& e) {
            // Handle memory problem
            std::cerr << "Failed to allocate memory in WriteCallback: " << e.what() << std::endl;
            return 0;
        }
        return newLength;
    }
} // end anonymous namespace

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

    if (opencv_available)
    {
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

        try
        {
            if (cv::imwrite(filename, mat))
            {
                std::cout << "📸 Screenshot saved as PNG: " << filename << std::endl;
            }
            else
            {
                std::cerr << "❌ Failed to save screenshot as PNG using OpenCV: " << filename << std::endl;
                // Potentially fallback to BMP if PNG fails, or just return empty
                filename = "";
            }
        }
        catch (const cv::Exception &ex)
        {
            std::cerr << "❌ OpenCV exception when saving PNG: " << ex.what() << std::endl;
            filename = "";
        }

        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);

        return filename;
    }
    else
    {
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
        if (!hDIB)
        {
            std::cerr << "❌ GlobalAlloc failed for BMP DIB" << std::endl;
            DeleteDC(hMemoryDC);
            ReleaseDC(NULL, hScreenDC);
            return "";
        }
        char *lpbitmap = (char *)GlobalLock(hDIB);
        if (!lpbitmap)
        {
            std::cerr << "❌ GlobalLock failed for BMP DIB" << std::endl;
            GlobalFree(hDIB);
            DeleteDC(hMemoryDC);
            ReleaseDC(NULL, hScreenDC);
            return "";
        }

        GetDIBits(hScreenDC, hBitmap, 0, (UINT)height, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

        std::ofstream file(filename, std::ios::binary);
        if (file.is_open())
        {
            DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
            bmfHeader.bfSize = dwSizeofDIB;
            bmfHeader.bfType = 0x4D42; // 'BM'

            file.write((char *)&bmfHeader, sizeof(BITMAPFILEHEADER));
            file.write((char *)&bi, sizeof(BITMAPINFOHEADER));
            file.write(lpbitmap, dwBmpSize);
            file.close();
            std::cout << "📸 Screenshot saved as BMP: " << filename << std::endl;
        }
        else
        {
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
    // analysis.elements = detectUIElements(analysis.screenshot_path); // Removed
    // Generate overall description
    // analysis.overall_description = generateScreenDescription(analysis); // Removed

    // Call Qwen analysis
    ScreenAnalysis qwen_analysis = analyzeImageWithQwen(analysis.screenshot_path);
    analysis.overall_description = qwen_analysis.overall_description;
    analysis.elements = qwen_analysis.elements; // If Qwen provides elements

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
    std::cout << "INFO: VisionProcessor::detectUIElements - UI element detection is now primarily handled by the Qwen model via analyzeImageWithQwen." << std::endl;
    // The image_path parameter is intentionally unused now.
    // (void)image_path; // Optional: explicitly mark as unused if compiler warnings are aggressive.
    return std::vector<UIElement>();
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
    std::cout << "INFO: VisionProcessor::extractTextFromImage - Text extraction is now primarily handled by the Qwen model via analyzeImageWithQwen." << std::endl;
    // The image_path parameter is intentionally unused now.
    // (void)image_path; // Optional: explicitly mark as unused
    return "Text extraction is now primarily handled by the Qwen model.";
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

    // TODO: Future Enhancement: Consider clipboard operations (copy-paste) for typing very large texts for performance and reliability.    // Type the text
    for (char c : text)
    {
        if (c == '\n')
        {
            // Simulate Enter key press
            INPUT enter_input[2] = {0};
            enter_input[0].type = INPUT_KEYBOARD;
            enter_input[0].ki.wVk = VK_RETURN;
            enter_input[1].type = INPUT_KEYBOARD;
            enter_input[1].ki.wVk = VK_RETURN;
            enter_input[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, enter_input, sizeof(INPUT));
        }
        else if (c == '\r')
        {
            // Typically, LF (\n) is the one that matters. CR (\r) might be part of CRLF (\r\n) sequences.
            // If \n is already handled as Enter, CR can often be skipped or also treated as Enter if needed by a specific application.
            // For simplicity here, we'll skip CR if we are already handling LF.
            continue;
        }
        else
        {
            SHORT vk = VkKeyScanA(c);
            if (vk != -1)
            {
                INPUT input[4] = {0}; // Max 4 inputs: ShiftDown, KeyDown, KeyUp, ShiftUp
                int input_count = 0;

                // Handle Shift key for uppercase or shifted symbols
                if (HIWORD(vk) & 1)
                { // Check if Shift key is required
                    input[input_count].type = INPUT_KEYBOARD;
                    input[input_count].ki.wVk = VK_SHIFT;
                    input[input_count].ki.dwFlags = 0; // Key down
                    input_count++;
                }
                // TODO: Add handling for Ctrl and Alt if necessary, VkKeyScanA's HIWORD can indicate these too.

                // Key down
                input[input_count].type = INPUT_KEYBOARD;
                input[input_count].ki.wVk = LOBYTE(vk); // Use only the low byte for the key code
                input[input_count].ki.dwFlags = 0;      // Key down
                input_count++;

                // Key up
                input[input_count].type = INPUT_KEYBOARD;
                input[input_count].ki.wVk = LOBYTE(vk);
                input[input_count].ki.dwFlags = KEYEVENTF_KEYUP;
                input_count++;

                if (HIWORD(vk) & 1)
                { // Release Shift key if it was pressed
                    input[input_count].type = INPUT_KEYBOARD;
                    input[input_count].ki.wVk = VK_SHIFT;
                    input[input_count].ki.dwFlags = KEYEVENTF_KEYUP;
                    input_count++;
                }
                SendInput(input_count, input, sizeof(INPUT));
            }
            else
            {
                // Character cannot be typed with VkKeyScanA (e.g., some complex Unicode not in current layout)
                std::cerr << "⚠️ Warning in typeAtElement: Character '" << c << "' (ASCII: " << static_cast<int>(c)
                          << ") cannot be directly typed using VkKeyScanA. It might be skipped or require alternative input methods." << std::endl;
            }
        }
        Sleep(30); // Keep the delay, but it's now per processed character/action.
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

ScreenAnalysis VisionProcessor::analyzeImageWithQwen(const std::string& image_path) {
    ScreenAnalysis analysis;
    analysis.overall_description = "Failed to analyze image with Qwen."; // Default error message
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Get API Key from environment variable
    const char* api_key_env = std::getenv("OPENROUTER_API_KEY");
    if (!api_key_env) {
        std::cerr << "Error: OPENROUTER_API_KEY environment variable not set." << std::endl;
        analysis.overall_description = "Error: OPENROUTER_API_KEY not set.";
        return analysis;
    }
    std::string api_key = api_key_env;

    std::string base64_image = base64_encode(image_path);
    if (base64_image.empty()) {
        analysis.overall_description = "Error: Failed to encode image to base64.";
        return analysis;
    }

    // Determine image type from path (simple check)
    std::string image_type = "image/png"; // Default
    if (image_path.size() > 4) {
        std::string ext = image_path.substr(image_path.size() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); // Convert extension to lowercase
        if (ext == ".jpg" || ext == "jpeg") image_type = "image/jpeg";
        else if (ext == ".bmp") image_type = "image/bmp";
        // Add more types if necessary, e.g., gif, webp
    }

    std::string image_data_url = "data:" + image_type + ";base64," + base64_image;

    curl_global_init(CURL_GLOBAL_ALL); // Should ideally be called once per program
    curl = curl_easy_init();

    if (curl) {
        std::string url = "https://openrouter.ai/api/v1/chat/completions";

        // Construct JSON payload
        json payload = {
            {"model", "qwen/qwen2.5-vl-32b-instruct:free"}, // Corrected model name
            {"messages", json::array({
                {
                    {"role", "user"},
                    {"content", json::array({
                        {{"type", "text"}, {"text", R"(Describe this image.
In addition, identify all significant UI elements visible in the image, such as buttons, input fields, text areas, labels, and icons.
For each element, provide its type (e.g., "button", "input_field", "text", "icon"), the text it contains (if any), and its bounding box coordinates.
The bounding box should be an array of four integers: [x_min, y_min, x_max, y_max], representing the pixel coordinates of the top-left and bottom-right corners of the element.
Please provide this list of UI elements as a JSON array string within your response, formatted like this:
ELEMENTS_JSON_START
[
  {"type": "button", "text": "Login", "bbox": [100, 200, 180, 230]},
  {"type": "input_field", "text": "", "bbox": [100, 150, 300, 180]},
  {"type": "icon", "text": "settings", "bbox": [10, 10, 30, 30]}
]
ELEMENTS_JSON_END
If no specific UI elements are identifiable, provide an empty array:
ELEMENTS_JSON_START
[]
ELEMENTS_JSON_END)"}},
                        {{"type": "image_url"}, {"image_url", {{"url", image_data_url}}}}
                    })}
                }
            })},
            {"max_tokens", 1024} // Optional: limit response size
        };
        std::string json_payload_str = payload.dump();

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string auth_header = "Authorization: Bearer " + api_key;
        headers = curl_slist_append(headers, auth_header.c_str());
        // headers = curl_slist_append(headers, "HTTP-Referer: your-app-url"); // Optional: Recommended by OpenRouter
        // headers = curl_slist_append(headers, "X-Title: Your App Name"); // Optional: Recommended by OpenRouter


        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L); // 30 seconds
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Enable for debugging cURL verbosity

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            analysis.overall_description = std::string("Qwen API call failed: ") + curl_easy_strerror(res);
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            std::cout << "Qwen API HTTP Response Code: " << http_code << std::endl;
            // std::cout << "Qwen API Response: " << readBuffer << std::endl; // For debugging

            if (http_code == 200) {
                try {
                    json response_json = json::parse(readBuffer);
                    if (response_json.contains("choices") && response_json["choices"].is_array() && !response_json["choices"].empty()) {
                        const auto& first_choice = response_json["choices"][0];
                        if (first_choice.contains("message") && first_choice["message"].contains("content")) {
                            // Qwen-VL models typically return content as a string directly
                            const auto& content = first_choice["message"]["content"];
                            std::string full_response_text;

                            if (content.is_string()) {
                                full_response_text = content.get<std::string>();
                            }
                            // Handling for older/different Qwen models that might return content as an array
                            else if (content.is_array() && !content.empty() && content[0].is_object() && content[0].contains("text")) {
                                full_response_text = content[0]["text"].get<std::string>();
                            } else {
                                analysis.overall_description = "Qwen response format error: Could not extract text from content.";
                                std::cerr << "Qwen response format error: 'content' is not a direct string or an array with text." << std::endl;
                                std::cerr << "Content received: " << content.dump(2) << std::endl;
                                // Early exit or skip UI element parsing if content structure is wrong
                            }

                            if (!full_response_text.empty()) {
                                const std::string elements_json_start_marker = "ELEMENTS_JSON_START";
                                const std::string elements_json_end_marker = "ELEMENTS_JSON_END";

                                size_t json_block_start_pos = full_response_text.find(elements_json_start_marker);
                                size_t json_block_end_pos = full_response_text.find(elements_json_end_marker);

                                if (json_block_start_pos != std::string::npos && json_block_end_pos != std::string::npos && json_block_start_pos < json_block_end_pos) {
                                    analysis.overall_description = full_response_text.substr(0, json_block_start_pos);
                                    // Trim whitespace (simple trim for trailing newlines/spaces before marker)
                                    size_t last_char = analysis.overall_description.find_last_not_of(" \n\r\t");
                                    if (std::string::npos != last_char) {
                                        analysis.overall_description.erase(last_char + 1);
                                    }

                                    size_t actual_json_start = json_block_start_pos + elements_json_start_marker.length();
                                    std::string json_str_block = full_response_text.substr(actual_json_start, json_block_end_pos - actual_json_start);

                                    try {
                                        json parsed_elements_json = json::parse(json_str_block);
                                        if (parsed_elements_json.is_array()) {
                                            for (const auto& elem_item : parsed_elements_json) {
                                                UIElement ui_el;
                                                ui_el.type = elem_item.value("type", "unknown");
                                                ui_el.text = elem_item.value("text", "");
                                                // Description for individual elements could be added if model provides it
                                                // ui_el.description = elem_item.value("description", "");

                                                if (elem_item.contains("bbox") && elem_item["bbox"].is_array() && elem_item["bbox"].size() == 4) {
                                                    const auto& bbox_arr = elem_item["bbox"];
                                                    try {
                                                        int x_min = bbox_arr[0].get<int>();
                                                        int y_min = bbox_arr[1].get<int>();
                                                        int x_max = bbox_arr[2].get<int>();
                                                        int y_max = bbox_arr[3].get<int>();

                                                        ui_el.x = x_min;
                                                        ui_el.y = y_min;
                                                        ui_el.width = x_max - x_min;
                                                        ui_el.height = y_max - y_min;
                                                        ui_el.confidence = 0.9; // Default confidence for Qwen identified elements

                                                        if (ui_el.width < 0) ui_el.width = 0;
                                                        if (ui_el.height < 0) ui_el.height = 0;

                                                        analysis.elements.push_back(ui_el);
                                                    } catch (const json::type_error& te) {
                                                        std::cerr << "Error parsing bbox array element: " << te.what()
                                                                  << " for element: " << elem_item.dump(2) << std::endl;
                                                    }
                                                } else {
                                                    std::cerr << "Warning: UI element missing valid bbox: " << elem_item.dump(2) << std::endl;
                                                }
                                            }
                                        } else {
                                             std::cerr << "Error: ELEMENTS_JSON_START/END block found, but content is not a JSON array. Content: " << json_str_block << std::endl;
                                             // Fallback: use the text before the block as description.
                                             // analysis.overall_description is already set to this.
                                        }
                                    } catch (const json::parse_error& e) {
                                        std::cerr << "Error parsing UI elements JSON block: " << e.what() << ". Block content: " << json_str_block << std::endl;
                                        // Fallback: use the text before the block as description.
                                        // analysis.overall_description is already set to this part.
                                        // If we prefer the full text in this case:
                                        // analysis.overall_description = full_response_text;
                                    }
                                } else {
                                    // Markers not found, or in wrong order, treat the whole response as description
                                    analysis.overall_description = full_response_text;
                                }
                            } else if (analysis.overall_description.empty()) {
                                // This case means full_response_text was empty and overall_description wasn't set by an error message already.
                                analysis.overall_description = "Qwen response format error: Content was empty or in unexpected format.";
                                std::cerr << "Qwen response format error: Content was empty or in unexpected format after checking string/array." << std::endl;
                            }
                            // If full_response_text is empty but analysis.overall_description was set by "Could not extract text from content", it will retain that error message.

                        } else {
                            analysis.overall_description = "Qwen response format error: 'message' or 'content' field missing.";
                            std::cerr << "Qwen response format error: 'message' or 'content' field missing in choice." << std::endl;
                        }
                    } else {
                        analysis.overall_description = "Qwen response format error: 'choices' array missing or empty.";
                        std::cerr << "Qwen response format error: 'choices' array missing or empty in response." << std::endl;
                        if(response_json.contains("error")) {
                            std::cerr << "Qwen API Error: " << response_json["error"].dump(2) << std::endl;
                            if (response_json["error"].contains("message")) {
                                analysis.overall_description = "Qwen API Error: " + response_json["error"]["message"].get<std::string>();
                            } else {
                                analysis.overall_description = "Qwen API Error: Unknown error structure.";
                            }
                        }
                    }
                } catch (const json::parse_error& e) {
                    std::cerr << "JSON parsing error: " << e.what() << std::endl;
                    analysis.overall_description = "Failed to parse Qwen API response.";
                }
            } else {
                std::cerr << "Qwen API returned HTTP " << http_code << std::endl;
                std::cerr << "Response: " << readBuffer << std::endl;
                analysis.overall_description = "Qwen API Error: HTTP " + std::to_string(http_code);
                 try {
                    json error_json = json::parse(readBuffer);
                    if(error_json.contains("error") && error_json["error"].contains("message")) {
                        analysis.overall_description += ": " + error_json["error"]["message"].get<std::string>();
                    }
                } catch (const json::parse_error& e) {
                    // Ignore if parsing error message fails, keep the HTTP code message
                }
            }
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "curl_easy_init() failed." << std::endl;
        analysis.overall_description = "Failed to initialize cURL for Qwen API.";
    }
    curl_global_cleanup(); // Should ideally be called once per program
    return analysis;
}
