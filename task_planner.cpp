#include "task_planner.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>

TaskPlanner::TaskPlanner() {
    // Initialize with basic task templates
    task_templates = {
        {"file_operation", {
            {"confidence_base", 0.8},
            {"safety_level", "medium"},
            {"typical_commands", {"Copy-Item", "Move-Item", "New-Item", "Remove-Item"}}
        }},
        {"application_launch", {
            {"confidence_base", 0.9},
            {"safety_level", "high"},
            {"typical_commands", {"Start-Process"}}
        }},
        {"calculation", {
            {"confidence_base", 0.95},
            {"safety_level", "high"},
            {"typical_commands", {"Write-Host"}}
        }},
        {"web_browsing", {
            {"confidence_base", 0.85},
            {"safety_level", "high"},
            {"typical_commands", {"Start-Process"}}
        }}
    };
}

std::string TaskPlanner::generateTaskId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "task_" << std::put_time(std::localtime(&time_t), "%H%M%S") << "_" << dis(gen);
    return ss.str();
}

std::string TaskPlanner::generatePlanId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "plan_" << std::put_time(std::localtime(&time_t), "%H%M%S") << "_" << dis(gen);
    return ss.str();
}

// TODO: Unit Test: Add tests for planTask to verify correct TaskPlan creation from AI context.
TaskPlan TaskPlanner::planTask(const std::string& user_request, const json& context) {
    TaskPlan plan;
    plan.plan_id = generatePlanId();
    plan.objective = user_request;
    plan.overall_status = TaskStatus::PENDING;
    
    // Simple task for now - in a real implementation, this would use AI to break down complex tasks
    Task task;
    task.id = generateTaskId();
    task.description = user_request;
    task.status = TaskStatus::PENDING;
    
    // Extract commands from context if available
    if (context.contains("script")) {
        for (const auto& cmd : context["script"]) {
            task.commands.push_back(cmd.get<std::string>());
        }
    }
    
    // Set confidence score
    task.confidence_score = context.value("confidence", 0.7);
    plan.overall_confidence = task.confidence_score;
    
    // Set timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    task.created_at = ss.str();
    
    plan.tasks.push_back(task);
    active_plans.push_back(plan);
    
    return plan;
}

std::vector<Task> TaskPlanner::breakDownComplexTask(const std::string& task_description) {
    std::vector<Task> subtasks;
    
    // TODO: Placeholder Implementation
    // Expected: This function should take a complex task description and use an LLM call
    // (e.g., to a planning model or a general LLM with specific instructions)
    // to break it down into a sequence of smaller, manageable sub-tasks.
    // Each sub-task should be well-defined and potentially executable by other agent components.
    // The current implementation is a simplified stub.
    Task main_task;
    main_task.id = generateTaskId();
    main_task.description = task_description;
    main_task.status = TaskStatus::PENDING;
    main_task.confidence_score = 0.7;
    
    subtasks.push_back(main_task);
    return subtasks;
}

TaskStatus TaskPlanner::getTaskStatus(const std::string& task_id) {
    for (const auto& plan : active_plans) {
        for (const auto& task : plan.tasks) {
            if (task.id == task_id) {
                return task.status;
            }
        }
    }
    return TaskStatus::FAILED; // Task not found
}

TaskStatus TaskPlanner::getPlanStatus(const std::string& plan_id) {
    for (const auto& plan : active_plans) {
        if (plan.plan_id == plan_id) {
            return plan.overall_status;
        }
    }
    return TaskStatus::FAILED; // Plan not found
}

std::vector<Task> TaskPlanner::getFailedTasks() {
    std::vector<Task> failed_tasks;
    for (const auto& plan : active_plans) {
        for (const auto& task : plan.tasks) {
            if (task.status == TaskStatus::FAILED) {
                failed_tasks.push_back(task);
            }
        }
    }
    return failed_tasks;
}

void TaskPlanner::updateTaskTemplate(const std::string& task_type, const json& template_data) {
    task_templates[task_type] = template_data;
}

void TaskPlanner::learnFromExecution(const Task& task, bool success) {
    // Update task templates based on execution results
    // This is a simplified learning mechanism
    if (success) {
        // Increase confidence for similar task types
        for (auto& [type, template_data] : task_templates.items()) {
            if (template_data.contains("confidence_base")) {
                double current_confidence = template_data["confidence_base"];
                template_data["confidence_base"] = std::min(0.98, current_confidence + 0.01);
            }
        }
    }
}

json TaskPlanner::getExecutionSummary() {
    json summary;
    summary["total_plans"] = active_plans.size();
    
    int completed = 0, failed = 0, pending = 0;
    for (const auto& plan : active_plans) {
        switch (plan.overall_status) {
            case TaskStatus::COMPLETED: completed++; break;
            case TaskStatus::FAILED: failed++; break;
            case TaskStatus::PENDING: pending++; break;
            default: break;
        }
    }
    
    summary["completed_plans"] = completed;
    summary["failed_plans"] = failed;
    summary["pending_plans"] = pending;
    
    return summary;
}

void TaskPlanner::cleanupCompletedTasks() {
    // Remove completed plans older than 1 hour
    auto now = std::chrono::system_clock::now();
    active_plans.erase(
        std::remove_if(active_plans.begin(), active_plans.end(),
            [&](const TaskPlan& plan) {
                return plan.overall_status == TaskStatus::COMPLETED;
                // In a real implementation, would check timestamp
            }),
        active_plans.end()
    );
}

std::vector<std::string> TaskPlanner::suggestAlternatives(const std::string& failed_task) {
    std::vector<std::string> alternatives;
    
    // TODO: Placeholder Implementation
    // Expected: This function should analyze a failed task and suggest alternative
    // approaches or commands. This would ideally involve an LLM call, providing context
    // about the failed task and asking for troubleshooting steps or different methods.
    // The current implementation is a simple rule-based stub.
    if (failed_task.find("open") != std::string::npos) {
        alternatives.push_back("Try using Start-Process instead");
        alternatives.push_back("Check if the application is installed");
        alternatives.push_back("Use the full path to the executable");
    }
    
    if (failed_task.find("file") != std::string::npos) {
        alternatives.push_back("Check if the file exists");
        alternatives.push_back("Verify file permissions");
        alternatives.push_back("Use absolute file paths");
    }
    
    return alternatives;
}
