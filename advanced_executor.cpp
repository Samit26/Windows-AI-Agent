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
        {"require_confirmation_for_deletion", true}
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
