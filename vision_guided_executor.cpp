#include "vision_guided_executor.h"
#include "ai_model.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <algorithm>
#include <filesystem>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <windows.h>

VisionGuidedExecutor::VisionGuidedExecutor(const std::string &api_key)
    : ai_api_key(api_key), temp_directory("temp/vision_tasks"),
      max_steps(20), verification_attempts(3)
{

    vision_processor = std::make_unique<VisionProcessor>();
    vision_processor->setTempDirectory(temp_directory);
    std::filesystem::create_directories(temp_directory);

    std::cout << "🤖 Vision-Guided Executor initialized" << std::endl;
}

VisionGuidedExecutor::~VisionGuidedExecutor()
{
    // Cleanup handled by smart pointers
}

VisionTaskExecution VisionGuidedExecutor::executeVisionTask(const std::string &task)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    VisionTaskExecution execution;
    execution.original_task = task;
    execution.overall_success = false;

    std::cout << "🎯 Starting vision task: " << task << std::endl;

    try
    {
        // Initial screen analysis
        ScreenAnalysis initial_state = vision_processor->analyzeCurrentScreen();
        std::cout << "📸 Initial screen captured: " << initial_state.application_name << std::endl;

        // Execute steps iteratively
        for (int step_count = 0; step_count < max_steps; ++step_count)
        {
            std::cout << "📋 Planning step " << (step_count + 1) << "..." << std::endl;

            // Get current screen state
            ScreenAnalysis current_state = vision_processor->analyzeCurrentScreen();

            // Plan next action
            VisionAction action = planNextAction(task, current_state, execution.steps);

            if (action.type == VisionActionType::COMPLETE)
            {
                std::cout << "✅ Task completed!" << std::endl;
                execution.overall_success = true;
                break;
            }

            // Execute the planned action
            VisionTaskStep step;
            step.description = action.explanation;
            step.action = action;
            step.before_state = current_state;

            std::cout << "⚡ Executing: " << action.explanation << std::endl;

            auto step_start = std::chrono::high_resolution_clock::now();
            step.success = executeAction(action, step);
            auto step_end = std::chrono::high_resolution_clock::now();

            step.execution_time = std::chrono::duration<double>(step_end - step_start).count();

            // Capture after state
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for UI to update
            step.after_state = vision_processor->analyzeCurrentScreen();

            // Verify action success
            if (step.success)
            {
                step.success = verifyActionSuccess(action, step.before_state, step.after_state);
            }

            execution.steps.push_back(step);

            if (!step.success)
            {
                std::cout << "❌ Step failed: " << step.error_message << std::endl;

                // Attempt recovery
                if (!attemptRecovery(step))
                {
                    std::cout << "💥 Recovery failed, aborting task" << std::endl;
                    break;
                }
            }
            else
            {
                std::cout << "✅ Step completed successfully" << std::endl;
            }

            // Check if task is complete
            if (isTaskComplete(task, step.after_state))
            {
                std::cout << "🎉 Task verification passed!" << std::endl;
                execution.overall_success = true;
                break;
            }
        }

        if (execution.steps.size() >= max_steps)
        {
            std::cout << "⚠️  Maximum steps reached, task may be incomplete" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "💥 Exception during task execution: " << e.what() << std::endl;
        execution.final_result = "Exception: " + std::string(e.what());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    execution.total_time = std::chrono::duration<double>(end_time - start_time).count();

    // Generate final result
    if (execution.overall_success)
    {
        execution.final_result = "Task completed successfully in " +
                                 std::to_string(execution.steps.size()) + " steps";
    }
    else
    {
        execution.final_result = "Task failed after " +
                                 std::to_string(execution.steps.size()) + " steps";
    }

    std::cout << "📊 Task execution completed in " << execution.total_time << " seconds" << std::endl;    return execution;
}

// Helper function to parse JSON action from AI response
json VisionGuidedExecutor::parseActionFromResponse(const std::string& response_text)
{
    try
    {
        // Method 1: Look for JSON in markdown blocks
        size_t json_block_start = response_text.find("```json");
        if (json_block_start != std::string::npos)
        {
            json_block_start += 7;
            size_t json_block_end = response_text.find("```", json_block_start);
            if (json_block_end != std::string::npos)
            {
                std::string json_str = response_text.substr(json_block_start, json_block_end - json_block_start);
                json_str.erase(0, json_str.find_first_not_of(" \t\n\r"));
                json_str.erase(json_str.find_last_not_of(" \t\n\r") + 1);
                return json::parse(json_str);
            }
        }

        // Method 2: Look for final JSON object
        size_t last_brace = response_text.rfind("}");
        if (last_brace != std::string::npos)
        {
            size_t json_start = response_text.find_last_of("{", last_brace);
            if (json_start != std::string::npos)
            {
                std::string json_str = response_text.substr(json_start, last_brace - json_start + 1);
                json_str.erase(0, json_str.find_first_not_of(" \t\n\r"));
                json_str.erase(json_str.find_last_not_of(" \t\n\r") + 1);
                return json::parse(json_str);
            }
        }
    }
    catch (const json::parse_error& e)
    {
        std::cerr << "❌ JSON parsing error: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ General error during JSON parsing: " << e.what() << std::endl;
    }

    return json::object();
}

// Helper function to create VisionAction from JSON
VisionAction VisionGuidedExecutor::createActionFromJson(const json& actionJson)
{
    VisionAction action;
    
    std::string action_type = actionJson.value("action_type", "wait");
    if (action_type == "click")
        action.type = VisionActionType::CLICK;
    else if (action_type == "type")
        action.type = VisionActionType::TYPE;
    else if (action_type == "scroll")
        action.type = VisionActionType::SCROLL;
    else if (action_type == "wait")
        action.type = VisionActionType::WAIT;
    else if (action_type == "complete")
        action.type = VisionActionType::COMPLETE;
    else
        action.type = VisionActionType::WAIT;
    
    action.target_description = actionJson.value("target_description", "");
    action.value = actionJson.value("value", "");
    action.explanation = actionJson.value("explanation", "AI-generated action");
    action.confidence = actionJson.value("confidence", 0.5);
    
    if (action.type == VisionActionType::WAIT)
    {
        action.wait_time = std::stoi(action.value.empty() ? "1000" : action.value);
    }
    
    return action;
}

VisionAction VisionGuidedExecutor::planNextAction(const std::string &task,
                                                  const ScreenAnalysis &current_state,
                                                  const std::vector<VisionTaskStep> &previous_steps)
{
    VisionAction action;
    
    try
    {
        // Build context for DeepSeek R1
        std::string context = "TASK: " + task + "\n\n";
        context += "CURRENT SCREEN STATE:\n";
        context += "Application: " + current_state.application_name + "\n";
        context += "Window Title: " + current_state.window_title + "\n";
        context += "Description: " + current_state.overall_description + "\n\n";
        
        // Add available UI elements
        context += "AVAILABLE UI ELEMENTS:\n";
        for (size_t i = 0; i < current_state.elements.size() && i < 15; i++)
        {
            const auto& element = current_state.elements[i];
            context += std::to_string(i + 1) + ". " + element.description;
            if (!element.text.empty())
            {
                context += " (text: \"" + element.text + "\")";
            }
            context += " [" + element.type + "]\n";
        }
        
        // Add previous steps context
        if (!previous_steps.empty())
        {
            context += "\nPREVIOUS STEPS:\n";
            for (size_t i = 0; i < previous_steps.size(); i++)
            {                context += std::to_string(i + 1) + ". " + previous_steps[i].description;
                context += " - " + std::string(previous_steps[i].success ? "SUCCESS" : "FAILED") + "\n";
            }
        }
        
        context += "\nDetermine the next action to accomplish the task. Focus on the main content area of the application.";
        
        // Call DeepSeek R1 for vision guidance
        json response = callVisionAIModel(ai_api_key, context);
        
        if (response.contains("choices") && !response["choices"].empty())
        {
            auto& choice = response["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content"))
            {
                std::string content = choice["message"]["content"];
                
                // Parse the JSON action from the response
                json actionJson = parseActionFromResponse(content);
                
                if (!actionJson.empty())
                {
                    action = createActionFromJson(actionJson);
                }
                else
                {
                    // Fallback action
                    action.type = VisionActionType::WAIT;
                    action.explanation = "Could not parse AI response, waiting";
                    action.confidence = 0.1;
                    action.wait_time = 1000;
                }
            }
        }
        else
        {
            // Fallback action
            action.type = VisionActionType::WAIT;
            action.explanation = "No valid AI response, waiting";
            action.confidence = 0.1;
            action.wait_time = 1000;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "❌ Error planning action: " << e.what() << std::endl;
        action.type = VisionActionType::WAIT;
        action.explanation = "Error in planning: " + std::string(e.what());
        action.confidence = 0.1;
        action.wait_time = 1000;
    }
    
    return action;
}

// Intelligent task analyzer that combines NLP parsing with vision guidance
VisionAction VisionGuidedExecutor::analyzeTaskIntelligently(const std::string &task,
                                                            const ScreenAnalysis &current_state,
                                                            const std::vector<VisionTaskStep> &previous_steps)
{
    VisionAction action;
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    std::cout << "🧠 Intelligently analyzing task: " << task << std::endl;

    // Parse task components using NLP-like analysis
    TaskComponents components = parseTaskComponents(task);

    // Analyze current progress
    TaskProgress progress = analyzeTaskProgress(components, previous_steps, current_state); // Determine next intelligent action
    if (components.needsAppLaunch && !progress.appLaunched)
    {
        return planAppLaunchAction(components.targetApp, current_state);
    }
    else if (components.needsAppLaunch && progress.appLaunched && components.needsTyping && !progress.textTyped)
    {
        // App was launched, check if it's actually ready for typing
        std::string currentApp = current_state.application_name;
        std::transform(currentApp.begin(), currentApp.end(), currentApp.begin(), ::tolower);

        bool app_is_ready = false;
        if (components.appName == "notepad" &&
            (currentApp.find("notepad") != std::string::npos || currentApp.find("untitled") != std::string::npos))
        {
            // For Notepad, also check if there are proper text input elements detected
            UIElement textArea = findBestTextInputElement(current_state);
            if (textArea.confidence > 0.3 &&
                textArea.type != "container" &&
                textArea.text != "Taskbar" &&
                textArea.text != "Start Button")
            {
                app_is_ready = true;
                std::cout << "✅ Notepad is ready with text area: " << textArea.text << " (type: " << textArea.type << ")" << std::endl;
            }
            else
            {
                std::cout << "⏳ Notepad active but no suitable text area found yet. Best element: " << textArea.text << " (confidence: " << textArea.confidence << ")" << std::endl;
            }
        }
        else if (currentApp.find(components.appName) != std::string::npos)
        {
            app_is_ready = true;
        }

        if (app_is_ready)
        {
            // App is active and ready, proceed with typing
            return planTypingAction(components.textToType, current_state);
        }
        else
        {
            // App launched but not ready yet, wait longer
            return planWaitAction("Waiting for " + components.appName + " to be ready for text input", 3000);
        }
    }
    else if (components.needsTyping && progress.appLaunched && !progress.textTyped)
    {
        return planTypingAction(components.textToType, current_state);
    }
    else if (components.needsNavigation)
    {
        return planNavigationAction(components.navigationTarget, current_state);
    }
    else if (components.needsInteraction)
    {
        return planInteractionAction(components.interactionTarget, current_state);
    }
    else if (progress.isComplete)
    {
        action.type = VisionActionType::COMPLETE;
        action.explanation = "Task completed successfully";
        action.confidence = 1.0;
        return action;
    }
    else
    {
        return planWaitAction("Analyzing task requirements", 1000);
    }
}

// Parse task into actionable components using NLP techniques
TaskComponents VisionGuidedExecutor::parseTaskComponents(const std::string &task)
{
    TaskComponents components;

    // Use AI to dynamically analyze the task instead of hardcoded parsing
    try
    {
        json intent = callIntentAI(ai_api_key, task);

        if (!intent.empty())
        {
            // Map AI analysis to TaskComponents structure
            components.needsAppLaunch = intent.value("requires_app_launch", false);
            components.targetApp = intent.value("target_application", "");
            components.appName = intent.value("app_name", "");
            components.needsTyping = intent.value("requires_typing", false);
            components.textToType = intent.value("text_to_type", "");
            components.needsInteraction = intent.value("requires_interaction", false);
            components.interactionTarget = intent.value("interaction_target", "");
            components.needsNavigation = intent.value("requires_navigation", false);
            components.navigationTarget = intent.value("navigation_target", "");

            std::cout << "🤖 AI Task Analysis:" << std::endl;
            std::cout << "   App Launch: " << (components.needsAppLaunch ? "Yes" : "No") << std::endl;
            if (components.needsAppLaunch)
            {
                std::cout << "   Target App: " << components.targetApp << " (" << components.appName << ")" << std::endl;
            }
            std::cout << "   Typing: " << (components.needsTyping ? "Yes" : "No") << std::endl;
            if (components.needsTyping)
            {
                std::cout << "   Text: \"" << components.textToType << "\"" << std::endl;
            }
            std::cout << "   Confidence: " << (intent.value("confidence", 0.0) * 100) << "%" << std::endl;

            return components;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "⚠️ AI task parsing failed, falling back to rule-based parsing: " << e.what() << std::endl;
    }

    // Fallback to simplified rule-based parsing if AI fails
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    // Smart application detection based on task intent
    components = detectApplicationIntent(lower_task);

    // If no specific app detected, try to infer from context
    if (!components.needsAppLaunch)
    {
        // Look for web-related keywords that suggest browser usage
        std::vector<std::string> webKeywords = {
            "youtube", "google", "search", "website", "browse", "online",
            "facebook", "twitter", "instagram", "gmail", "email", "news",
            "weather", "maps", "shopping", "netflix", "spotify", "twitch"};

        for (const auto &keyword : webKeywords)
        {
            if (lower_task.find(keyword) != std::string::npos)
            {
                components.needsAppLaunch = true;
                components.targetApp = "msedge.exe"; // Default browser
                components.appName = "browser";
                components.needsNavigation = true;

                // Extract the web query/destination
                if (keyword == "youtube" && lower_task.find("watch") != std::string::npos)
                {
                    components.navigationTarget = "https://youtube.com/results?search_query=" + extractSearchQuery(task, {"watch", "on", "youtube"});
                }
                else if (keyword == "weather")
                {
                    components.navigationTarget = "https://google.com/search?q=weather";
                }
                else if (keyword == "news")
                {
                    components.navigationTarget = "https://google.com/search?q=today+news";
                }
                else
                {
                    components.navigationTarget = "https://google.com/search?q=" + extractSearchQuery(task, {keyword});
                }
                break;
            }
        }

        // Look for messaging apps
        std::vector<std::string> messagingKeywords = {"whatsapp", "telegram", "discord", "slack", "teams", "skype"};
        for (const auto &keyword : messagingKeywords)
        {
            if (lower_task.find(keyword) != std::string::npos)
            {
                components.needsAppLaunch = true;
                if (keyword == "whatsapp")
                {
                    components.targetApp = "WhatsApp.exe"; // or try web version
                    components.appName = "whatsapp";
                }
                else if (keyword == "discord")
                {
                    components.targetApp = "Discord.exe";
                    components.appName = "discord";
                }
                // Add more messaging apps as needed

                components.needsInteraction = true;
                components.interactionTarget = extractContactName(task);
                break;
            }
        }
    } // Detect text input requirements
    std::regex quote_regex("\"([^\"]+)\"|'([^']+)'");
    std::smatch match;
    if (std::regex_search(task, match, quote_regex))
    {
        components.needsTyping = true;
        for (size_t i = 1; i <= 2; ++i)
        {
            if (match[i].matched)
            {
                components.textToType = match[i].str();
                break;
            }
        }
    }

    // Detect typing action words
    std::vector<std::string> typingWords = {"type", "write", "enter", "input", "fill"};
    for (const auto &word : typingWords)
    {
        if (lower_task.find(word) != std::string::npos)
        {
            components.needsTyping = true;
            if (components.textToType.empty())
            {
                // Try to extract text after typing word
                size_t pos = lower_task.find(word);
                if (pos != std::string::npos)
                {
                    pos += word.length();
                    while (pos < lower_task.length() && std::isspace(lower_task[pos]))
                        pos++;
                    if (pos < lower_task.length())
                    {
                        size_t end = pos;
                        while (end < lower_task.length() && !std::isspace(lower_task[end]))
                            end++;
                        if (end > pos)
                        {
                            components.textToType = task.substr(pos, end - pos);
                        }
                    }
                }
            }
            break;
        }
    }

    // Detect navigation requirements
    std::vector<std::string> navWords = {"navigate", "go to", "browse", "visit", "open website", "search"};
    for (const auto &word : navWords)
    {
        if (lower_task.find(word) != std::string::npos)
        {
            components.needsNavigation = true;
            // Extract navigation target
            size_t pos = lower_task.find(word);
            if (pos != std::string::npos)
            {
                pos += word.length();
                while (pos < lower_task.length() && std::isspace(lower_task[pos]))
                    pos++;
                if (pos < lower_task.length())
                {
                    components.navigationTarget = task.substr(pos);
                }
            }
            break;
        }
    }

    // Detect interaction requirements
    std::vector<std::string> interactionWords = {"click", "press", "select", "choose", "tap"};
    for (const auto &word : interactionWords)
    {
        if (lower_task.find(word) != std::string::npos)
        {
            components.needsInteraction = true;
            // Extract interaction target
            size_t pos = lower_task.find(word);
            if (pos != std::string::npos)
            {
                pos += word.length();
                while (pos < lower_task.length() && std::isspace(lower_task[pos]))
                    pos++;
                if (pos < lower_task.length())
                {
                    components.interactionTarget = task.substr(pos);
                }
            }
            break;
        }
    }

    return components;
}

// Analyze current task progress
TaskProgress VisionGuidedExecutor::analyzeTaskProgress(const TaskComponents &components,
                                                       const std::vector<VisionTaskStep> &previous_steps,
                                                       const ScreenAnalysis &current_state)
{
    TaskProgress progress; // Check if app was launched
    if (components.needsAppLaunch)
    {
        for (const auto &step : previous_steps)
        {
            if (step.action.type == VisionActionType::CLICK &&
                step.action.explanation.find("PowerShell") != std::string::npos &&
                step.success)
            {
                progress.appLaunched = true;
                break;
            }
        } // Also check current screen state - be more strict about app readiness
        if (!progress.appLaunched && !components.appName.empty())
        {
            std::string currentApp = current_state.application_name;
            std::transform(currentApp.begin(), currentApp.end(), currentApp.begin(), ::tolower);

            // Be VERY strict - only consider app launched if it's the active foreground window
            bool is_target_app_active = false;
            if (components.appName == "notepad" &&
                (currentApp.find("notepad") != std::string::npos || currentApp.find("untitled") != std::string::npos))
            {
                is_target_app_active = true;
            }
            else if (currentApp.find(components.appName) != std::string::npos)
            {
                is_target_app_active = true;
            }

            if (is_target_app_active)
            {
                progress.appLaunched = true;
                std::cout << "✅ Detected target app is now active and ready: " << currentApp << std::endl;
            }
            else
            {
                std::cout << "⏳ Target app not yet active/ready. Current: '" << currentApp << "', Target: '" << components.appName << "'" << std::endl;
            }
        }
    }
    else
    {
        progress.appLaunched = true; // No app launch needed
    }

    // Check if text was typed
    if (components.needsTyping)
    {
        for (const auto &step : previous_steps)
        {
            if (step.action.type == VisionActionType::TYPE && step.success)
            {
                progress.textTyped = true;
                break;
            }
        }
    }
    else
    {
        progress.textTyped = true; // No typing needed
    }

    // Check if navigation occurred
    if (components.needsNavigation)
    {
        for (const auto &step : previous_steps)
        {
            if ((step.action.type == VisionActionType::CLICK ||
                 step.action.type == VisionActionType::TYPE) &&
                step.success)
            {
                progress.navigationComplete = true;
                break;
            }
        }
    }
    else
    {
        progress.navigationComplete = true; // No navigation needed
    }

    // Check if interaction occurred
    if (components.needsInteraction)
    {
        for (const auto &step : previous_steps)
        {
            if (step.action.type == VisionActionType::CLICK && step.success)
            {
                progress.interactionComplete = true;
                break;
            }
        }
    }
    else
    {
        progress.interactionComplete = true; // No interaction needed
    }

    // Determine if task is complete
    progress.isComplete = progress.appLaunched && progress.textTyped &&
                          progress.navigationComplete && progress.interactionComplete;

    return progress;
}

// Plan application launch using PowerShell
VisionAction VisionGuidedExecutor::planAppLaunchAction(const std::string &appExecutable,
                                                       const ScreenAnalysis &current_state)
{
    VisionAction action;
    action.type = VisionActionType::CLICK;
    action.target_description = "powershell_launch";
    action.value = appExecutable;
    action.explanation = "Launching " + appExecutable + " using PowerShell command";
    action.confidence = 0.9;

    std::cout << "📋 Plan: Launch " << appExecutable << " via PowerShell" << std::endl;
    return action;
}

// Plan typing action using vision guidance
VisionAction VisionGuidedExecutor::planTypingAction(const std::string &text,
                                                    const ScreenAnalysis &current_state)
{
    VisionAction action;

    // Use vision to find best text input area
    UIElement textElement = findBestTextInputElement(current_state);

    std::cout << "📊 Planning typing action - best element found:" << std::endl;
    std::cout << "   - Text: '" << textElement.text << "'" << std::endl;
    std::cout << "   - Type: '" << textElement.type << "'" << std::endl;
    std::cout << "   - Confidence: " << textElement.confidence << std::endl;

    // Be more strict about text element selection - avoid containers and system elements
    bool is_valid_text_element = (textElement.confidence > 0.3 &&
                                  textElement.type != "container" &&
                                  textElement.text != "Taskbar" &&
                                  textElement.text != "Start Button" &&
                                  textElement.text.find("Search") == std::string::npos);

    if (is_valid_text_element)
    {
        action.type = VisionActionType::TYPE;
        action.target_description = textElement.text.empty() ? textElement.type : textElement.text;
        action.value = text;
        action.explanation = "Typing '" + text + "' using vision-guided input detection";
        action.confidence = textElement.confidence;
    }
    else
    {
        // Fallback: try to click in the center and type (for apps like Notepad)
        action.type = VisionActionType::TYPE;
        action.target_description = "main_text_area";
        action.value = text;
        action.explanation = "Typing '" + text + "' in main application area (fallback)";
        action.confidence = 0.7;        std::cout << "⚠️ No suitable text element found, using fallback approach" << std::endl;
    }

std::cout << "📋 Plan: Type '" << text << "' using vision guidance" << std::endl;
return action;
}

// Plan navigation action
VisionAction VisionGuidedExecutor::planNavigationAction(const std::string &target,
                                                        const ScreenAnalysis &current_state)
{
    VisionAction action;
    action.type = VisionActionType::CLICK;
    action.target_description = target;
    action.value = "";
    action.explanation = "Navigating to " + target + " using vision guidance";
    action.confidence = 0.8;

    std::cout << "📋 Plan: Navigate to " << target << std::endl;
    return action;
}

// Plan interaction action
VisionAction VisionGuidedExecutor::planInteractionAction(const std::string &target,
                                                         const ScreenAnalysis &current_state)
{
    VisionAction action;
    action.type = VisionActionType::CLICK;
    action.target_description = target;
    action.value = "";
    action.explanation = "Interacting with " + target + " using vision guidance";
    action.confidence = 0.8;

    std::cout << "📋 Plan: Interact with " << target << std::endl;
    return action;
}

// Plan wait action
VisionAction VisionGuidedExecutor::planWaitAction(const std::string &reason, int milliseconds)
{
    VisionAction action;
    action.type = VisionActionType::WAIT;
    action.target_description = "system";
    action.value = std::to_string(milliseconds);
    action.explanation = reason;
    action.confidence = 0.6;

    std::cout << "📋 Plan: Wait - " << reason << std::endl;
    return action;
}

// Find best text input element using vision
UIElement VisionGuidedExecutor::findBestTextInputElement(const ScreenAnalysis &state)
{
    std::cout << "🤖 Using AI to find best text input element from " << state.elements.size() << " available elements..." << std::endl;

    // Use AI to intelligently select the best text input element
    try
    {
        // Build list of available elements for AI analysis
        std::vector<std::string> element_descriptions;
        for (size_t i = 0; i < state.elements.size() && i < 20; ++i) // Limit to first 20 elements
        {
            const auto &element = state.elements[i];
            std::string desc = "Element " + std::to_string(i + 1) + ": ";
            desc += "'" + element.text + "' ";
            desc += "(type: " + element.type + ", ";
            desc += "size: " + std::to_string(element.width) + "x" + std::to_string(element.height) + ", ";
            desc += "confidence: " + std::to_string(element.confidence) + ")";
            element_descriptions.push_back(desc);
        }

        // Create screen description
        std::string screen_desc = "Application: " + state.application_name + "\n";
        screen_desc += "Window Title: " + state.window_title + "\n";
        screen_desc += "Elements found: " + std::to_string(state.elements.size());

        // Ask AI to select the best element for text input
        json ai_response = callVisionAI(ai_api_key,
                                        "Find the best text input area for typing text content",
                                        screen_desc,
                                        element_descriptions);

        if (!ai_response.empty() && ai_response.contains("target_description"))
        {
            std::string target_desc = ai_response["target_description"];
            double confidence = ai_response.value("confidence", 0.5);

            std::cout << "🤖 AI selected target: '" << target_desc << "' (confidence: " << (confidence * 100) << "%)" << std::endl;

            // Find the element that best matches AI's selection
            UIElement bestElement;
            double bestMatch = 0.0;

            for (const auto &element : state.elements)
            {
                double match_score = 0.0;

                // Simple text matching with the AI's target description
                std::string element_text_lower = element.text;
                std::string target_lower = target_desc;
                std::transform(element_text_lower.begin(), element_text_lower.end(), element_text_lower.begin(), ::tolower);
                std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);

                if (element_text_lower.find("text") != std::string::npos && target_lower.find("text") != std::string::npos)
                    match_score += 0.5;
                if (element_text_lower.find("input") != std::string::npos && target_lower.find("input") != std::string::npos)
                    match_score += 0.5;
                if (element_text_lower.find("area") != std::string::npos && target_lower.find("area") != std::string::npos)
                    match_score += 0.5;

                // Prefer larger elements for main content areas
                int area = element.width * element.height;
                if (area > 50000)
                    match_score += 0.3;

                // Exclude system elements
                if (element.text == "Taskbar" || element.text.find("Search") != std::string::npos)
                    match_score -= 1.0;

                if (match_score > bestMatch)
                {
                    bestMatch = match_score;
                    bestElement = element;
                    bestElement.confidence = confidence;
                }
            }

            if (bestMatch > 0.0)
            {
                std::cout << "✅ AI-selected element: '" << bestElement.text << "' (type: " << bestElement.type << ")" << std::endl;
                return bestElement;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "⚠️ AI element selection failed, falling back to rule-based selection: " << e.what() << std::endl;
    }

    // Fallback to rule-based selection if AI fails
    UIElement bestElement;
    double bestScore = 0.0;

    std::cout << "🔍 Analyzing " << state.elements.size() << " UI elements for best text input..." << std::endl;
    for (const auto &element : state.elements)
    {
        double score = 0.0;

        std::string type_lower = element.type;
        std::transform(type_lower.begin(), type_lower.end(), type_lower.begin(), ::tolower);

        // EXCLUDE containers, taskbars, and non-input elements
        if (type_lower.find("container") != std::string::npos ||
            type_lower.find("taskbar") != std::string::npos ||
            type_lower.find("toolbar") != std::string::npos ||
            type_lower.find("menubar") != std::string::npos ||
            type_lower.find("statusbar") != std::string::npos ||
            element.text == "Taskbar" ||
            element.text == "Start Button")
        {
            std::cout << "   🚫 Excluding non-text element: '" << element.text << "' (type: " << element.type << ")" << std::endl;
            continue; // Skip this element entirely
        }

        // Score based on element type - prioritize actual text input elements
        if (type_lower.find("edit") != std::string::npos ||
            type_lower.find("textbox") != std::string::npos ||
            type_lower.find("textarea") != std::string::npos)
        {
            score += 1.5; // Higher score for actual text input types
        }
        else if (type_lower.find("text") != std::string::npos ||
                 type_lower.find("input") != std::string::npos)
        {
            score += 0.8;
        }

        // Boost score for application-specific text areas (avoid system search boxes)
        std::string app_lower = state.application_name;
        std::transform(app_lower.begin(), app_lower.end(), app_lower.begin(), ::tolower);

        // HEAVILY penalize Windows search boxes and system elements
        bool is_search_box = (element.text.find("Search") != std::string::npos ||
                              element.text.find("search") != std::string::npos ||
                              element.description.find("Search") != std::string::npos ||
                              element.description.find("search") != std::string::npos ||
                              element.text.find("Type here to search") != std::string::npos ||
                              element.description.find("Type here to search") != std::string::npos);

        if (is_search_box)
        {
            score -= 2.0; // HEAVILY penalize search boxes
            std::cout << "   🚫 Penalizing search box: '" << element.text << "' (type: " << element.type << ")" << std::endl;
        }

        // HEAVILY boost score if element is in the main application window
        bool is_app_specific = false;
        if (app_lower.find("notepad") != std::string::npos ||
            app_lower.find("word") != std::string::npos ||
            app_lower.find("excel") != std::string::npos ||
            app_lower.find("code") != std::string::npos ||
            app_lower.find("editor") != std::string::npos)
        {
            score += 1.0; // HEAVILY boost for main app elements
            is_app_specific = true;
            std::cout << "   ✅ Boosting app-specific element: '" << element.text << "' (type: " << element.type << ")" << std::endl;
        }

        // Score based on size (larger text areas are often better for content creation)
        int area = element.width * element.height;
        if (area > 50000) // Very large text areas (like Notepad main area)
            score += 0.6;
        else if (area > 10000) // Medium text areas
            score += 0.3;
        else if (area > 1000) // Small text areas
            score += 0.2;

        // Score based on position (center elements often better, but not always for search)
        int screen_width = GetSystemMetrics(SM_CXSCREEN);
        int screen_height = GetSystemMetrics(SM_CYSCREEN);
        double centerDistance = std::sqrt(std::pow(element.x - screen_width / 2, 2) +
                                          std::pow(element.y - screen_height / 2, 2));
        double maxDistance = std::sqrt(std::pow(screen_width / 2, 2) + std::pow(screen_height / 2, 2));

        // Only boost center position if it's not a search box
        if (!is_search_box)
        {
            score += 0.2 * (1.0 - centerDistance / maxDistance);
        }

        // Add base confidence
        score += element.confidence * 0.5;

        std::cout << "   📊 Element: '" << element.text << "' (type: " << element.type << ") - Score: " << score << std::endl;

        if (score > bestScore)
        {
            bestScore = score;
            bestElement = element;
            bestElement.confidence = score;
        }
    }

    std::cout << "🏆 Best element selected: '" << bestElement.text << "' with score: " << bestScore << std::endl;
    return bestElement;
}

std::string VisionGuidedExecutor::createVisionPrompt(const std::string &task,
                                                     const std::string &screen_description,
                                                     const std::vector<VisionTaskStep> &previous_steps)
{
    std::stringstream prompt;

    prompt << "You are an AI assistant that helps automate Windows tasks by controlling the mouse and keyboard.\n\n";
    prompt << "CURRENT TASK: " << task << "\n\n";
    prompt << "CURRENT SCREEN DESCRIPTION:\n";
    prompt << screen_description << "\n\n";

    if (!previous_steps.empty())
    {
        prompt << "PREVIOUS ACTIONS TAKEN:\n";
        for (size_t i = 0; i < previous_steps.size(); ++i)
        {
            const auto &step = previous_steps[i];
            prompt << (i + 1) << ". " << step.description;
            if (step.success)
            {
                prompt << " ✅";
            }
            else
            {
                prompt << " ❌ (" << step.error_message << ")";
            }
            prompt << "\n";
        }
        prompt << "\n";
    }

    prompt << "Based on the current screen and task, determine the NEXT SINGLE ACTION to take.\n\n";
    prompt << "YOU MUST respond with ONLY a JSON object in this EXACT format:\n\n";
    prompt << "```json\n";
    prompt << "{\n";
    prompt << "  \"action_type\": \"click|type|scroll|wait|complete\",\n";
    prompt << "  \"target_description\": \"exact description of UI element\",\n";
    prompt << "  \"value\": \"text to type or parameter\",\n";
    prompt << "  \"explanation\": \"why this action is needed\",\n";
    prompt << "  \"confidence\": 0.8\n";
    prompt << "}\n";
    prompt << "```\n\n";
    prompt << "ACTION TYPES:\n";
    prompt << "- click: Click on buttons, icons, menu items, or UI elements\n";
    prompt << "- type: Type text into input fields or text areas\n";
    prompt << "- scroll: Scroll the window (value: \"up\", \"down\", \"left\", \"right\")\n";
    prompt << "- wait: Wait for UI to load (value: milliseconds like \"2000\")\n";
    prompt << "- complete: Task is finished successfully\n\n";
    prompt << "WINDOWS AUTOMATION STRATEGY:\n";
    prompt << "- Analyze the current screen to understand what's visible\n";
    prompt << "- Look for UI elements relevant to your task\n";
    prompt << "- If you need to open an application, find a way to launch it (Start menu, search, desktop icons, etc.)\n";
    prompt << "- Be adaptive - use whatever UI elements are currently visible\n";
    prompt << "- Think step by step about what a human would do\n\n";

    prompt << "GENERAL APPROACH:\n";
    prompt << "- Use the target_description to describe exactly what you want to click/type in\n";
    prompt << "- Be specific about button text, field labels, or visual elements you can see\n";
    prompt << "- If you can't find the exact element, try clicking on related areas\n";
    prompt << "- Use 'wait' if you need time for UI to load after an action\n";
    prompt << "- Only use 'complete' when the entire task is 100% finished\n";
    prompt << "- Respond ONLY with the JSON, no other text\n";

    return prompt.str();
}

VisionAction VisionGuidedExecutor::parseGeminiResponse(const json &response, const std::string &task, const ScreenAnalysis &current_state)
{
    VisionAction action;

    try
    {
        std::string text_response;
        if (response.contains("candidates") && !response["candidates"].empty())
        {
            text_response = response["candidates"][0]["content"]["parts"][0]["text"];
        }
        else if (response.contains("choices") && !response["choices"].empty())
        {
            text_response = response["choices"][0]["message"]["content"].get<std::string>();
        }
        else if (response.contains("content"))
        {
            text_response = response["content"].get<std::string>();
        }
        else
        {
            throw std::runtime_error("Invalid DeepSeek R1 response format");
        }

        json action_json = json::parse(text_response);

        if (!action_json.contains("action_type"))
        {
            throw std::runtime_error("Missing 'action_type' in DeepSeek R1 response");
        }

        std::string action_type_str = action_json.value("action_type", "wait");
        if (action_type_str == "click")
            action.type = VisionActionType::CLICK;
        else if (action_type_str == "type")
            action.type = VisionActionType::TYPE;
        else if (action_type_str == "scroll")
            action.type = VisionActionType::SCROLL;
        else if (action_type_str == "wait")
            action.type = VisionActionType::WAIT;
        else if (action_type_str == "screenshot")
            action.type = VisionActionType::SCREENSHOT;
        else if (action_type_str == "verify")
            action.type = VisionActionType::VERIFY;
        else if (action_type_str == "navigate")
            action.type = VisionActionType::NAVIGATE;
        else
            throw std::runtime_error("Unknown 'action_type' in Gemini response");

        return action;
    }
    catch (const json::exception &e)
    {
        std::cerr << "❌ JSON parsing error: " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Error in parseGeminiResponse: " << e.what() << std::endl;
    }

    return action;
}

bool VisionGuidedExecutor::executeAction(const VisionAction &action, VisionTaskStep &step)
{
    try
    {
        switch (action.type)
        {
        case VisionActionType::CLICK:
            // Check if this is a PowerShell launch command
            if (action.target_description == "powershell_launch" && !action.value.empty())
            {
                std::cout << "🚀 Launching " << action.value << " via PowerShell" << std::endl;

                // Execute PowerShell command to launch application
                std::string command = "Start-Process " + action.value;
                std::string powershell_cmd = "powershell.exe -Command \"" + command + "\"";

                int result = system(powershell_cmd.c_str());
                if (result == 0)
                {
                    std::cout << "✅ Successfully launched " << action.value << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Wait for app to start
                    return true;
                }
                else
                {
                    step.error_message = "Failed to launch " + action.value + " via PowerShell";
                    return false;
                }
            }
            else
            {
                return executeClick(action.target_description, step.before_state);
            }

        case VisionActionType::TYPE:
            return executeType(action.target_description, action.value, step.before_state);

        case VisionActionType::SCROLL:
            return executeScroll(action.value, step.before_state);

        case VisionActionType::WAIT:
            return executeWait(std::stoi(action.value.empty() ? "1000" : action.value));

        case VisionActionType::SCREENSHOT:
            vision_processor->saveScreenshot("action_screenshot.png");
            return true;

        case VisionActionType::COMPLETE:
            return true;

        default:
            step.error_message = "Unknown action type";
            return false;
        }
    }
    catch (const std::exception &e)
    {
        step.error_message = "Action execution error: " + std::string(e.what());
        return false;
    }
}

bool VisionGuidedExecutor::executeClick(const std::string &target, const ScreenAnalysis &state)
{
    std::cout << "🎯 Looking for element: " << target << std::endl;

    // Handle special PowerShell launch command
    if (target == "powershell_launch")
    {
        std::cout << "🚀 Executing PowerShell launch command" << std::endl;
        // This will be handled by the calling code using the action.value
        return true;
    }

    // First, try to find element by exact text match
    UIElement target_element = vision_processor->findElementByText(target, state);

    if (target_element.confidence > 0.0)
    {
        std::cout << "✅ Found element by exact text: " << target_element.text << std::endl;
        return vision_processor->clickElement(target_element);
    }

    // Second, try fuzzy matching with partial text, description, or type
    for (const auto &element : state.elements)
    {
        // Convert to lowercase for case-insensitive matching
        std::string target_lower = target;
        std::string element_text_lower = element.text;
        std::string element_desc_lower = element.description;
        std::string element_type_lower = element.type;

        std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);
        std::transform(element_text_lower.begin(), element_text_lower.end(), element_text_lower.begin(), ::tolower);
        std::transform(element_desc_lower.begin(), element_desc_lower.end(), element_desc_lower.begin(), ::tolower);
        std::transform(element_type_lower.begin(), element_type_lower.end(), element_type_lower.begin(), ::tolower);

        // Check for partial matches
        bool text_match = !element_text_lower.empty() && element_text_lower.find(target_lower) != std::string::npos;
        bool desc_match = !element_desc_lower.empty() && element_desc_lower.find(target_lower) != std::string::npos;
        bool type_match = !element_type_lower.empty() && element_type_lower.find(target_lower) != std::string::npos;
        bool reverse_match = !target_lower.empty() && target_lower.find(element_text_lower) != std::string::npos;

        if (text_match || desc_match || type_match || reverse_match)
        {
            std::cout << "✅ Found element by fuzzy match: " << element.text << " (type: " << element.type << ")" << std::endl;
            return vision_processor->clickElement(element);
        }
    }

    // Third, try intelligent positioning based on common Windows patterns
    std::string target_lower = target;
    std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);

    // Dynamic detection of Windows UI patterns
    if (target_lower.find("start") != std::string::npos || target_lower.find("menu") != std::string::npos)
    {
        std::cout << "🔍 Attempting to find Start button/menu dynamically..." << std::endl;

        // Try to find Start button in taskbar area (bottom 100px of screen)
        int screen_height = GetSystemMetrics(SM_CYSCREEN);
        int screen_width = GetSystemMetrics(SM_CXSCREEN);

        // Look for elements in the taskbar area
        for (const auto &element : state.elements)
        {
            if (element.y > screen_height - 100 && element.x < 200) // Likely taskbar area
            {
                std::cout << "🏠 Found potential Start element in taskbar area" << std::endl;
                return vision_processor->clickElement(element);
            }
        }

        // Fallback: Try common Start button positions
        std::vector<std::pair<int, int>> start_positions = {
            {20, screen_height - 20}, // Windows 10/11 bottom-left
            {50, screen_height - 50}, // Alternative position
            {10, screen_height - 40}, // Edge case
        };

        for (const auto &pos : start_positions)
        {
            std::cout << "🎯 Trying Start button at (" << pos.first << "," << pos.second << ")" << std::endl;
            SetCursorPos(pos.first, pos.second);
            mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Check if Start menu opened by analyzing screen change
            ScreenAnalysis new_state = vision_processor->analyzeCurrentScreen();
            if (hasScreenChanged(state, new_state))
            {
                std::cout << "✅ Successfully opened Start menu" << std::endl;
                return true;
            }
        }
    }

    // Handle search box dynamically
    if (target_lower.find("search") != std::string::npos)
    {
        std::cout << "🔍 Looking for search functionality..." << std::endl;

        // Try Windows key to open search
        keybd_event(VK_LWIN, 0, 0, 0);
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "✅ Opened search with Windows key" << std::endl;
        return true;
    }

    // For other elements, try to find the closest match or make intelligent guesses
    if (!state.elements.empty())
    {
        std::cout << "🤔 No exact match found. Trying closest semantic match..." << std::endl;

        // Use simple keyword matching for common UI elements
        std::vector<std::string> keywords;
        std::istringstream iss(target_lower);
        std::string word;
        while (iss >> word)
        {
            keywords.push_back(word);
        }

        int best_score = 0;
        UIElement best_element;

        for (const auto &element : state.elements)
        {
            int score = 0;
            std::string element_full = element.text + " " + element.description + " " + element.type;
            std::transform(element_full.begin(), element_full.end(), element_full.begin(), ::tolower);

            for (const auto &keyword : keywords)
            {
                if (element_full.find(keyword) != std::string::npos)
                {
                    score++;
                }
            }

            if (score > best_score)
            {
                best_score = score;
                best_element = element;
            }
        }

        if (best_score > 0)
        {
            std::cout << "✅ Found best semantic match with score " << best_score << ": " << best_element.text << std::endl;
            return vision_processor->clickElement(best_element);
        }
    }

    std::cout << "❌ Could not find any matching element for: " << target << std::endl;
    std::cout << "📝 Available elements on screen:" << std::endl;
    for (size_t i = 0; i < state.elements.size() && i < 5; ++i)
    {
        const auto &elem = state.elements[i];
        std::cout << "  - " << elem.type << ": \"" << elem.text << "\" at (" << elem.x << "," << elem.y << ")" << std::endl;
    }

    return false;
}

bool VisionGuidedExecutor::executeType(const std::string &target, const std::string &text, const ScreenAnalysis &state)
{
    std::cout << "⌨️ Looking for text input: " << target << " to type: " << text << std::endl;

    // Always try to use smart text element detection first to avoid search boxes
    std::cout << "🔍 Using intelligent text input detection..." << std::endl;
    UIElement bestTextArea = findBestTextInputElement(state);

    std::cout << "📊 Best text element analysis:" << std::endl;
    std::cout << "   - Text: '" << bestTextArea.text << "'" << std::endl;
    std::cout << "   - Type: '" << bestTextArea.type << "'" << std::endl;
    std::cout << "   - Description: '" << bestTextArea.description << "'" << std::endl;
    std::cout << "   - Confidence: " << bestTextArea.confidence << std::endl;
    std::cout << "   - Position: (" << bestTextArea.x << ", " << bestTextArea.y << ")" << std::endl;
    std::cout << "   - Size: " << bestTextArea.width << "x" << bestTextArea.height << std::endl;

    // Use the intelligent element if it has decent confidence and isn't a search box
    bool is_search_box = (bestTextArea.text.find("Search") != std::string::npos ||
                          bestTextArea.text.find("search") != std::string::npos ||
                          bestTextArea.description.find("Search") != std::string::npos ||
                          bestTextArea.description.find("search") != std::string::npos);

    if (bestTextArea.confidence > 0.3 && !is_search_box)
    {
        std::cout << "✅ Using intelligent text area detection: " << bestTextArea.text << " (type: " << bestTextArea.type << ")" << std::endl;
        return vision_processor->typeAtElement(bestTextArea, text);
    }
    else if (is_search_box)
    {
        std::cout << "⚠️ Rejecting search box, looking for application text area..." << std::endl;
    }
    else
    {
        std::cout << "⚠️ Low confidence in intelligent detection (" << bestTextArea.confidence << "), trying fallback methods..." << std::endl;
    }

    // Convert target to lowercase for comparison
    std::string target_lower = target;
    std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);

    // First, try to find element by exact text match
    UIElement target_element = vision_processor->findElementByText(target, state);

    if (target_element.confidence > 0.0)
    {
        std::cout << "✅ Found text input by exact match: " << target_element.text << std::endl;
        return vision_processor->typeAtElement(target_element, text);
    }

    // Second, try fuzzy matching with text fields, input boxes, or similar elements
    for (const auto &element : state.elements)
    {
        std::string element_text_lower = element.text;
        std::string element_desc_lower = element.description;
        std::string element_type_lower = element.type;

        std::transform(element_text_lower.begin(), element_text_lower.end(), element_text_lower.begin(), ::tolower);
        std::transform(element_desc_lower.begin(), element_desc_lower.end(), element_desc_lower.begin(), ::tolower);
        std::transform(element_type_lower.begin(), element_type_lower.end(), element_type_lower.begin(), ::tolower);

        // Check for input-related elements
        bool is_input_element = (element_type_lower.find("text") != std::string::npos ||
                                 element_type_lower.find("input") != std::string::npos ||
                                 element_type_lower.find("edit") != std::string::npos ||
                                 element_type_lower.find("field") != std::string::npos ||
                                 element_type_lower.find("box") != std::string::npos); // Check for semantic matches
        bool text_match = !element_text_lower.empty() &&
                          (element_text_lower.find(target_lower) != std::string::npos ||
                           target_lower.find(element_text_lower) != std::string::npos);
        bool desc_match = !element_desc_lower.empty() &&
                          (element_desc_lower.find(target_lower) != std::string::npos ||
                           target_lower.find(element_desc_lower) != std::string::npos);

        // Avoid search boxes in fuzzy matching too
        bool is_search_element = (element_text_lower.find("search") != std::string::npos ||
                                  element_desc_lower.find("search") != std::string::npos);

        if (is_input_element && (text_match || desc_match) && !is_search_element)
        {
            std::cout << "✅ Found text input by fuzzy match: " << element.text << " (type: " << element.type << ")" << std::endl;
            return vision_processor->typeAtElement(element, text);
        }
        else if (is_input_element && (text_match || desc_match) && is_search_element)
        {
            std::cout << "⚠️ Skipping search element in fuzzy match: " << element.text << std::endl;
        }
    } // Third, look for any text input fields if no specific match found (avoiding search boxes)
    UIElement input_element = vision_processor->findElementByType("text_field", state);
    if (input_element.confidence == 0.0)
    {
        // Try other common input types, but exclude searchbox from fallback
        std::vector<std::string> input_types = {"edit", "textbox", "input"};
        for (const auto &input_type : input_types)
        {
            input_element = vision_processor->findElementByType(input_type, state);
            if (input_element.confidence > 0.0)
                break;
        }
    }

    // Check if found element is a search box and skip it
    bool found_element_is_search = (input_element.text.find("Search") != std::string::npos ||
                                    input_element.text.find("search") != std::string::npos ||
                                    input_element.description.find("Search") != std::string::npos ||
                                    input_element.description.find("search") != std::string::npos);

    if (input_element.confidence > 0.0 && !found_element_is_search)
    {
        std::cout << "✅ Found general text input element (non-search): " << input_element.text << std::endl;
        return vision_processor->typeAtElement(input_element, text);
    }
    else if (found_element_is_search)
    {
        std::cout << "⚠️ Skipping search element in general detection: " << input_element.text << std::endl;
    }

    // Fourth, try intelligent positioning based on common UI patterns
    std::cout << "🔍 Attempting dynamic text input detection..." << std::endl;

    // Look for elements that might be text inputs based on keywords
    std::vector<std::string> input_keywords = {"search", "type", "enter", "input", "text", "field", "box"};
    int best_score = 0;
    UIElement best_element;

    for (const auto &element : state.elements)
    {
        int score = 0;
        std::string element_full = element.text + " " + element.description + " " + element.type;
        std::transform(element_full.begin(), element_full.end(), element_full.begin(), ::tolower);

        // Score based on input-related keywords
        for (const auto &keyword : input_keywords)
        {
            if (element_full.find(keyword) != std::string::npos)
            {
                score++;
            }
        }

        // Additional scoring for target-specific keywords
        std::istringstream iss(target_lower);
        std::string word;
        while (iss >> word)
        {
            if (element_full.find(word) != std::string::npos)
            {
                score += 2; // Higher weight for target-specific terms
            }
        }

        if (score > best_score)
        {
            best_score = score;
            best_element = element;
        }
    }

    if (best_score > 0)
    {
        std::cout << "✅ Found best input candidate with score " << best_score << ": " << best_element.text << std::endl;
        return vision_processor->typeAtElement(best_element, text);
    }

    // Fifth, fallback strategies for common scenarios
    if (target_lower.find("search") != std::string::npos)
    {
        std::cout << "🔍 Attempting search box fallback strategies..." << std::endl;

        // Try Windows key + typing (universal search)
        keybd_event(VK_LWIN, 0, 0, 0);
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Type the text directly
        for (char c : text)
        {
            SHORT vk = VkKeyScan(c);
            if (vk != -1)
            {
                keybd_event(LOBYTE(vk), 0, 0, 0);
                keybd_event(LOBYTE(vk), 0, KEYEVENTF_KEYUP, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        std::cout << "✅ Used Windows search fallback" << std::endl;
        return true;
    }

    // If all else fails, try clicking in likely text input areas and typing
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    std::vector<std::pair<int, int>> likely_input_positions;

    // Add positions based on common UI layouts
    if (target_lower.find("address") != std::string::npos || target_lower.find("url") != std::string::npos)
    {
        // Browser address bar area
        likely_input_positions.push_back({screen_width / 2, 50});
        likely_input_positions.push_back({screen_width / 2, 100});
    }
    else
    {
        // General input areas - center of screen, dialog areas
        likely_input_positions.push_back({screen_width / 2, screen_height / 2});
        likely_input_positions.push_back({screen_width / 2, screen_height / 3});
        likely_input_positions.push_back({screen_width / 2, 200});
    }

    for (const auto &pos : likely_input_positions)
    {
        std::cout << "🎯 Trying text input at position (" << pos.first << "," << pos.second << ")" << std::endl;

        // Click at the position
        SetCursorPos(pos.first, pos.second);
        mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // Try typing
        bool typing_success = true;
        for (char c : text)
        {
            SHORT vk = VkKeyScan(c);
            if (vk == -1)
            {
                typing_success = false;
                break;
            }
            keybd_event(LOBYTE(vk), 0, 0, 0);
            keybd_event(LOBYTE(vk), 0, KEYEVENTF_KEYUP, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (typing_success)
        {
            // Check if text appeared by analyzing screen change
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            ScreenAnalysis new_state = vision_processor->analyzeCurrentScreen();
            if (hasScreenChanged(state, new_state))
            {
                std::cout << "✅ Successfully typed text at fallback position" << std::endl;
                return true;
            }
        }
    }

    std::cout << "❌ Could not find any suitable text input for: " << target << std::endl;
    std::cout << "📝 Available elements on screen:" << std::endl;
    for (size_t i = 0; i < state.elements.size() && i < 5; ++i)
    {
        const auto &elem = state.elements[i];
        std::cout << "  - " << elem.type << ": \"" << elem.text << "\" at (" << elem.x << "," << elem.y << ")" << std::endl;
    }

    return false;
}

bool VisionGuidedExecutor::executeScroll(const std::string &direction, const ScreenAnalysis &state)
{
    // Simple scroll implementation
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;

    if (direction == "up")
    {
        input.mi.mouseData = WHEEL_DELTA;
    }
    else if (direction == "down")
    {
        input.mi.mouseData = -WHEEL_DELTA;
    }
    else
    {
        return false;
    }

    SendInput(1, &input, sizeof(INPUT));
    return true;
}

bool VisionGuidedExecutor::executeWait(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    return true;
}

bool VisionGuidedExecutor::verifyActionSuccess(const VisionAction &action,
                                               const ScreenAnalysis &before,
                                               const ScreenAnalysis &after)
{
    // Basic verification - check if screen changed
    return hasScreenChanged(before, after);
}

bool VisionGuidedExecutor::hasScreenChanged(const ScreenAnalysis &before, const ScreenAnalysis &after)
{
    // Simple comparison - check if element count changed or window title changed
    return (before.elements.size() != after.elements.size()) ||
           (before.window_title != after.window_title) ||
           (before.application_name != after.application_name);
}

bool VisionGuidedExecutor::isTaskComplete(const std::string &task, const ScreenAnalysis &state)
{
    // Simple heuristic - task-specific completion checking would go here
    // For now, we rely on Gemini to tell us when complete
    return false;
}

bool VisionGuidedExecutor::attemptRecovery(const VisionTaskStep &failed_step)
{
    std::cout << "🔄 Attempting recovery from failed step..." << std::endl;

    // Simple recovery - take a screenshot and wait
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    return true;
}

void VisionGuidedExecutor::setMaxSteps(int max)
{
    max_steps = max;
}

void VisionGuidedExecutor::setTempDirectory(const std::string &path)
{
    temp_directory = path;
    vision_processor->setTempDirectory(path);
}

ScreenAnalysis VisionGuidedExecutor::getCurrentScreenState()
{
    return vision_processor->analyzeCurrentScreen();
}

VisionProcessor *VisionGuidedExecutor::getVisionProcessor()
{
    return vision_processor.get();
}

// Helper functions for dynamic task analysis

// Detect application intent from natural language
TaskComponents VisionGuidedExecutor::detectApplicationIntent(const std::string &lower_task)
{
    TaskComponents components;

    // Basic application mappings
    std::vector<std::pair<std::string, std::string>> appMappings = {
        {"notepad", "notepad.exe"},
        {"calculator", "calc.exe"},
        {"chrome", "chrome.exe"},
        {"edge", "msedge.exe"},
        {"browser", "msedge.exe"},
        {"word", "winword.exe"},
        {"excel", "excel.exe"},
        {"powerpoint", "powerpnt.exe"},
        {"paint", "mspaint.exe"},
        {"file explorer", "explorer.exe"},
        {"task manager", "taskmgr.exe"},
        {"file manager", "explorer.exe"},
        {"text editor", "notepad.exe"}};

    // Check for direct app mentions
    for (const auto &mapping : appMappings)
    {
        if (lower_task.find(mapping.first) != std::string::npos)
        {
            components.needsAppLaunch = true;
            components.targetApp = mapping.second;
            components.appName = mapping.first;
            return components;
        }
    }

    // Check for intent-based patterns
    if (isWebTask(lower_task))
    {
        components.needsAppLaunch = true;
        components.targetApp = "msedge.exe";
        components.appName = "browser";
        components.needsNavigation = true;
        components.navigationTarget = buildWebUrl(lower_task);
    }
    else if (isMessagingTask(lower_task))
    {
        auto messagingInfo = getMessagingInfo(lower_task);
        components.needsAppLaunch = true;
        components.targetApp = messagingInfo.first;
        components.appName = messagingInfo.second;
        components.needsInteraction = true;
        components.interactionTarget = extractContactName(lower_task);
        components.needsTyping = true;
        components.textToType = extractMessageText(lower_task);
    }
    else if (isFileTask(lower_task))
    {
        components.needsAppLaunch = true;
        components.targetApp = "explorer.exe";
        components.appName = "file explorer";
    }
    else if (isSystemTask(lower_task))
    {
        auto systemInfo = getSystemAppInfo(lower_task);
        components.needsAppLaunch = true;
        components.targetApp = systemInfo.first;
        components.appName = systemInfo.second;
    }

    return components;
}

// Extract search query from task, removing specified words
std::string VisionGuidedExecutor::extractSearchQuery(const std::string &task, const std::vector<std::string> &removeWords)
{
    std::string query = task;
    std::string lower_query = task;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    // Remove action words and app names
    std::vector<std::string> commonWords = {"watch", "search", "find", "look", "for", "on", "in", "the", "a", "an", "to", "and", "or"};
    commonWords.insert(commonWords.end(), removeWords.begin(), removeWords.end());

    for (const auto &word : commonWords)
    {
        std::string lower_word = word;
        std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);

        size_t pos = 0;
        while ((pos = lower_query.find(lower_word, pos)) != std::string::npos)
        {
            // Check if it's a whole word
            bool isWholeWord = (pos == 0 || !std::isalnum(lower_query[pos - 1])) &&
                               (pos + lower_word.length() >= lower_query.length() || !std::isalnum(lower_query[pos + lower_word.length()]));

            if (isWholeWord)
            {
                query.erase(pos, word.length());
                lower_query.erase(pos, lower_word.length());
                // Remove extra spaces
                while (pos < query.length() && std::isspace(query[pos]))
                {
                    query.erase(pos, 1);
                    lower_query.erase(pos, 1);
                }
            }
            else
            {
                pos += lower_word.length();
            }
        }
    }

    // Clean up extra spaces
    query = std::regex_replace(query, std::regex("\\s+"), " ");
    query = std::regex_replace(query, std::regex("^\\s+|\\s+$"), "");

    return query.empty() ? task : query;
}

// Extract contact name from messaging task
std::string VisionGuidedExecutor::extractContactName(const std::string &task)
{
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    // Look for patterns like "message [name]" or "send to [name]"
    std::vector<std::regex> patterns = {
        std::regex("message\\s+(\\w+)", std::regex_constants::icase),
        std::regex("send\\s+to\\s+(\\w+)", std::regex_constants::icase),
        std::regex("text\\s+(\\w+)", std::regex_constants::icase),
        std::regex("whatsapp\\s+(\\w+)", std::regex_constants::icase),
        std::regex("telegram\\s+(\\w+)", std::regex_constants::icase)};

    std::smatch match;
    for (const auto &pattern : patterns)
    {
        if (std::regex_search(task, match, pattern))
        {
            return match[1].str();
        }
    }

    return "";
}

// Extract message text from messaging task
std::string VisionGuidedExecutor::extractMessageText(const std::string &task)
{
    // Look for text in quotes first
    std::regex quote_regex("\"([^\"]+)\"|'([^']+)'");
    std::smatch match;
    if (std::regex_search(task, match, quote_regex))
    {
        for (size_t i = 1; i <= 2; ++i)
        {
            if (match[i].matched)
            {
                return match[i].str();
            }
        }
    }

    // Look for text after "hello", "hi", etc.
    std::vector<std::string> greetings = {"hello", "hi", "hey", "good morning", "good evening"};
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    for (const auto &greeting : greetings)
    {
        size_t pos = lower_task.find(greeting);
        if (pos != std::string::npos)
        {
            return greeting;
        }
    }





    return "Hello"; // Default message
}

// Check if task is web-related
bool VisionGuidedExecutor::isWebTask(const std::string &task)
{
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    std::vector<std::string> webKeywords = {
        "youtube", "google", "search", "website", "browse", "online",
        "facebook", "twitter", "instagram", "gmail", "email", "news",
        "weather", "maps", "shopping", "netflix", "spotify", "twitch",
        "watch", "stream", "video", "music", "social media", "web",
        "internet", "url", "link", "site", "cricket", "sports", "movie"};

    for (const auto &keyword : webKeywords)
    {
        if (lower_task.find(keyword) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

// Check if task is messaging-related
bool VisionGuidedExecutor::isMessagingTask(const std::string &task)
{
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    std::vector<std::string> messagingKeywords = {
        "whatsapp", "telegram", "discord", "slack", "teams", "skype",
        "message", "text", "send", "chat", "dm", "call"};

    for (const auto &keyword : messagingKeywords)
    {
        if (lower_task.find(keyword) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

// Check if task is file-related
bool VisionGuidedExecutor::isFileTask(const std::string &task)
{
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    std::vector<std::string> fileKeywords = {
        "file", "folder", "directory", "open", "save", "copy", "move",
        "delete", "create", "new file", "new folder", "document"};

    for (const auto &keyword : fileKeywords)
    {
        if (lower_task.find(keyword) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

// Check if task is system-related
bool VisionGuidedExecutor::isSystemTask(const std::string &task)
{
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    std::vector<std::string> systemKeywords = {
        "settings", "control panel", "task manager", "device manager",
        "system", "registry", "services", "startup", "shutdown", "restart"};

    for (const auto &keyword : systemKeywords)
    {
        if (lower_task.find(keyword) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

// Helper function to build web URLs
std::string VisionGuidedExecutor::buildWebUrl(const std::string &task)
{
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    // YouTube patterns
    if (lower_task.find("youtube") != std::string::npos)
    {
        if (lower_task.find("watch") != std::string::npos || lower_task.find("video") != std::string::npos)
        {
            std::string query = extractSearchQuery(task, {"watch", "on", "youtube", "video"});
            return "https://youtube.com/results?search_query=" + urlEncode(query);
        }
        return "https://youtube.com";
    }

    // Weather patterns
    if (lower_task.find("weather") != std::string::npos)
    {
        return "https://google.com/search?q=weather";
    }

    // News patterns
    if (lower_task.find("news") != std::string::npos)
    {
        if (lower_task.find("today") != std::string::npos || lower_task.find("latest") != std::string::npos)
        {
            return "https://google.com/search?q=today+news";
        }
        return "https://news.google.com";
    }

    // Sports patterns
    if (lower_task.find("cricket") != std::string::npos || lower_task.find("sports") != std::string::npos)
    {
        std::string query = extractSearchQuery(task, {"watch", "see", "latest"});
        return "https://google.com/search?q=" + urlEncode(query);
    }

    // Social media patterns
    if (lower_task.find("facebook") != std::string::npos)
        return "https://facebook.com";
    if (lower_task.find("twitter") != std::string::npos)
        return "https://twitter.com";
    if (lower_task.find("instagram") != std::string::npos)
        return "https://instagram.com";

    // Generic search
    std::string query = extractSearchQuery(task, {"search", "find", "look"});
    return "https://google.com/search?q=" + urlEncode(query);
}

// Helper function to get messaging app info
std::pair<std::string, std::string> VisionGuidedExecutor::getMessagingInfo(const std::string &task)
{
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    if (lower_task.find("whatsapp") != std::string::npos)
        return {"WhatsApp.exe", "whatsapp"};
    else if (lower_task.find("telegram") != std::string::npos)
        return {"Telegram.exe", "telegram"};
    else if (lower_task.find("discord") != std::string::npos)
        return {"Discord.exe", "discord"};
    else if (lower_task.find("slack") != std::string::npos)
        return {"slack.exe", "slack"};
    else if (lower_task.find("teams") != std::string::npos)
        return {"ms-teams.exe", "teams"};
    else if (lower_task.find("skype") != std::string::npos)
        return {"Skype.exe", "skype"};

    // Default to web WhatsApp
    return {"msedge.exe", "whatsapp"};
}

// Helper function to get system app info
std::pair<std::string, std::string> VisionGuidedExecutor::getSystemAppInfo(const std::string &task)
{
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);

    if (lower_task.find("task manager") != std::string::npos)
        return {"taskmgr.exe", "task manager"};
    else if (lower_task.find("control panel") != std::string::npos)
        return {"control.exe", "control panel"};
    else if (lower_task.find("settings") != std::string::npos)
        return {"ms-settings:", "settings"};
    else if (lower_task.find("device manager") != std::string::npos)
        return {"devmgmt.msc", "device manager"};

    return {"", ""};
}

// Helper function to URL encode strings
std::string VisionGuidedExecutor::urlEncode(const std::string &value)
{
    std::string encoded;
    for (char c : value)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded += c;
        }
        else if (c == ' ')
        {
            encoded += '+';
        }
        else
        {
            encoded += '%';
            encoded += "0123456789ABCDEF"[static_cast<unsigned char>(c) >> 4];
            encoded += "0123456789ABCDEF"[static_cast<unsigned char>(c) & 15];
        }
    }
    return encoded;
}
