#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "include/json.hpp"
#include "gemini.h"
#include "context_manager.h"
#include "task_planner.h"
#include "advanced_executor.h"
#include "multimodal_handler.h"
#include "http_server.h"

using json = nlohmann::json;

class AdvancedAIAgent {
private:
    std::string api_key;
    ContextManager context_manager;
    TaskPlanner task_planner;
    AdvancedExecutor advanced_executor;
    MultiModalHandler multimodal_handler;
    HttpServer http_server;
    bool interactive_mode;
    bool learning_enabled;
    bool server_mode;
    
public:
    AdvancedAIAgent() : interactive_mode(true), learning_enabled(true), server_mode(false), http_server(8080) {
        loadConfiguration();
        displayWelcomeMessage();
    }
    
    void loadConfiguration() {
        std::ifstream config_file("config.json");
        if(!config_file.is_open()) {
            std::cerr << "Could not open config.json" << std::endl;
            exit(1);
        }
        json config;
        config_file >> config;
        api_key = config["api_key"].get<std::string>();
        
        // Load advanced settings if available
        if (config.contains("execution_mode")) {
            std::string mode = config["execution_mode"];
            if (mode == "safe") advanced_executor.setExecutionMode(ExecutionMode::SAFE);
            else if (mode == "interactive") advanced_executor.setExecutionMode(ExecutionMode::INTERACTIVE);
            else if (mode == "autonomous") advanced_executor.setExecutionMode(ExecutionMode::AUTONOMOUS);
        }
        
        if (config.contains("enable_voice")) {
            bool voice_enabled = config["enable_voice"];
            if (voice_enabled) {
                multimodal_handler.enableVoiceInput();
            }
        }
          if (config.contains("enable_image_analysis")) {
            bool image_enabled = config["enable_image_analysis"];
            if (image_enabled) {
                multimodal_handler.enableImageAnalysis();
            }
        }
        
        // Check for server mode
        if (config.contains("server_mode")) {
            server_mode = config["server_mode"];
        }
        
        // Configure HTTP server if needed
        if (server_mode) {
            http_server.setComponents(&advanced_executor, &context_manager, 
                                    &task_planner, &multimodal_handler, api_key);
        }
    }
      void displayWelcomeMessage() {
        std::cout << "========================================" << std::endl;
        std::cout << "🤖 ADVANCED WINDOWS AI AGENT v2.0 🤖" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Features:" << std::endl;
        std::cout << "✓ Context Memory & Learning" << std::endl;
        std::cout << "✓ Advanced Task Planning" << std::endl;
        std::cout << "✓ Multi-Modal Input Support" << std::endl;
        std::cout << "✓ Safe Execution Environment" << std::endl;
        std::cout << "✓ Session Management" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  Type your request normally" << std::endl;
        std::cout << "  ':voice' - Enable voice input" << std::endl;
        std::cout << "  ':screenshot' - Analyze current screen" << std::endl;
        std::cout << "  ':history' - Show conversation history" << std::endl;
        std::cout << "  ':mode safe/interactive/autonomous' - Change execution mode" << std::endl;
        std::cout << "  ':quit', 'quit', 'exit', or 'q' - Exit the application" << std::endl;
        std::cout << "========================================" << std::endl;
    }
      void processUserInput(const std::string& input) {
        // Handle special commands
        if (input.length() > 0 && input[0] == ':') {
            handleSpecialCommand(input);
            return;
        }
        
        try {
            // Process input through context manager
            std::string contextual_prompt = context_manager.getContextualPrompt(input);
            
            // Call Gemini API with enhanced context
            json response = callGemini(api_key, contextual_prompt);
            
            if (response.contains("type") && response["type"] == "powershell_script") {
                // Create task plan
                TaskPlan plan = task_planner.planTask(input, response);
                
                // Display plan to user
                displayTaskPlan(plan);
                
                // Ask for confirmation if in interactive mode
                bool proceed = true;
                if (advanced_executor.getExecutionMode() == ExecutionMode::INTERACTIVE) {
                    proceed = askForConfirmation(response);
                }
                
                if (proceed) {
                    // Execute the plan
                    ExecutionResult result = advanced_executor.executeWithPlan(plan);
                    
                    // Update context with results
                    context_manager.addToHistory(input, 
                                               response.value("explanation", "Executed script"),
                                               response.dump(), 
                                               result.success);
                    
                    // Display results
                    displayExecutionResult(result);
                    
                    // Learn from execution if enabled
                    if (learning_enabled) {
                        advanced_executor.learnFromExecution(response, result);
                    }
                } else {
                    std::cout << "🚫 Execution cancelled by user." << std::endl;
                }
            } else {
                // Handle non-script responses
                std::cout << "💬 AI Response: " << response.value("content", "No valid response") << std::endl;
                context_manager.addToHistory(input, response.dump(), "text_response", true);
            }
            
        } catch(const std::exception &e) {
            std::cerr << "❌ Error: " << e.what() << std::endl;
            context_manager.addToHistory(input, "", "error", false);
        }
    }
    
    void handleSpecialCommand(const std::string& command) {
        if (command == ":voice") {
            std::cout << "🎤 Starting voice input..." << std::endl;
            std::string audio_file = multimodal_handler.startVoiceRecording();
            std::cout << "Press Enter when done speaking..." << std::endl;
            std::cin.get();
            multimodal_handler.stopVoiceRecording();
            
            MultiModalInput voice_input = multimodal_handler.processVoiceInput(audio_file);
            std::cout << "🎤 Transcribed: " << voice_input.content << std::endl;
            processUserInput(voice_input.content);
            
        } else if (command == ":screenshot") {
            std::cout << "📸 Analyzing current screen..." << std::endl;
            MultiModalInput screen_input = multimodal_handler.processScreenCapture();
            std::string analysis = multimodal_handler.analyzeScreenshot();
            std::cout << "👁️ Screen Analysis: " << analysis << std::endl;
            
        } else if (command == ":history") {
            std::cout << "📜 Recent conversation history:" << std::endl;
            std::cout << context_manager.getRecentContext(5) << std::endl;
              } else if (command.length() > 6 && command.substr(0, 6) == ":mode ") {
            std::string mode = command.substr(6);
            if (mode == "safe") {
                advanced_executor.setExecutionMode(ExecutionMode::SAFE);
                std::cout << "🛡️ Execution mode set to SAFE" << std::endl;
            } else if (mode == "interactive") {
                advanced_executor.setExecutionMode(ExecutionMode::INTERACTIVE);
                std::cout << "🤝 Execution mode set to INTERACTIVE" << std::endl;
            } else if (mode == "autonomous") {
                advanced_executor.setExecutionMode(ExecutionMode::AUTONOMOUS);
                std::cout << "🚀 Execution mode set to AUTONOMOUS" << std::endl;
            }
            
        } else if (command == ":quit") {
            std::cout << "👋 Goodbye! Session saved." << std::endl;
            context_manager.saveSession();
            exit(0);
        } else {
            std::cout << "❓ Unknown command: " << command << std::endl;
        }
    }
    
    void displayTaskPlan(const TaskPlan& plan) {
        std::cout << "📋 Task Plan: " << plan.objective << std::endl;
        std::cout << "🎯 Confidence: " << (plan.overall_confidence * 100) << "%" << std::endl;
        std::cout << "📝 Steps:" << std::endl;
        for (size_t i = 0; i < plan.tasks.size(); i++) {
            const auto& task = plan.tasks[i];
            std::cout << "  " << (i+1) << ". " << task.description << std::endl;
            for (const auto& cmd : task.commands) {
                std::cout << "     → " << cmd << std::endl;
            }
        }
    }
    
    bool askForConfirmation(const json& response) {
        std::cout << "⚠️ Execution Plan:" << std::endl;
        std::cout << "📝 Description: " << response.value("explanation", "No description") << std::endl;
        std::cout << "🎯 Confidence: " << (response.value("confidence", 0.0) * 100) << "%" << std::endl;
        
        if (response.contains("script")) {
            std::cout << "💻 Commands to execute:" << std::endl;
            for (const auto& cmd : response["script"]) {
                std::cout << "  → " << cmd.get<std::string>() << std::endl;
            }
        }
        
        std::cout << "🤔 Do you want to proceed? (y/N): ";
        std::string confirmation;
        std::getline(std::cin, confirmation);
        return (confirmation == "y" || confirmation == "Y" || confirmation == "yes");
    }
    
    void displayExecutionResult(const ExecutionResult& result) {
        if (result.success) {
            std::cout << "✅ Execution successful!" << std::endl;
            if (!result.output.empty()) {
                std::cout << "📤 Output: " << result.output << std::endl;
            }
        } else {
            std::cout << "❌ Execution failed!" << std::endl;
            std::cout << "⚠️ Error: " << result.error_message << std::endl;
        }
        std::cout << "⏱️ Execution time: " << result.execution_time << "s" << std::endl;
    }      void run() {
        if (server_mode) {
            runServerMode();
        } else {
            runInteractiveMode();
        }
    }
    
    void runServerMode() {
        std::cout << "\n🌐 Starting HTTP Server Mode..." << std::endl;
        
        if (http_server.start()) {
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
            while (http_server.isRunning()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } else {
            std::cerr << "❌ Failed to start HTTP server" << std::endl;
        }
    }
    
    void runInteractiveMode() {
        while (true) {
            std::cout << "\n🤖 Enter your task (or ':quit' to exit): ";
            std::string user_input;
            std::getline(std::cin, user_input);
            
            if (user_input.empty()) continue;
            
            // Check for common exit commands
            if (user_input == "exit" || user_input == "quit" || user_input == "q") {
                std::cout << "👋 Goodbye! Session saved." << std::endl;
                context_manager.saveSession();
                break;
            }
            
            processUserInput(user_input);
        }
    }
};

int main() {
    try {
        AdvancedAIAgent agent;
        agent.run();
    } catch(const std::exception &e) {
        std::cerr << "❌ Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
