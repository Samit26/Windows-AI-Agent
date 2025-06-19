#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "include/json.hpp"
#include "advanced_executor.h"
#include "task_planner.h"
#include "multimodal_handler.h"
#include <string>
#include <thread>
#include <atomic>
#include <functional>

using json = nlohmann::json;

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
};

struct HttpResponse
{
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;

    HttpResponse(int code = 200) : status_code(code)
    {
        headers["Content-Type"] = "application/json";
        headers["Access-Control-Allow-Origin"] = "*";
        headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    }
};

class HttpServer
{
private:
    int port;
    std::atomic<bool> running;
    std::thread server_thread;
    // Backend components
    AdvancedExecutor *executor;
    TaskPlanner *task_planner;
    MultiModalHandler *multimodal_handler;
    VisionProcessor *vision_processor_ptr = nullptr;   // Added
    AdvancedExecutor *advanced_executor_ptr = nullptr; // For clarity, ensure it's present, though 'executor' might be it
    std::string api_key;

    // HTTP handling
    void handleRequest(const HttpRequest &request, HttpResponse &response);
    void parseRequest(const std::string &raw_request, HttpRequest &request);
    std::string buildResponse(const HttpResponse &response);

    // API endpoints
    void handleExecuteTask(const json &request_data, HttpResponse &response);
    void handleGetHistory(const json &request_data, HttpResponse &response);
    void handleGetSystemInfo(const json &request_data, HttpResponse &response);
    void handleUpdatePreferences(const json &request_data, HttpResponse &response);
    void handleGetActiveProcesses(const json &request_data, HttpResponse &response);
    void handleRollback(const json &request_data, HttpResponse &response);
    void handleGetSuggestions(const json &request_data, HttpResponse &response);
    void handleVoiceInput(const json &request_data, HttpResponse &response);
    void handleImageInput(const json &request_data, HttpResponse &response);

    // Vision task handling
    bool isVisionTask(const std::string &input);            // This seems more like a helper for general task execution
    json handleVisionTaskRequest(const std::string &input); // This seems more like a helper for general task execution
                                                            // New API Handlers for Vision - removed httplib references
    // void handleApiVisionAnalyzeScreen(...);
    // void handleApiVisionExecuteAction(...);

public:
    HttpServer(int port = 8080);
    ~HttpServer();

    // Configuration
    void setComponents(AdvancedExecutor *adv_exec,
                       TaskPlanner *planner, MultiModalHandler *mm_handler,
                       VisionProcessor *vp, const std::string &key);

    // Server control
    bool start();
    void stop();
    bool isRunning() const;

    // Utility
    static std::string urlDecode(const std::string &str);
    static std::map<std::string, std::string> parseQueryString(const std::string &query);
};

#endif // HTTP_SERVER_H
