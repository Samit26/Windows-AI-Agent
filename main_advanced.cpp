#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include "include/json.hpp"
#include "ai_model.h"
// #include "context_manager.h" // Removed
#include "task_planner.h"
#include "advanced_executor.h"
#include "multimodal_handler.h"
#include "http_server.h"

using json = nlohmann::json;

class AdvancedAIAgent
{
private:
    std::string api_key;
    // ContextManager context_manager; // Removed
    TaskPlanner task_planner; // Will be initialized in constructor
    AdvancedExecutor advanced_executor;
    MultiModalHandler multimodal_handler;
    HttpServer http_server;
    bool interactive_mode;
    bool learning_enabled;
    bool server_mode;

public:
    AdvancedAIAgent() : interactive_mode(true),
                        learning_enabled(true),
                        server_mode(false),
                        http_server(8080),
                        task_planner("") // Initialize with empty key, will be set in loadConfiguration
    {
        loadConfiguration(); // api_key is loaded here, then task_planner is re-initialized
        displayWelcomeMessage();
    }

    void loadConfiguration()
    {
        std::ifstream config_file("config_advanced.json");
        if (!config_file.is_open())
        {
            std::cerr << "❌ Error: Could not open config_advanced.json" << std::endl;
            std::cerr << "Please ensure the configuration file 'config_advanced.json' exists in the correct location." << std::endl;
            exit(1);
        }
        else
        {
            std::cout << "📝 Loaded configuration from config_advanced.json" << std::endl;
        }

        json config;
        try
        {
            config_file >> config;
        }
        catch (const json::parse_error &e)
        {
            std::cerr << "❌ Error: Failed to parse config_advanced.json. Malformed JSON: " << e.what() << std::endl;
            exit(1);
        }
        api_key = config["api_key"].get<std::string>();

        // Re-initialize task_planner with the loaded API key
        task_planner = TaskPlanner(api_key);

        // Initialize vision capabilities with AI API key
        advanced_executor.setAIApiKey(api_key);

        // Load advanced settings if available
        if (config.contains("execution_mode"))
        {
            std::string mode = config["execution_mode"];
            if (mode == "safe")
                advanced_executor.setExecutionMode(ExecutionMode::SAFE);
            else if (mode == "interactive")
                advanced_executor.setExecutionMode(ExecutionMode::INTERACTIVE);
            else if (mode == "autonomous")
                advanced_executor.setExecutionMode(ExecutionMode::AUTONOMOUS);
        }

        if (config.contains("enable_voice"))
        {
            bool voice_enabled = config["enable_voice"];
            if (voice_enabled)
            {
                multimodal_handler.enableVoiceInput();
            }
        }
        if (config.contains("enable_image_analysis"))
        {
            bool image_enabled = config["enable_image_analysis"];
            if (image_enabled)
            {
                multimodal_handler.enableImageAnalysis();
            }
        }

        // Check for server mode
        if (config.contains("server_mode"))
        {
            server_mode = config["server_mode"];
        }

        // Configure HTTP server if needed        if (server_mode)
        {
            VisionProcessor *vp = multimodal_handler.getVisionProcessor();
            http_server.setComponents(&advanced_executor,
                                      &task_planner, &multimodal_handler,
                                      vp, api_key);
        }
    }
    void displayWelcomeMessage()
    {
        std::cout << "========================================" << std::endl;
        std::cout << "🤖 ADVANCED WINDOWS AI AGENT v2.0 🤖" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Features:" << std::endl;
        // std::cout << "✓ Context Memory & Learning" << std::endl; // Removed
        std::cout << "✓ Advanced Task Planning" << std::endl;
        std::cout << "✓ Multi-Modal Input Support" << std::endl;
        std::cout << "✓ Safe Execution Environment" << std::endl;
        std::cout << "✓ Session Management" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  Type your request normally" << std::endl;
        std::cout << "  ':voice' - Enable voice input" << std::endl;
        std::cout << "  ':screenshot' - Analyze current screen" << std::endl;
        // std::cout << "  ':history' - Show conversation history" << std::endl; // Removed
        std::cout << "  ':mode safe/interactive/autonomous' - Change execution mode" << std::endl;
        std::cout << "  ':quit', 'quit', 'exit', or 'q' - Exit the application" << std::endl;
        std::cout << "========================================" << std::endl;
    }
    // TODO: Unit Test: Consider tests for processUserInput logic, mocking AI calls and executor actions.
    void processUserInput(const std::string &input)
    {
        // Handle special commands
        if (input.length() > 0 && input[0] == ':')
        {
            handleSpecialCommand(input);
            return;
        }

        // Check if this is a vision task (natural language task)
        if (isVisionTask(input))
        {
            handleVisionTask(input);
            return;
        }

        try
        {
            // Process input through context manager (replaced with direct input)
            std::string contextual_prompt = input; // Was context_manager.getContextualPrompt(input);
            // Call AI Model API with enhanced context
            json response = callAIModel(api_key, contextual_prompt);

            // Enhanced Error Handling for AI Response
            if (response.empty() || response.is_null())
            {
                std::cerr << "❌ Error in processUserInput: AI model returned an empty or invalid JSON response." << std::endl;
                // Optionally, inform the user if this is a critical failure path
                // std::cout << "⚠️ AI service seems to be having issues. Please try again later." << std::endl;
                return;
            }

            if (!response.contains("type"))
            {
                std::cerr << "❌ Error in processUserInput: AI response JSON does not contain a 'type' field. Response: "
                          << response.dump(2) << std::endl;
                std::cout << "⚠️ AI response was not in the expected format. The response was: " << response.dump(2) << std::endl;
                return;
            }

            // Proceed with processing if "type" field exists
            std::string response_type = response["type"];

            if (response_type == "powershell_script")
            {
                if (!response.contains("script"))
                {
                    std::cerr << "❌ Error in processUserInput: PowerShell script task is missing 'script' field. Response: "
                              << response.dump(2) << std::endl;
                    std::cout << "⚠️ AI response for PowerShell script was malformed (missing 'script')." << std::endl;
                    return;
                }
                // Handle PowerShell script execution
                TaskPlan plan = task_planner.planTask(input, response);
                displayTaskPlan(plan);

                bool proceed = true;
                if (advanced_executor.getExecutionMode() == ExecutionMode::INTERACTIVE)
                {
                    proceed = askForConfirmation(response);
                }

                if (proceed)
                {
                    ExecutionResult result = advanced_executor.executeWithPlan(plan);
                    // context_manager.addToHistory(input, // Removed
                    //                              response.value("explanation", "Executed script"),
                    //                              response.dump(),
                    //                              result.success);
                    displayExecutionResult(result);

                    if (learning_enabled)
                    {
                        advanced_executor.learnFromExecution(response, result);
                    }
                }
                else
                {
                    std::cout << "🚫 Execution cancelled by user." << std::endl;
                }
            }
            else if (response_type == "vision_task")
            {
                // This block handles when callAIModel *directly* identifies a vision_task,
                // bypassing the isVisionTask() -> callIntentAI() path.
                if (!response.contains("objective"))
                {
                    std::cerr << "⚠️ Warning in processUserInput: Vision task from callAIModel is missing 'objective' field. Response: "
                              << response.dump(2) << std::endl;
                    // If objective is critical and cannot be defaulted to 'input', consider returning.
                    // For now, it defaults to 'input' via .value("objective", input) below.
                }
                if (!response.contains("initial_action") && !response.contains("objective"))
                {
                    std::cerr << "⚠️ Warning in processUserInput: Vision task from callAIModel is missing both 'initial_action' and 'objective'. Response: "
                              << response.dump(2) << std::endl;
                    // This task might be too vague to proceed.
                }

                // Handle vision task - first execute initial action if present
                if (response.contains("initial_action"))
                {
                    std::string initial_cmd = response["initial_action"];
                    std::cout << "🚀 Launching application (direct vision_task): " << initial_cmd << std::endl;

                    json launch_task = {
                        {"type", "powershell_script"},
                        {"script", {initial_cmd}},
                        {"explanation", "Launching required application for vision task"}};

                    TaskPlan launch_plan = task_planner.planTask("Launch application for vision task", launch_task);
                    ExecutionResult launch_result = advanced_executor.executeWithPlan(launch_plan);

                    if (!launch_result.success)
                    {
                        std::cout << "❌ Failed to launch application for vision task. Aborting." << std::endl;
                        return;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                }

                std::string objective = response.value("objective", input); // Default to original input if objective is missing
                handleVisionTask(objective);
            }
            else // Handles "text" type or any other unknown types
            {
                std::string content = response.value("content", "");
                if (content.empty() && response_type != "text")
                {
                    std::cerr << "⚠️ Warning in processUserInput: AI response type '" << response_type
                              << "' but 'content' field is missing or empty. Response: " << response.dump(2) << std::endl;
                    std::cout << "💬 AI Response: Received an unusual response type '" << response_type << "' without content." << std::endl;
                }
                else if (content.empty() && response_type == "text")
                {
                    std::cerr << "⚠️ Warning in processUserInput: AI response type 'text' but 'content' is empty. Response: "
                              << response.dump(2) << std::endl;
                    std::cout << "💬 AI returned an empty text response." << std::endl;
                }
                else
                {
                    std::cout << "💬 AI Response: " << content << std::endl;
                }
                // context_manager.addToHistory(input, response.dump(), response_type, true); // Removed
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "❌ Error: " << e.what() << std::endl;
            // context_manager.addToHistory(input, "", "error", false); // Removed
        }
    }

    void handleSpecialCommand(const std::string &command)
    {
        if (command == ":voice")
        {
            std::cout << "🎤 Starting voice input..." << std::endl;
            std::string audio_file = multimodal_handler.startVoiceRecording();
            std::cout << "Press Enter when done speaking..." << std::endl;
            std::cin.get();
            multimodal_handler.stopVoiceRecording();

            MultiModalInput voice_input = multimodal_handler.processVoiceInput(audio_file);
            std::cout << "🎤 Transcribed: " << voice_input.content << std::endl;
            processUserInput(voice_input.content);
        }
        else if (command == ":screenshot")
        {
            std::cout << "📸 Analyzing current screen with Qwen..." << std::endl;
            ScreenAnalysis analysis_result = multimodal_handler.analyzeCurrentScreen();

            std::cout << "\n--- Qwen Screen Analysis ---" << std::endl;
            std::cout << "📝 Description: " << analysis_result.overall_description << std::endl;

            if (!analysis_result.screenshot_path.empty()) {
                std::cout << "🖼️ Screenshot saved at: " << analysis_result.screenshot_path << std::endl;
            }

            std::cout << "\n--- Detected UI Elements (" << analysis_result.elements.size() << ") ---" << std::endl;
            if (analysis_result.elements.empty()) {
                std::cout << "No UI elements were specifically identified by Qwen or extracted." << std::endl;
            } else {
                for (size_t i = 0; i < analysis_result.elements.size(); ++i) {
                    const auto& el = analysis_result.elements[i];
                    std::cout << "  Element " << i + 1 << ":" << std::endl;
                    std::cout << "    Type: " << el.type << std::endl;
                    std::cout << "    Text: \"" << el.text << "\"" << std::endl;
                    std::cout << "    BBox: [x:" << el.x << ", y:" << el.y << ", w:" << el.width << ", h:" << el.height << "]" << std::endl;
                    std::cout << "    Confidence: " << el.confidence << std::endl;
                    if (i >= 4 && analysis_result.elements.size() > 5) { // Print max 5 elements if many
                        std::cout << "  ... and " << (analysis_result.elements.size() - (i + 1)) << " more elements." << std::endl;
                        break;
                    }
                }
            }
            std::cout << "---------------------------------" << std::endl;
        }
        // else if (command == ":history") // Removed
        // {
        //     std::cout << "📜 Recent conversation history:" << std::endl;
        //     std::cout << context_manager.getRecentContext(5) << std::endl;
        // }
        else if (command.length() > 6 && command.substr(0, 6) == ":mode ")
        {
            std::string mode = command.substr(6);
            if (mode == "safe")
            {
                advanced_executor.setExecutionMode(ExecutionMode::SAFE);
                std::cout << "🛡️ Execution mode set to SAFE" << std::endl;
            }
            else if (mode == "interactive")
            {
                advanced_executor.setExecutionMode(ExecutionMode::INTERACTIVE);
                std::cout << "🤝 Execution mode set to INTERACTIVE" << std::endl;
            }
            else if (mode == "autonomous")
            {
                advanced_executor.setExecutionMode(ExecutionMode::AUTONOMOUS);
                std::cout << "🚀 Execution mode set to AUTONOMOUS" << std::endl;
            }
        }
        else if (command == ":quit")
        {
            std::cout << "👋 Goodbye!" << std::endl; // "Session saved" removed
            // context_manager.saveSession(); // Removed
            exit(0);
        }
        else
        {
            std::cout << "❓ Unknown command: " << command << std::endl;
        }
    }

    void displayTaskPlan(const TaskPlan &plan)
    {
        std::cout << "📋 Task Plan: " << plan.objective << std::endl;
        std::cout << "🎯 Confidence: " << (plan.overall_confidence * 100) << "%" << std::endl;
        std::cout << "📝 Steps:" << std::endl;
        for (size_t i = 0; i < plan.tasks.size(); i++)
        {
            const auto &task = plan.tasks[i];
            std::cout << "  " << (i + 1) << ". " << task.description << std::endl;
            for (const auto &cmd : task.commands)
            {
                std::cout << "     → " << cmd << std::endl;
            }
        }
    }

    bool askForConfirmation(const json &response)
    {
        std::cout << "⚠️ Execution Plan:" << std::endl;
        std::cout << "📝 Description: " << response.value("explanation", "No description") << std::endl;
        std::cout << "🎯 Confidence: " << (response.value("confidence", 0.0) * 100) << "%" << std::endl;

        if (response.contains("script"))
        {
            std::cout << "💻 Commands to execute:" << std::endl;
            for (const auto &cmd : response["script"])
            {
                std::cout << "  → " << cmd.get<std::string>() << std::endl;
            }
        }

        std::cout << "🤔 Do you want to proceed? (y/N): ";
        std::string confirmation;
        std::getline(std::cin, confirmation);
        return (confirmation == "y" || confirmation == "Y" || confirmation == "yes");
    }

    void displayExecutionResult(const ExecutionResult &result)
    {
        if (result.success)
        {
            std::cout << "✅ Execution successful!" << std::endl;
            if (!result.output.empty())
            {
                std::cout << "📤 Output: " << result.output << std::endl;
            }
        }
        else
        {
            std::cout << "❌ Execution failed!" << std::endl;
            std::cout << "⚠️ Error: " << result.error_message << std::endl;
        }
        std::cout << "⏱️ Execution time: " << result.execution_time << "s" << std::endl;
    }
    bool isVisionTask(const std::string &input)
    {
        // Use AI to dynamically determine if this is a vision task
        try
        {
            json intent = callIntentAI(api_key, input);

            if (intent.empty() || intent.is_null())
            {
                std::cerr << "⚠️ Warning in isVisionTask: AI intent analysis returned empty or null JSON. Defaulting to non-vision task. Raw input: " << input << std::endl;
                return false; // Default to non-vision task
            }

            if (!intent.contains("is_vision_task"))
            {
                std::cerr << "⚠️ Warning in isVisionTask: AI intent JSON is missing 'is_vision_task' field. Defaulting to non-vision task. Intent: "
                          << intent.dump(2) << std::endl;
                return false; // Default to non-vision task
            }

            bool is_vision = intent["is_vision_task"];
            double confidence = intent.value("confidence", 0.5);

            std::cout << "🤖 AI Intent Analysis: " << (is_vision ? "Vision Task" : "Regular Task")
                      << " (confidence: " << (confidence * 100) << "%)" << std::endl;

            return is_vision;
        }
        catch (const std::exception &e)
        {
            std::cerr << "❌ Error in isVisionTask during AI call: " << e.what() << ". Defaulting to non-vision task for input: " << input << std::endl;
        }

        // If AI fails or JSON is malformed, default to false
        return false;
    }

    void handleVisionTask(const std::string &input)
    {
        std::cout << "🎯 Detected vision task: " << input << std::endl;
        std::cout << "👁️ Analyzing screen and planning execution..." << std::endl;

        try
        {
            // Execute natural language task using vision
            ExecutionResult result = advanced_executor.executeNaturalLanguageTask(input);

            // Display detailed results
            displayVisionTaskResult(result, input);

            // Add to context
            // context_manager.addToHistory(input, result.output, "vision_task", result.success); // Removed
        }
        catch (const std::exception &e)
        {
            std::cout << "❌ Vision task failed: " << e.what() << std::endl;
        }
    }

    void displayVisionTaskResult(const ExecutionResult &result, const std::string &original_task)
    {
        std::cout << "\n🎬 Vision Task Execution Summary" << std::endl;
        std::cout << "📋 Task: " << original_task << std::endl;

        if (result.success)
        {
            std::cout << "✅ Status: Completed Successfully" << std::endl;
        }
        else
        {
            std::cout << "❌ Status: Failed" << std::endl;
        }

        std::cout << "📝 Result: " << result.output << std::endl;
        std::cout << "⏱️ Total Time: " << result.execution_time << "s" << std::endl;

        if (result.metadata.contains("steps_executed"))
        {
            std::cout << "🔢 Steps Executed: " << result.metadata["steps_executed"] << std::endl;
        }

        if (result.metadata.contains("step_details"))
        {
            std::cout << "📊 Step Details:" << std::endl;
            int step_num = 1;
            for (const auto &step : result.metadata["step_details"])
            {
                std::cout << "  " << step_num << ". " << step.value("description", "Unknown step");
                if (step.value("success", false))
                {
                    std::cout << " ✅";
                }
                else
                {
                    std::cout << " ❌";
                    if (step.contains("error"))
                    {
                        std::cout << " (" << step["error"] << ")";
                    }
                }
                std::cout << std::endl;
                step_num++;
            }
        }

        if (!result.success && !result.error_message.empty())
        {
            std::cout << "🚨 Error Details: " << result.error_message << std::endl;
        }

        std::cout << std::endl;
    }
    void run()
    {
        if (server_mode)
        {
            runServerMode();
        }
        else
        {
            runInteractiveMode();
        }
    }

    void runServerMode()
    {
        std::cout << "\n🌐 Starting HTTP Server Mode..." << std::endl;

        if (http_server.start())
        {
            std::cout << "✅ Server started successfully on port 8080" << std::endl;
            std::cout << "🌐 Frontend can connect at: http://localhost:8080" << std::endl;
            std::cout << "📋 Available endpoints:" << std::endl;
            std::cout << "   POST /api/execute - Execute tasks" << std::endl;
            std::cout << "   GET  /api/history - Get conversation history" << std::endl;
            std::cout << "   GET  /api/system-info - Get system information" << std::endl;
            std::cout << "   POST /api/preferences - Update user preferences" << std::endl;
            std::cout << "   GET  /api/processes - Get active processes" << std::endl;
            std::cout << "   POST /api/rollback - Rollback last action" << std::endl;
            std::cout << "   GET  /api/suggestions - Get suggestions" << std::endl;
            std::cout << "\n💡 Press Ctrl+C to stop the server" << std::endl;

            // Keep server running
            while (http_server.isRunning())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        else
        {
            std::cerr << "❌ Failed to start HTTP server" << std::endl;
        }
    }

    void runInteractiveMode()
    {
        while (true)
        {
            std::cout << "\n🤖 Enter your task (or ':quit' to exit): ";
            std::string user_input;
            std::getline(std::cin, user_input);

            if (user_input.empty())
                continue;

            // Check for common exit commands
            if (user_input == "exit" || user_input == "quit" || user_input == "q")
            {
                std::cout << "👋 Goodbye!" << std::endl; // "Session saved" removed
                // context_manager.saveSession(); // Removed
                break;
            }

            processUserInput(user_input);
        }
    }
};

int main()
{
    try
    {
        AdvancedAIAgent agent;
        agent.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
