#ifndef MULTIMODAL_HANDLER_H
#define MULTIMODAL_HANDLER_H

#include "include/json.hpp"
#include <string>
#include <vector>

using json = nlohmann::json;

enum class InputType {
    TEXT,
    VOICE,
    IMAGE,
    FILE,
    SCREEN_CAPTURE,
    GESTURE
};

struct MultiModalInput {
    InputType type;
    std::string content;
    std::string file_path;
    json metadata;
    std::string timestamp;
};

class MultiModalHandler {
private:
    bool voice_enabled;
    bool image_analysis_enabled;
    std::string temp_directory;
    
    std::string transcribeAudio(const std::string& audio_file);
    std::string analyzeImage(const std::string& image_file);
    std::string extractTextFromFile(const std::string& file_path);
    std::string captureScreen();
    
public:
    MultiModalHandler();
    
    // Input processing methods
    MultiModalInput processTextInput(const std::string& text);
    MultiModalInput processVoiceInput(const std::string& audio_file);
    MultiModalInput processImageInput(const std::string& image_file);
    MultiModalInput processFileInput(const std::string& file_path);
    MultiModalInput processScreenCapture();
    
    // Voice capabilities
    bool enableVoiceInput();
    bool disableVoiceInput();
    std::string startVoiceRecording();
    std::string stopVoiceRecording();
    
    // Image capabilities
    bool enableImageAnalysis();
    std::string analyzeScreenshot();
    std::string findUIElement(const std::string& element_description);
    std::vector<std::string> extractTextFromScreen();
    
    // File handling
    std::string analyzeDocument(const std::string& file_path);
    json extractMetadata(const std::string& file_path);
    
    // Integration methods
    std::string convertToUnifiedFormat(const MultiModalInput& input);
    json combineInputs(const std::vector<MultiModalInput>& inputs);
    
    // Configuration
    void setTempDirectory(const std::string& path);
    void configureVoiceSettings(const json& settings);
    void configureImageSettings(const json& settings);
};

#endif // MULTIMODAL_HANDLER_H
