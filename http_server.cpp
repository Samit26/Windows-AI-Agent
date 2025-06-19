#include "http_server.h"
#include "ai_model.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include "vision_processor.h" // Added for ScreenAnalysis, UIElement
// httplib.h removed - not available

// Always include winsock2.h before windows.h or any other socket headers
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_ // Prevent inclusion of winsock.h
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

HttpServer::HttpServer(int port) : port(port), running(false),
                                   executor(nullptr),
                                   task_planner(nullptr), multimodal_handler(nullptr),
                                   vision_processor_ptr(nullptr), advanced_executor_ptr(nullptr)
{
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

HttpServer::~HttpServer()
{
    stop();
    WSACleanup();
}

void HttpServer::setComponents(AdvancedExecutor *adv_exec,
                               TaskPlanner *tp_planner, MultiModalHandler *mm_handler,
                               VisionProcessor *vp, const std::string &key)
{
    executor = adv_exec;              // Keep using the existing 'executor' member for AdvancedExecutor
    advanced_executor_ptr = adv_exec; // Explicitly assign to the new pointer as well if needed, or just use 'executor'
    task_planner = tp_planner;
    multimodal_handler = mm_handler;
    vision_processor_ptr = vp;
    api_key = key;
}

bool HttpServer::start()
{
    if (running.load())
    {
        return false; // Already running
    }

    running.store(true);

    server_thread = std::thread([this]()
                                {
        SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket" << std::endl;
            running.store(false);
            return;
        }
        
        // Set socket options
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Failed to bind socket to port " << port << std::endl;
            closesocket(server_socket);
            running.store(false);
            return;
        }
        
        if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Failed to listen on socket" << std::endl;
            closesocket(server_socket);
            running.store(false);
            return;
        }
        
        std::cout << "🌐 HTTP Server started on port " << port << std::endl;
        
        while (running.load()) {
            sockaddr_in client_addr;
            int client_size = sizeof(client_addr);
            SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_size);
            
            if (client_socket == INVALID_SOCKET) {
                if (running.load()) {
                    std::cerr << "Failed to accept client connection" << std::endl;
                }
                continue;
            }
            
            // Handle request in a separate thread
            std::thread([this, client_socket]() {
                char buffer[4096];
                int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    std::string raw_request(buffer);
                    
                    HttpRequest request;
                    parseRequest(raw_request, request);
                    
                    HttpResponse response;
                    handleRequest(request, response);
                    
                    std::string response_str = buildResponse(response);
                    send(client_socket, response_str.c_str(), response_str.length(), 0);
                }
                
                closesocket(client_socket);
            }).detach();
        }
        
        closesocket(server_socket); });

    // Wait a moment to ensure server started
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return running.load();
}

void HttpServer::stop()
{
    if (running.load())
    {
        running.store(false);
        if (server_thread.joinable())
        {
            server_thread.join();
        }
        std::cout << "🌐 HTTP Server stopped" << std::endl;
    }
}

bool HttpServer::isRunning() const
{
    return running.load();
}

void HttpServer::parseRequest(const std::string &raw_request, HttpRequest &request)
{
    std::istringstream iss(raw_request);
    std::string line;

    // Parse request line
    if (std::getline(iss, line))
    {
        std::istringstream line_stream(line);
        std::string path_and_query;
        line_stream >> request.method >> path_and_query;

        // Parse path and query parameters
        size_t query_pos = path_and_query.find('?');
        if (query_pos != std::string::npos)
        {
            request.path = path_and_query.substr(0, query_pos);
            std::string query = path_and_query.substr(query_pos + 1);
            request.query_params = parseQueryString(query);
        }
        else
        {
            request.path = path_and_query;
        }
    }

    // Parse headers
    while (std::getline(iss, line) && line != "\r")
    {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 2); // Skip ": "
            if (!value.empty() && value.back() == '\r')
            {
                value.pop_back();
            }
            request.headers[key] = value;
        }
    }

    // Parse body
    std::string body_line;
    while (std::getline(iss, body_line))
    {
        request.body += body_line;
    }
}

std::string HttpServer::buildResponse(const HttpResponse &response)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << response.status_code << " OK\r\n";

    for (const auto &header : response.headers)
    {
        oss << header.first << ": " << header.second << "\r\n";
    }

    oss << "Content-Length: " << response.body.length() << "\r\n";
    oss << "\r\n";
    oss << response.body;

    return oss.str();
}

void HttpServer::handleRequest(const HttpRequest &request, HttpResponse &response)
{
    // Handle CORS preflight
    if (request.method == "OPTIONS")
    {
        response.status_code = 200;
        response.body = "";
        return;
    }

    try
    {
        json request_data;
        if (!request.body.empty())
        {
            request_data = json::parse(request.body);
        }

        // Route requests
        if (request.path == "/api/execute" && request.method == "POST")
        {
            handleExecuteTask(request_data, response);
        }
        else if (request.path == "/api/history" && request.method == "GET")
        {
            handleGetHistory(request_data, response);
        }
        else if (request.path == "/api/system-info" && request.method == "GET")
        {
            handleGetSystemInfo(request_data, response);
        }
        else if (request.path == "/api/preferences" && request.method == "POST")
        {
            handleUpdatePreferences(request_data, response);
        }
        else if (request.path == "/api/processes" && request.method == "GET")
        {
            handleGetActiveProcesses(request_data, response);
        }
        else if (request.path == "/api/rollback" && request.method == "POST")
        {
            handleRollback(request_data, response);
        }
        else if (request.path == "/api/suggestions" && request.method == "GET")
        {
            handleGetSuggestions(request_data, response);
        }
        else if (request.path == "/api/voice" && request.method == "POST")
        {
            handleVoiceInput(request_data, response);
        }
        else if (request.path == "/api/image" && request.method == "POST")
        {
            handleImageInput(request_data, response);
        }
        // New Vision Endpoints
        else if (request.path == "/api/vision/analyzeScreen" && request.method == "POST") // POST as it might trigger actions
        {
            // For httplib, the parameters are directly passed to the handler
            // This generic handleRequest might need adjustment if using httplib directly for routes
            // For now, assuming the structure where handleRequest dispatches based on path
            // If using httplib's server.Post, this function's role changes or these handlers are called directly.
            // Let's assume these will be adapted to be called directly by httplib routes.
            // So, the call would look like: handleApiVisionAnalyzeScreen(req, res);
            // This function (handleRequest) might not be the one registering routes if httplib is used as typical.
            // The prompt implies direct registration in start() or constructor.
            // For now, this part is more conceptual for where the new path would be handled.
            json empty_req_data_for_now; // analyzeScreen might not need a body
            HttpResponse vision_res;
            // This is a conceptual call, actual call will be via httplib::Request, httplib::Response
            // handleApiVisionAnalyzeScreen(request, vision_res);
            // response = vision_res; // Assign to outer response
            response.status_code = 501; // Not Implemented Yet via this dispatcher
            response.body = R"({"error": "Vision analyzeScreen not yet routed correctly in generic handler"})";
        }
        else if (request.path == "/api/vision/executeAction" && request.method == "POST")
        {
            HttpResponse vision_res;
            // handleApiVisionExecuteAction(request, vision_res);
            // response = vision_res;
            response.status_code = 501; // Not Implemented Yet
            response.body = R"({"error": "Vision executeAction not yet routed correctly in generic handler"})";
        }
        else
        {
            response.status_code = 404;
            response.body = R"({"error": "Endpoint not found"})";
        }
    }
    catch (const json::exception &e)
    {
        response.status_code = 400;
        response.body = R"({"error": "Invalid JSON in request: "})" + std::string(e.what());
    }
    catch (const std::exception &e)
    {
        response.status_code = 500;
        response.body = R"({"error": "Internal server error: "})" + std::string(e.what());
    }
}

void HttpServer::handleExecuteTask(const json &request_data, HttpResponse &response)
{
    try
    {
        std::string user_input = request_data.at("input").get<std::string>();
        if (user_input.empty())
        {
            response.status_code = 400;
            response.body = R"({"error": "Missing 'input' field"})";
            return;
        }

        std::string mode = request_data.value("mode", "agent");

        json result;

        if (mode == "chatbot")
        {
            std::string chatbot_prompt = "You are a helpful AI assistant. The user will ask you questions or make requests. "
                                         "Respond conversationally and helpfully, but do not provide executable commands or scripts. "
                                         "If the user asks you to perform a task that would require system access, explain what you would do "
                                         "but mention that you're in chatbot mode and cannot execute commands. "
                                         "Always be friendly, informative, and helpful.\n\n"
                                         "User: " +
                                         user_input;

            json ai_response = callAIModel(api_key, chatbot_prompt);
            result["response_type"] = "text";
            result["content"] = ai_response.value("content", "I understand your request, but I'm currently in chatbot mode. I can provide information and suggestions, but I cannot execute commands or perform system tasks. How else can I help you?");

            // Removed context_manager reference
            // context_manager->addToHistory(user_input, result["content"], "chatbot_response", true);
        }
        else
        {
            if (isVisionTask(user_input))
            {
                result = handleVisionTaskRequest(user_input);
            }
            else
            {
                json ai_response = callAIModel(api_key, user_input);
                result["response_type"] = "text";
                result["content"] = ai_response.value("content", "Unable to process your request.");
            }
        }

        response.status_code = 200;
        response.body = result.dump();
    }
    catch (const json::exception &e)
    {
        response.status_code = 400;
        response.body = R"({"error": "Invalid JSON format: "})" + std::string(e.what());
    }
    catch (const std::exception &e)
    {
        response.status_code = 500;
        response.body = R"({"error": "Internal server error: "})" + std::string(e.what());
    }
}

void HttpServer::handleGetHistory(const json &request_data, HttpResponse &response)
{
    json history_response;
    history_response["history"] = json::array(); // Placeholder
    history_response["session_id"] = "current_session";
    response.body = history_response.dump();
}

void HttpServer::handleGetSystemInfo(const json &request_data, HttpResponse &response)
{
    json system_info;
    system_info["execution_mode"] = static_cast<int>(executor->getExecutionMode());
    // Removed context_manager references
    system_info["system_state"] = "active";
    system_info["user_preferences"] = json::object();
    response.body = system_info.dump();
}

void HttpServer::handleUpdatePreferences(const json &request_data, HttpResponse &response)
{
    try
    {
        // Preferences update removed - no context manager
        response.body = R"({"success": true})";
    }
    catch (const std::exception &e)
    {
        response.status_code = 400;
        response.body = R"({"error": "Failed to update preferences"})";
    }
}

void HttpServer::handleGetActiveProcesses(const json &request_data, HttpResponse &response)
{
    json processes_response;
    processes_response["processes"] = executor->getActiveProcesses();
    response.body = processes_response.dump();
}

void HttpServer::handleRollback(const json &request_data, HttpResponse &response)
{
    executor->rollbackLastAction();
    response.body = R"({"success": true, "message": "Rollback initiated"})";
}

void HttpServer::handleGetSuggestions(const json &request_data, HttpResponse &response)
{
    json suggestions = executor->getSuggestedImprovements();
    response.body = suggestions.dump();
}

void HttpServer::handleVoiceInput(const json &request_data, HttpResponse &response)
{
    response.body = R"({"error": "Voice input not yet implemented"})";
}

void HttpServer::handleImageInput(const json &request_data, HttpResponse &response)
{
    response.body = R"({"error": "Image input not yet implemented"})";
}

std::string HttpServer::urlDecode(const std::string &str)
{
    std::string result;
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '%' && i + 2 < str.length())
        {
            int hex = std::stoi(str.substr(i + 1, 2), nullptr, 16);
            result += static_cast<char>(hex);
            i += 2;
        }
        else if (str[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += str[i];
        }
    }
    return result;
}

std::map<std::string, std::string> HttpServer::parseQueryString(const std::string &query)
{
    std::map<std::string, std::string> params;
    std::istringstream iss(query);
    std::string pair;

    while (std::getline(iss, pair, '&'))
    {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string key = urlDecode(pair.substr(0, eq_pos));
            std::string value = urlDecode(pair.substr(eq_pos + 1));
            params[key] = value;
        }
    }

    return params;
}

bool HttpServer::isVisionTask(const std::string &input)
{
    // Use AI to dynamically determine if this is a vision task
    try
    {
        json intent = callIntentAI(api_key, input);

        if (intent.contains("is_vision_task"))
        {
            bool is_vision = intent["is_vision_task"];
            double confidence = intent.value("confidence", 0.5);

            std::cout << "🤖 AI Intent Analysis (HTTP): " << (is_vision ? "Vision Task" : "Regular Task")
                      << " (confidence: " << (confidence * 100) << "%)" << std::endl;

            return is_vision;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "⚠️ AI intent analysis failed, falling back to keyword detection: " << e.what() << std::endl;
    }

    // Fallback to simplified keyword detection if AI fails
    std::vector<std::string> vision_keywords = {
        "click", "type", "send", "open", "message", "whatsapp", "change",
        "setting", "search", "find", "button", "window", "app", "application",
        "screenshot", "capture", "on screen", "in the", "select", "choose"};

    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

    for (const auto &keyword : vision_keywords)
    {
        if (lower_input.find(keyword) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

json HttpServer::handleVisionTaskRequest(const std::string &input)
{
    json result;

    try
    {
        std::cout << "🎯 Processing vision task via HTTP: " << input << std::endl;

        // Execute natural language task using vision
        ExecutionResult exec_result = executor->executeNaturalLanguageTask(input);

        result["response_type"] = "vision_task";
        result["success"] = exec_result.success;
        result["content"] = exec_result.output;
        result["execution_time"] = exec_result.execution_time;

        if (exec_result.metadata.contains("steps_executed"))
        {
            result["steps_executed"] = exec_result.metadata["steps_executed"];
        }

        if (exec_result.metadata.contains("step_details"))
        {
            result["step_details"] = exec_result.metadata["step_details"];
        }

        if (!exec_result.success)
        {
            result["error"] = exec_result.error_message;
        } // Add to context - removed context_manager
        // context_manager->addToHistory(input, exec_result.output, "vision_task", exec_result.success);

        std::cout << "✅ Vision task completed via HTTP" << std::endl;
    }
    catch (const std::exception &e)
    {
        result["response_type"] = "vision_task";
        result["success"] = false;
        result["error"] = "Vision task execution error: " + std::string(e.what());
        result["content"] = "Failed to execute vision task";

        std::cout << "❌ Vision task failed via HTTP: " << e.what() << std::endl;
    }

    return result;
}
