#include "task_planner.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>
#include <iostream> // For std::cerr, std::cout
#include "ai_model.h" // For callLLMForTextGeneration (conceptual)

// Placeholder for callLLMForTextGeneration - this will be in ai_model.cpp
// For now, to make task_planner.cpp compile, we can add a stub here.
// std::string callLLMForTextGeneration(const std::string& api_key, const std::string& prompt) {
//    std::cout << "DEBUG: callLLMForTextGeneration called with prompt: " << prompt << std::endl;
//    return "Generated content based on: " + prompt;
// }


TaskPlanner::TaskPlanner(std::string key) : api_key(std::move(key)) {
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
TaskPlan TaskPlanner::planTask(const std::string& user_request, const json& llm_response_json) {
    TaskPlan plan;
    plan.plan_id = generatePlanId();
    plan.objective = user_request; // Original user request is the overall objective
    plan.overall_status = TaskStatus::PENDING;
    plan.overall_confidence = llm_response_json.value("confidence", 0.7); // Default confidence

    std::string plan_type = llm_response_json.value("type", "unknown");

    if (plan_type == "multi_step_plan") {
        if (llm_response_json.contains("steps") && llm_response_json["steps"].is_array()) {
            for (const auto& step_json : llm_response_json["steps"]) {
                processSinglePlanStep(step_json, plan, user_request);
            }
        } else {
            std::cerr << "TaskPlanner::planTask: multi_step_plan is missing 'steps' array or it's not an array." << std::endl;
            // Create a failed task to indicate planning issue
            Task failed_task;
            failed_task.id = generateTaskId();
            failed_task.description = "Planning failed: multi_step_plan malformed.";
            failed_task.status = TaskStatus::FAILED;
            failed_task.error_message = "multi_step_plan missing or invalid 'steps'.";
            plan.tasks.push_back(failed_task);
            plan.overall_status = TaskStatus::FAILED;
        }
    } else if (plan_type == "generate_content_and_execute" ||
               plan_type == "powershell_script" ||
               plan_type == "vision_task")
    {
        processSinglePlanStep(llm_response_json, plan, user_request);
    } else if (plan_type == "text") { // If LLM directly returns a text response
         Task text_response_task;
         text_response_task.id = generateTaskId();
         text_response_task.description = "Simple text response from AI.";
         // We store the pure text content in commands[0] for this special case,
         // AdvancedExecutor would need to know how to handle this if it were to execute it.
         // Or, more likely, this type of response is handled directly by the caller of planTask.
         // For now, we'll package it as a task.
         text_response_task.commands.push_back(llm_response_json.value("content", ""));
         text_response_task.metadata["type"] = "text_response"; // Special metadata type
         text_response_task.status = TaskStatus::COMPLETED; // It's just text, so "completed"
         text_response_task.confidence_score = llm_response_json.value("confidence", 1.0);
         plan.tasks.push_back(text_response_task);
    }
    else {
        std::cerr << "TaskPlanner::planTask: Unknown or unsupported plan type: " << plan_type << std::endl;
        Task failed_task;
        failed_task.id = generateTaskId();
        failed_task.description = "Planning failed: Unknown plan type from LLM.";
        failed_task.status = TaskStatus::FAILED;
        failed_task.error_message = "Unknown plan type: " + plan_type;
        plan.tasks.push_back(failed_task);
        plan.overall_status = TaskStatus::FAILED;
    }
    
    active_plans.push_back(plan);
    return plan;
}

void TaskPlanner::processSinglePlanStep(const json& step_json, TaskPlan& current_plan, const std::string& original_request) {
    Task task;
    task.id = generateTaskId();
    task.status = TaskStatus::PENDING;
    task.confidence_score = step_json.value("confidence", 0.7);
    task.description = step_json.value("explanation", original_request); // Fallback to original request if no explanation

    auto now = std::chrono::system_clock::now();
    auto time_t_val = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss_time;
    ss_time << std::put_time(std::localtime(&time_t_val), "%Y-%m-%d %H:%M:%S");
    task.created_at = ss_time.str();

    std::string step_type = step_json.value("type", "unknown");
    task.metadata["original_type"] = step_type;


    if (step_type == "powershell_script") {
        if (step_json.contains("script") && step_json["script"].is_array()) {
            for (const auto& cmd : step_json["script"]) {
                task.commands.push_back(cmd.get<std::string>());
            }
        } else {
            task.status = TaskStatus::FAILED;
            task.error_message = "Powershell script step missing 'script' array.";
            std::cerr << "TaskPlanner::processSinglePlanStep: " << task.error_message << std::endl;
        }
        // For AdvancedExecutor, description can be the explanation for PowerShell tasks
        task.description = step_json.value("explanation", "Execute PowerShell script");
        task.metadata["task_type_for_executor"] = "powershell_script";
    }
    else if (step_type == "vision_task") {
        // For vision tasks, the 'objective' is key.
        // AdvancedExecutor's executeVisionTask takes a JSON. We'll pass the whole step_json.
        // The `commands` field in Task struct might not be directly used.
        // Description can be the objective.
        task.description = step_json.value("objective", original_request);
        // Store the full JSON for the vision task in metadata or a dedicated field if Task struct changes.
        // For now, let's assume AdvancedExecutor can receive this via metadata or task_data.
        // We will put the whole step_json into commands[0] as a serialized string for now.
        // This is a temporary way to pass complex data until Task struct or executor is refactored.
        task.commands.push_back(step_json.dump());
        task.metadata["task_type_for_executor"] = "vision_task";
    }
    else if (step_type == "generate_content_and_execute") {
        std::string gen_prompt = step_json.value("content_generation_prompt", "");
        if (gen_prompt.empty()) {
            task.status = TaskStatus::FAILED;
            task.error_message = "Content generation step missing 'content_generation_prompt'.";
            std::cerr << "TaskPlanner::processSinglePlanStep: " << task.error_message << std::endl;
            current_plan.tasks.push_back(task);
            return;
        }

        std::cout << "TaskPlanner: Requesting content generation for prompt: " << gen_prompt << std::endl;
        // Conceptual call: This function needs to be implemented in ai_model.cpp
        std::string generated_content = callLLMForTextGeneration(this->api_key, gen_prompt);

        if (generated_content.empty() || generated_content.rfind("Error generating text:", 0) == 0) {
            task.status = TaskStatus::FAILED;
            task.error_message = "Failed to generate content: " + generated_content;
            std::cerr << "TaskPlanner::processSinglePlanStep: " << task.error_message << std::endl;
            current_plan.tasks.push_back(task);
            return;
        }
        std::cout << "TaskPlanner: Content generated: " << generated_content.substr(0, 50) << "..." << std::endl;

        json subsequent_action_json = step_json.value("subsequent_action", json::object());
        if (subsequent_action_json.empty() || !subsequent_action_json.contains("type")) {
            task.status = TaskStatus::FAILED;
            task.error_message = "Generate content step missing valid 'subsequent_action'.";
            std::cerr << "TaskPlanner::processSinglePlanStep: " << task.error_message << std::endl;
            current_plan.tasks.push_back(task);
            return;
        }

        // Modify the subsequent_action JSON to include the generated content.
        // This is crucial for the executor.
        std::string sub_action_type = subsequent_action_json["type"].get<std::string>();
        if (sub_action_type == "vision_task") {
            // Inject generated content, e.g., as text_to_type or part of a modified objective
            // A common way is to have a placeholder in the objective like "{{generated_content}}"
            // or add a specific field like "text_to_type".
            // For this example, let's assume the vision_task can take "text_to_type".
            subsequent_action_json["text_to_type"] = generated_content;
            task.description = subsequent_action_json.value("objective", "Execute vision task with generated content");
            task.commands.push_back(subsequent_action_json.dump());
            task.metadata["task_type_for_executor"] = "vision_task";

        } else if (sub_action_type == "powershell_script") {
            // This is less common for generated text, but possible.
            // The script itself would need to be designed to use this content.
            // For example, `echo '${generated_content}' > file.txt`
            // This requires careful templating of commands.
            // For now, just pass it in metadata, executor needs to handle it.
            task.metadata["generated_content"] = generated_content;
            if (subsequent_action_json.contains("script") && subsequent_action_json["script"].is_array()) {
                 for (const auto& cmd_template : subsequent_action_json["script"]) {
                    std::string cmd = cmd_template.get<std::string>();
                    // Basic templating, replace {{generated_content}} - more robust solution needed for production
                    size_t pos = cmd.find("{{generated_content}}");
                    if (pos != std::string::npos) {
                        cmd.replace(pos, std::string("{{generated_content}}").length(), generated_content);
                    }
                    task.commands.push_back(cmd);
                }
            }
            task.description = subsequent_action_json.value("explanation", "Execute PowerShell with generated content");
            task.metadata["task_type_for_executor"] = "powershell_script";
        } else {
            task.status = TaskStatus::FAILED;
            task.error_message = "Unsupported subsequent_action type: " + sub_action_type;
            std::cerr << "TaskPlanner::processSinglePlanStep: " << task.error_message << std::endl;
            current_plan.tasks.push_back(task);
            return;
        }
        // The explanation for the task should come from the generate_content_and_execute step.
        task.description = step_json.value("explanation", task.description);
    }
    else {
        task.status = TaskStatus::FAILED;
        task.error_message = "Unknown step type in plan: " + step_type;
        std::cerr << "TaskPlanner::processSinglePlanStep: " << task.error_message << std::endl;
    }
    current_plan.tasks.push_back(task);
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
