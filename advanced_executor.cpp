#include "advanced_executor.h"
#include "executor.h" // Include the original executor
#include <chrono>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>

AdvancedExecutor::AdvancedExecutor() : current_mode(ExecutionMode::INTERACTIVE) {
    // Initialize safety rules
    safety_rules = {
        {"allow_file_operations", true},
        {"allow_network_access", true},
        {"allow_system_commands", false},
        {"require_confirmation_for_deletion", true},
        {"allow_vision_tasks", true}
    };
    
    // Define dangerous commands that require extra caution
    dangerous_commands = {
        "format", "del", "rm", "rmdir", "shutdown", "restart",
        "reg delete", "net user", "diskpart", "fdisk"
    };
    
    // Register default command handlers
    registerCommandHandler("powershell_script", 
        [this](const json& task_data) -> ExecutionResult {
            if (task_data.contains("script")) {
                std::vector<std::string> commands;
                for (const auto& cmd : task_data["script"]) {
                    commands.push_back(cmd.get<std::string>());
                }
                return executePowerShellScript(commands);
            }
            return {false, "", "No script provided", 0.0, json::object()};
        });
        
    // Register vision task handler
    registerCommandHandler("vision_task", 
        [this](const json& task_data) -> ExecutionResult {
            return executeVisionTask(task_data);
        });
        
    registerCommandHandler("ui_automation", 
        [this](const json& task_data) -> ExecutionResult {
            return executeUIAutomation(task_data);
        });
}

bool AdvancedExecutor::isCommandSafe(const std::string& command) {
    // Convert to lowercase for comparison
    std::string lower_cmd = command;
    std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
    
    // Check against dangerous commands
    for (const auto& dangerous : dangerous_commands) {
        if (lower_cmd.find(dangerous) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

bool AdvancedExecutor::requiresConfirmation(const std::string& command) {
    if (current_mode == ExecutionMode::AUTONOMOUS) {
        return false;
    }
    
    if (current_mode == ExecutionMode::SAFE) {
        return !isCommandSafe(command);
    }
    
    // Interactive mode - require confirmation for potentially dangerous operations
    std::string lower_cmd = command;
    std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
    
    std::vector<std::string> confirmation_triggers = {
        "delete", "remove", "del", "rm", "format", "shutdown", "restart",
        "registry", "reg ", "net user", "install", "uninstall"
    };
    
    for (const auto& trigger : confirmation_triggers) {
        if (lower_cmd.find(trigger) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

ExecutionResult AdvancedExecutor::executeWindowsCommand(const std::string& command) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ExecutionResult result;
    result.success = false;
    result.output = "";
    result.error_message = "";
    
    // Safety check
    if (!isCommandSafe(command) && current_mode == ExecutionMode::SAFE) {
        result.error_message = "Command blocked by safety rules: " + command;
        result.execution_time = 0.0;
        return result;
    }
    
    try {
        // For now, delegate to the original executor
        // In a full implementation, this would have more sophisticated execution
        std::filesystem::create_directories("scripts");
        std::ofstream script_file("scripts/temp_advanced.ps1");
        script_file << command << std::endl;
        script_file.close();
        
        std::string script_path = std::filesystem::absolute("scripts/temp_advanced.ps1").string();
        std::string full_command = "powershell.exe -ExecutionPolicy Bypass -File \"" + script_path + "\"";
        
        int exit_code = system(full_command.c_str());
        
        result.success = (exit_code == 0);
        if (!result.success) {
            result.error_message = "Command failed with exit code: " + std::to_string(exit_code);
        } else {
            result.output = "Command executed successfully";
        }
        
    } catch (const std::exception& e) {
        result.error_message = e.what();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result.execution_time = duration.count() / 1000.0;
    
    return result;
}

ExecutionResult AdvancedExecutor::executePowerShellScript(const std::vector<std::string>& commands) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ExecutionResult result;
    result.success = true;
    result.output = "";
    result.error_message = "";
    
    try {
        // Safety checks for all commands
        for (const auto& cmd : commands) {
            if (!isCommandSafe(cmd) && current_mode == ExecutionMode::SAFE) {
                result.success = false;
                result.error_message = "Command blocked by safety rules: " + cmd;
                return result;
            }
        }
        
        // Use the original executor function
        json script_array = json::array();
        for (const auto& cmd : commands) {
            script_array.push_back(cmd);
        }
        
        executeScript(script_array);
        result.output = "PowerShell script executed successfully";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result.execution_time = duration.count() / 1000.0;
    
    return result;
}

ExecutionResult AdvancedExecutor::execute(const json& task_data) {
    if (task_data.contains("type")) {
        std::string type = task_data["type"];
        if (command_handlers.find(type) != command_handlers.end()) {
            return command_handlers[type](task_data);
        }
    }
    
    return {false, "", "Unknown task type", 0.0, json::object()};
}

ExecutionResult AdvancedExecutor::executeWithPlan(const TaskPlan& plan) {
    ExecutionResult overall_result;
    overall_result.success = true;
    overall_result.output = "";
    overall_result.error_message = "";
    overall_result.execution_time = 0.0;
    
    for (const auto& task : plan.tasks) {
        json task_data = {
            {"type", "powershell_script"},
            {"script", task.commands}
        };
        
        ExecutionResult task_result = execute(task_data);
        overall_result.execution_time += task_result.execution_time;
        
        if (!task_result.success) {
            overall_result.success = false;
            overall_result.error_message += "Task '" + task.description + "' failed: " + task_result.error_message + "; ";
        } else {
            overall_result.output += task_result.output + "; ";
        }
    }
    
    return overall_result;
}

void AdvancedExecutor::setExecutionMode(ExecutionMode mode) {
    current_mode = mode;
}

ExecutionMode AdvancedExecutor::getExecutionMode() {
    return current_mode;
}

bool AdvancedExecutor::addSafetyRule(const std::string& rule, const json& parameters) {
    safety_rules[rule] = parameters;
    return true;
}

void AdvancedExecutor::registerCommandHandler(const std::string& command_type, 
                                             std::function<ExecutionResult(const json&)> handler) {
    command_handlers[command_type] = handler;
}

json AdvancedExecutor::getExecutionHistory() {
    // Placeholder implementation
    return json::object();
}

std::vector<std::string> AdvancedExecutor::getActiveProcesses() {
    // Placeholder implementation
    return std::vector<std::string>();
}

void AdvancedExecutor::rollbackLastAction() {
    // Placeholder implementation
    std::cout << "Rollback functionality not yet implemented" << std::endl;
}

void AdvancedExecutor::learnFromExecution(const json& task, const ExecutionResult& result) {
    // Placeholder implementation for learning
    // In a real system, this would update internal models based on success/failure patterns
}

json AdvancedExecutor::getSuggestedImprovements() {
    // Placeholder implementation
    return json::object();
}

ExecutionResult AdvancedExecutor::executeFileOperation(const json& operation) {
    // Placeholder implementation
    return {false, "", "File operations not yet implemented", 0.0, json::object()};
}

ExecutionResult AdvancedExecutor::executeSystemCommand(const json& command) {
    // Placeholder implementation
    return {false, "", "System commands not yet implemented", 0.0, json::object()};
}

ExecutionResult AdvancedExecutor::executeWebAction(const json& action) {
    // Placeholder implementation
    return {false, "", "Web actions not yet implemented", 0.0, json::object()};
}

ExecutionResult AdvancedExecutor::executeApplicationAction(const json& action) {
    // Placeholder implementation
    return {false, "", "Application actions not yet implemented", 0.0, json::object()};
}

void AdvancedExecutor::enableSandboxMode(bool enabled) {
    // Placeholder implementation
}

ExecutionResult AdvancedExecutor::callExternalAPI(const std::string& api_name, const json& parameters) {
    // Placeholder implementation
    return {false, "", "External API calls not yet implemented", 0.0, json::object()};
}

ExecutionResult AdvancedExecutor::executeVisionTask(const json& task_data) {
    auto start_time = std::chrono::high_resolution_clock::now();
    ExecutionResult result;
    
    try {
        if (!task_data.contains("task")) {
            result.success = false;
            result.error_message = "No task specified for vision execution";
            return result;
        }
          std::string task = task_data["task"].get<std::string>();
        
        // Safety check for vision tasks
        if (!isVisionTaskSafe(task)) {
            result.success = false;
            result.error_message = "Vision task rejected by safety rules";
            return result;
        }
        
        // Initialize vision executor if needed
        if (!vision_executor && !ai_api_key.empty()) {
            vision_executor = std::make_unique<VisionGuidedExecutor>(ai_api_key);
        }
        
        if (!vision_executor) {
            result.success = false;
            result.error_message = "Vision executor not available - API key required";
            return result;
        }
        
        // Execute vision task
        std::cout << "🎯 Executing vision task: " << task << std::endl;
        VisionTaskExecution execution = vision_executor->executeVisionTask(task);
        
        result.success = execution.overall_success;
        result.output = execution.final_result;
        
        if (!result.success) {
            result.error_message = "Vision task execution failed";
        }
        
        // Add execution metadata
        result.metadata = {
            {"task_type", "vision"},
            {"steps_executed", execution.steps.size()},
            {"total_time", execution.total_time},
            {"step_details", json::array()}
        };
        
        // Add step details
        for (const auto& step : execution.steps) {
            json step_json = {
                {"description", step.description},
                {"success", step.success},
                {"execution_time", step.execution_time}
            };
            if (!step.error_message.empty()) {
                step_json["error"] = step.error_message;
            }
            result.metadata["step_details"].push_back(step_json);
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Exception in vision task execution: " + std::string(e.what());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.execution_time = std::chrono::duration<double>(end_time - start_time).count();
    
    return result;
}

ExecutionResult AdvancedExecutor::executeUIAutomation(const json& automation_data) {
    auto start_time = std::chrono::high_resolution_clock::now();
    ExecutionResult result;
    
    try {        if (!automation_data.contains("actions")) {
            result.success = false;
            result.error_message = "No actions specified for UI automation";
            return result;
        }
        
        // Initialize vision executor if needed
        if (!vision_executor && !ai_api_key.empty()) {
            vision_executor = std::make_unique<VisionGuidedExecutor>(ai_api_key);
        }
        
        if (!vision_executor) {
            result.success = false;
            result.error_message = "Vision executor not available - API key required";
            return result;
        }
        
        // Execute each action
        json actions = automation_data["actions"];
        std::vector<json> results;
        bool all_success = true;
        
        for (const auto& action : actions) {
            std::string action_type = action.value("type", "unknown");
            std::string target = action.value("target", "");
            std::string value = action.value("value", "");
            
            json action_result = {
                {"action_type", action_type},
                {"target", target},
                {"success", false}
            };
            
            if (action_type == "click") {
                // Perform click action
                ScreenAnalysis screen = vision_executor->getCurrentScreenState();
                UIElement element = vision_executor->getVisionProcessor()->findElementByText(target, screen);
                bool success = vision_executor->getVisionProcessor()->clickElement(element);
                action_result["success"] = success;
                if (!success) all_success = false;
                
            } else if (action_type == "type") {
                // Perform type action
                ScreenAnalysis screen = vision_executor->getCurrentScreenState();
                UIElement element = vision_executor->getVisionProcessor()->findElementByText(target, screen);
                bool success = vision_executor->getVisionProcessor()->typeAtElement(element, value);
                action_result["success"] = success;
                if (!success) all_success = false;
            }
            
            results.push_back(action_result);
        }
        
        result.success = all_success;
        result.output = "UI automation completed with " + std::to_string(results.size()) + " actions";
        result.metadata = {
            {"automation_type", "ui"},
            {"actions_executed", results.size()},
            {"action_results", results}
        };
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Exception in UI automation: " + std::string(e.what());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.execution_time = std::chrono::duration<double>(end_time - start_time).count();
    
    return result;
}

bool AdvancedExecutor::isVisionTaskSafe(const std::string& task) {
    // Convert to lowercase for safety checking
    std::string lower_task = task;
    std::transform(lower_task.begin(), lower_task.end(), lower_task.begin(), ::tolower);
    
    // Block dangerous system operations
    std::vector<std::string> dangerous_keywords = {
        "delete", "format", "shutdown", "restart", "uninstall",
        "registry", "system32", "admin", "password"
    };
    
    for (const auto& keyword : dangerous_keywords) {
        if (lower_task.find(keyword) != std::string::npos) {
            std::cout << "⚠️  Vision task blocked due to dangerous keyword: " << keyword << std::endl;
            return false;
        }
    }
    
    return safety_rules.value("allow_vision_tasks", true);
}

ExecutionResult AdvancedExecutor::executeNaturalLanguageTask(const std::string& task) {
    json task_data = {
        {"task", task},
        {"type", "vision_task"}
    };
    
    return executeVisionTask(task_data);
}

void AdvancedExecutor::setAIApiKey(const std::string& api_key) {
    ai_api_key = api_key;
    
    // Initialize vision executor with the API key
    if (!api_key.empty()) {
        vision_executor = std::make_unique<VisionGuidedExecutor>(api_key);
        std::cout << "✅ Vision capabilities enabled with AI API" << std::endl;
    }
}
