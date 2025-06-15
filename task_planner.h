#ifndef TASK_PLANNER_H
#define TASK_PLANNER_H

#include "include/json.hpp"
#include <string>
#include <vector>
#include <functional>

using json = nlohmann::json;

enum class TaskStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct Task {
    std::string id;
    std::string description;
    std::vector<std::string> commands;
    TaskStatus status;
    std::string error_message;
    double confidence_score;
    std::string created_at;
    std::string completed_at;
    json metadata;
};

struct TaskPlan {
    std::string plan_id;
    std::string objective;
    std::vector<Task> tasks;
    TaskStatus overall_status;
    double overall_confidence;
};

class TaskPlanner {
private:
    std::vector<TaskPlan> active_plans;
    json task_templates;
    
    std::string generateTaskId();
    std::string generatePlanId();
    TaskPlan createComplexPlan(const std::string& objective, const json& context);
    
public:
    TaskPlanner();
    
    // Planning methods
    TaskPlan planTask(const std::string& user_request, const json& context);
    std::vector<Task> breakDownComplexTask(const std::string& task_description);
    
    // Execution methods
    bool executeTask(Task& task);
    bool executePlan(TaskPlan& plan);
    
    // Monitoring methods
    TaskStatus getTaskStatus(const std::string& task_id);
    TaskStatus getPlanStatus(const std::string& plan_id);
    std::vector<Task> getFailedTasks();
    
    // Learning methods
    void updateTaskTemplate(const std::string& task_type, const json& template_data);
    void learnFromExecution(const Task& task, bool success);
    
    // Utility methods
    json getExecutionSummary();
    void cleanupCompletedTasks();
    std::vector<std::string> suggestAlternatives(const std::string& failed_task);
};

#endif // TASK_PLANNER_H
