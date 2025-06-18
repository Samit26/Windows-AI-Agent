#ifndef VISION_GUIDED_EXECUTOR_H
#define VISION_GUIDED_EXECUTOR_H

#include "vision_processor.h"
#include "ai_model.h"
#include "include/json.hpp"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <map>

using json = nlohmann::json;

enum class VisionActionType
{
    CLICK,
    TYPE,
    SCROLL,
    WAIT,
    SCREENSHOT,
    VERIFY,
    NAVIGATE,
    COMPLETE,
    LAUNCH_APP,
    FIND_ELEMENT
};

// Forward declaration
struct VisionAction;

// Dynamic intent structure - no hardcoded apps or actions
struct UserIntent
{
    std::string raw_command;
    std::string action_type;                       // launch, type, click, navigate, etc.
    std::string target_app;                        // dynamically determined
    std::string target_element;                    // dynamically determined
    std::string content;                           // text to type, search query, etc.
    std::map<std::string, std::string> parameters; // flexible parameters
    double confidence;
};

// VisionAction struct definition (moved up)
struct VisionAction
{
    VisionActionType type;
    std::string target_description;
    std::string value;       // Text to type, etc.
    std::string explanation; // Why this action
    double confidence;
    int wait_time = 1000;    // Milliseconds to wait for WAIT actions
    json metadata;
};

// Dynamic execution plan - completely flexible
struct ExecutionPlan
{
    std::string task_id;
    UserIntent intent;
    std::vector<VisionAction> steps;
    std::string current_context; // what's currently on screen
    bool is_complete;
};

struct VisionTaskStep
{
    std::string description;
    VisionAction action;
    ScreenAnalysis before_state;
    ScreenAnalysis after_state;
    bool success;
    std::string error_message;
    double execution_time;
};

struct VisionTaskExecution
{
    std::string original_task;
    std::vector<VisionTaskStep> steps;
    bool overall_success;
    std::string final_result;
    double total_time;
    json metadata;
};

// Structures for intelligent task analysis
struct TaskComponents
{
    bool needsAppLaunch = false;
    bool needsTyping = false;
    bool needsNavigation = false;
    bool needsInteraction = false;
    std::string targetApp;
    std::string appName;
    std::string textToType;
    std::string navigationTarget;
    std::string interactionTarget;
};

struct TaskProgress
{
    bool appLaunched = false;
    bool textTyped = false;
    bool navigationComplete = false;
    bool interactionComplete = false;
    bool isComplete = false;
};

class VisionGuidedExecutor
{
private:
    std::unique_ptr<VisionProcessor> vision_processor;
    std::string ai_api_key;
    std::string temp_directory;
    int max_steps;
    int verification_attempts;

    // Dynamic AI-powered intent analysis
    UserIntent analyzeUserIntent(const std::string &command);
    ExecutionPlan createDynamicPlan(const UserIntent &intent, const ScreenAnalysis &screen);

    // Dynamic screen analysis using OpenCV
    std::vector<UIElement> findDynamicElements(const std::string &search_criteria, const ScreenAnalysis &screen);
    UIElement findBestMatch(const std::string &description, const ScreenAnalysis &screen);
    bool isElementVisible(const std::string &element_description, const ScreenAnalysis &screen);

    // Dynamic action execution
    bool executeDynamicAction(const VisionAction &action, const ScreenAnalysis &screen);
    bool launchApplicationDynamically(const std::string &app_description);
    bool findAndClickDynamically(const std::string &element_description, const ScreenAnalysis &screen);
    bool findAndTypeDynamically(const std::string &text, const ScreenAnalysis &screen);

    // AI-powered helpers
    std::string queryAIForIntent(const std::string &command);
    std::string queryAIForElementLocation(const std::string &element_description, const ScreenAnalysis &screen);
    std::string queryAIForNextAction(const ExecutionPlan &plan, const ScreenAnalysis &screen);

    // OpenCV-powered screen analysis
    cv::Mat captureScreen();
    std::vector<cv::Rect> detectTextRegions(const cv::Mat &screen);
    std::vector<cv::Rect> detectButtons(const cv::Mat &screen);
    std::vector<cv::Rect> detectIcons(const cv::Mat &screen);
    std::string extractTextFromRegion(const cv::Mat &screen, const cv::Rect &region);

    // Dynamic application detection
    std::string detectCurrentApplication();
    std::vector<std::string> getAvailableApplications();
    std::string mapIntentToApplication(const std::string &intent);
    
    // Utility functions
    double calculateElementSimilarity(const UIElement &element, const std::string &description);
    bool verifyActionSuccess(const VisionAction &action, const ScreenAnalysis &before, const ScreenAnalysis &after);
    std::string normalizeText(const std::string &text);

public:
    VisionGuidedExecutor(const std::string &api_key);
    ~VisionGuidedExecutor();

    // Main dynamic execution interface - completely flexible
    VisionTaskExecution executeDynamicTask(const std::string &user_command);

    // Single-line execution - the goal!
    bool executeCommand(const std::string &command);

    // Real-time dynamic analysis
    UserIntent parseIntent(const std::string &command);
    ExecutionPlan planExecution(const UserIntent &intent);
    bool executeStep(const VisionAction &step);

    // Configuration
    void setMaxSteps(int max);
    void setTempDirectory(const std::string &path);
    void setAIApiKey(const std::string &key);

    // Access methods
    ScreenAnalysis getCurrentScreenState();
    VisionProcessor *getVisionProcessor();    // Core execution methods
    VisionAction planNextAction(const std::string &task,
                                const ScreenAnalysis &current_state,
                                const std::vector<VisionTaskStep> &previous_steps);

    // Helper methods for AI response parsing
    json parseActionFromResponse(const std::string& response_text);
    VisionAction createActionFromJson(const json& actionJson);

    bool executeAction(const VisionAction &action, VisionTaskStep &step);

    // Gemini integration
    std::string createVisionPrompt(const std::string &task,
                                   const std::string &screen_description,
                                   const std::vector<VisionTaskStep> &previous_steps);
    VisionAction parseGeminiResponse(const json &response, const std::string &task, const ScreenAnalysis &current_state);

    // Intelligent task analysis methods
    VisionAction analyzeTaskIntelligently(const std::string &task,
                                          const ScreenAnalysis &current_state,
                                          const std::vector<VisionTaskStep> &previous_steps);
    TaskComponents parseTaskComponents(const std::string &task);
    TaskProgress analyzeTaskProgress(const TaskComponents &components,
                                     const std::vector<VisionTaskStep> &previous_steps,
                                     const ScreenAnalysis &current_state);
    VisionAction planAppLaunchAction(const std::string &appExecutable,
                                     const ScreenAnalysis &current_state);
    VisionAction planTypingAction(const std::string &text,
                                  const ScreenAnalysis &current_state);
    VisionAction planNavigationAction(const std::string &target,
                                      const ScreenAnalysis &current_state);
    VisionAction planInteractionAction(const std::string &target,
                                       const ScreenAnalysis &current_state);
    VisionAction planWaitAction(const std::string &reason, int milliseconds);
    UIElement findBestTextInputElement(const ScreenAnalysis &state);

    // Helper functions for dynamic task analysis
    TaskComponents detectApplicationIntent(const std::string &lower_task);
    std::string extractSearchQuery(const std::string &task, const std::vector<std::string> &removeWords);
    std::string extractContactName(const std::string &task);
    std::string extractMessageText(const std::string &task);
    bool isWebTask(const std::string &task);
    bool isMessagingTask(const std::string &task);
    bool isFileTask(const std::string &task);
    bool isSystemTask(const std::string &task);
    std::string buildWebUrl(const std::string &task);
    std::pair<std::string, std::string> getMessagingInfo(const std::string &task);
    std::pair<std::string, std::string> getSystemAppInfo(const std::string &task);
    std::string urlEncode(const std::string &value);

    // Action execution helpers
    bool executeClick(const std::string &target, const ScreenAnalysis &state);
    bool executeType(const std::string &target, const std::string &text, const ScreenAnalysis &state);
    bool executeScroll(const std::string &direction, const ScreenAnalysis &state);
    bool executeWait(int milliseconds);

    // Verification helpers
    bool isTaskComplete(const std::string &task, const ScreenAnalysis &state);
    bool hasScreenChanged(const ScreenAnalysis &before, const ScreenAnalysis &after);

    // Error handling
    bool handleExecutionError(const std::string &error, VisionTaskStep &step);
    bool attemptRecovery(const VisionTaskStep &failed_step);

    // Main execution interface
    VisionTaskExecution executeVisionTask(const std::string &task);
    VisionTaskExecution executeVisionTaskWithContext(const std::string &task,
                                                     const json &context);

    // Step-by-step execution
    VisionTaskStep executeNextStep(const std::string &task,
                                   const std::vector<VisionTaskStep> &previous_steps);

    // Additional configuration
    void setVerificationAttempts(int attempts);

    // Utility methods
    bool validateTask(const std::string &task);
    std::vector<std::string> suggestTaskBreakdown(const std::string &task);

    // Debug and monitoring
    void enableDebugMode(bool enable);
    void saveExecutionLog(const VisionTaskExecution &execution, const std::string &filename);
    json getExecutionMetrics(const VisionTaskExecution &execution);
};

#endif // VISION_GUIDED_EXECUTOR_H
