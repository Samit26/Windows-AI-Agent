#ifndef VISION_PROCESSOR_H
#define VISION_PROCESSOR_H

#include "include/json.hpp"
#include <string>
#include <vector>
#include <memory>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_ // Prevent inclusion of winsock.h
#endif
#include <windows.h>

using json = nlohmann::json;

struct UIElement
{
    int x, y, width, height;
    std::string type;        // button, text_field, image, label, etc.
    std::string text;        // OCR extracted text
    std::string description; // AI generated description
    double confidence;       // Detection confidence
    std::string id;          // Unique identifier
};

struct ScreenAnalysis
{
    std::string screenshot_path;
    std::vector<UIElement> elements;
    std::string overall_description;
    std::string window_title;
    std::string application_name;
    json metadata;
};

class VisionProcessor
{
private:
    std::string temp_directory;
    bool opencv_available;

    // Screen capture methods
    std::string captureScreenshot();
    std::string captureWindowScreenshot(HWND window); // Image processing methods
    std::vector<UIElement> detectUIElements(const std::string &image_path);
    std::string extractTextFromImage(const std::string &image_path);
    std::vector<UIElement> findButtons(const std::string &image_path);
    std::vector<UIElement> findTextFields(const std::string &image_path);
    void addCommonWindowsElements(std::vector<UIElement> &elements);

    // Template matching
    std::vector<UIElement> findTemplateMatches(const std::string &image_path,
                                               const std::string &template_path,
                                               double threshold = 0.8);

    // Windows API helpers
    HWND getActiveWindow();
    std::string getWindowTitle(HWND window);
    std::string getApplicationName(HWND window);

public:
    VisionProcessor();
    ~VisionProcessor();

    // Main analysis methods
    ScreenAnalysis analyzeCurrentScreen();
    ScreenAnalysis analyzeWindow(HWND window);
    ScreenAnalysis analyzeScreenshot(const std::string &image_path);

    // Element finding methods
    UIElement findElementByText(const std::string &text, const ScreenAnalysis &analysis);
    UIElement findElementByType(const std::string &type, const ScreenAnalysis &analysis);
    std::vector<UIElement> findElementsContaining(const std::string &text, const ScreenAnalysis &analysis);

    // Coordinate and interaction helpers
    bool clickElement(const UIElement &element);
    bool typeAtElement(const UIElement &element, const std::string &text);
    bool scrollToElement(const UIElement &element);

    // Screenshot utilities
    std::string saveScreenshot(const std::string &filename = "");
    bool compareScreenshots(const std::string &before, const std::string &after, double threshold = 0.95);

    // Description generation for Gemini
    std::string generateScreenDescription(const ScreenAnalysis &analysis);
    json createElementsJson(const std::vector<UIElement> &elements);

    // Configuration
    void setTempDirectory(const std::string &path);
    void enableOpenCV(bool enable);
    bool isOpenCVAvailable() const;
};

#endif // VISION_PROCESSOR_H
