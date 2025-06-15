#ifndef ADVANCED_EXECUTOR_H
#define ADVANCED_EXECUTOR_H

#include "include/json.hpp"
#include "task_planner.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

using json = nlohmann::json;

enum class ExecutionMode {
    SAFE,           // Only safe, reversible operations
    INTERACTIVE,    // Ask for confirmation on risky operations  
    AUTONOMOUS     // Full autonomous execution
};

struct ExecutionResult {
    bool success;
    std::string output;
    std::string error_message;
    double execution_time;
    json metadata;
};

class AdvancedExecutor {
private:
    ExecutionMode current_mode;
    std::map<std::string, std::function<ExecutionResult(const json&)>> command_handlers;
    json safety_rules;
    std::vector<std::string> dangerous_commands;
    
    bool isCommandSafe(const std::string& command);
    bool requiresConfirmation(const std::string& command);
    ExecutionResult executeWindowsCommand(const std::string& command);
    ExecutionResult executePowerShellScript(const std::vector<std::string>& commands);
    
public:
    AdvancedExecutor();
    
    // Core execution methods
    ExecutionResult execute(const json& task_data);
    ExecutionResult executeWithPlan(const TaskPlan& plan);
    
    // Specialized execution methods
    ExecutionResult executeFileOperation(const json& operation);
    ExecutionResult executeSystemCommand(const json& command);
    ExecutionResult executeWebAction(const json& action);
    ExecutionResult executeApplicationAction(const json& action);
    
    // Safety and control methods
    void setExecutionMode(ExecutionMode mode);
    ExecutionMode getExecutionMode();
    bool addSafetyRule(const std::string& rule, const json& parameters);
    void enableSandboxMode(bool enabled);
    
    // Monitoring and feedback
    json getExecutionHistory();
    std::vector<std::string> getActiveProcesses();
    void rollbackLastAction();
    
    // Learning and adaptation
    void learnFromExecution(const json& task, const ExecutionResult& result);
    json getSuggestedImprovements();
    
    // Integration methods
    void registerCommandHandler(const std::string& command_type, 
                               std::function<ExecutionResult(const json&)> handler);
    ExecutionResult callExternalAPI(const std::string& api_name, const json& parameters);
};

#endif // ADVANCED_EXECUTOR_H
