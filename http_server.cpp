#include "http_server.h"
#include "gemini.h"
#include <iostream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

HttpServer::HttpServer(int port) : port(port), running(false), 
    executor(nullptr), context_manager(nullptr), task_planner(nullptr), multimodal_handler(nullptr) {
    
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

HttpServer::~HttpServer() {
    stop();
    WSACleanup();
}

void HttpServer::setComponents(AdvancedExecutor* exec, ContextManager* ctx, 
                              TaskPlanner* planner, MultiModalHandler* modal, 
                              const std::string& gemini_key) {
    executor = exec;
    context_manager = ctx;
    task_planner = planner;
    multimodal_handler = modal;
    api_key = gemini_key;
}

bool HttpServer::start() {
    if (running.load()) {
        return false; // Already running
    }
    
    running.store(true);
    
    server_thread = std::thread([this]() {
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
        
        closesocket(server_socket);
    });
    
    // Wait a moment to ensure server started
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return running.load();
}

void HttpServer::stop() {
    if (running.load()) {
        running.store(false);
        if (server_thread.joinable()) {
            server_thread.join();
        }
        std::cout << "🌐 HTTP Server stopped" << std::endl;
    }
}

bool HttpServer::isRunning() const {
    return running.load();
}

void HttpServer::parseRequest(const std::string& raw_request, HttpRequest& request) {
    std::istringstream iss(raw_request);
    std::string line;
    
    // Parse request line
    if (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string path_and_query;
        line_stream >> request.method >> path_and_query;
        
        // Parse path and query parameters
        size_t query_pos = path_and_query.find('?');
        if (query_pos != std::string::npos) {
            request.path = path_and_query.substr(0, query_pos);
            std::string query = path_and_query.substr(query_pos + 1);
            request.query_params = parseQueryString(query);
        } else {
            request.path = path_and_query;
        }
    }
    
    // Parse headers
    while (std::getline(iss, line) && line != "\r") {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 2); // Skip ": "
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            request.headers[key] = value;
        }
    }
    
    // Parse body
    std::string body_line;
    while (std::getline(iss, body_line)) {
        request.body += body_line;
    }
}

std::string HttpServer::buildResponse(const HttpResponse& response) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << response.status_code << " OK\r\n";
    
    for (const auto& header : response.headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    oss << "Content-Length: " << response.body.length() << "\r\n";
    oss << "\r\n";
    oss << response.body;
    
    return oss.str();
}

void HttpServer::handleRequest(const HttpRequest& request, HttpResponse& response) {
    // Handle CORS preflight
    if (request.method == "OPTIONS") {
        response.status_code = 200;
        response.body = "";
        return;
    }
    
    try {
        json request_data;
        if (!request.body.empty()) {
            request_data = json::parse(request.body);
        }
        
        // Route requests
        if (request.path == "/api/execute" && request.method == "POST") {
            handleExecuteTask(request_data, response);
        }
        else if (request.path == "/api/history" && request.method == "GET") {
            handleGetHistory(request_data, response);
        }
        else if (request.path == "/api/system-info" && request.method == "GET") {
            handleGetSystemInfo(request_data, response);
        }
        else if (request.path == "/api/preferences" && request.method == "POST") {
            handleUpdatePreferences(request_data, response);
        }
        else if (request.path == "/api/processes" && request.method == "GET") {
            handleGetActiveProcesses(request_data, response);
        }
        else if (request.path == "/api/rollback" && request.method == "POST") {
            handleRollback(request_data, response);
        }
        else if (request.path == "/api/suggestions" && request.method == "GET") {
            handleGetSuggestions(request_data, response);
        }
        else if (request.path == "/api/voice" && request.method == "POST") {
            handleVoiceInput(request_data, response);
        }
        else if (request.path == "/api/image" && request.method == "POST") {
            handleImageInput(request_data, response);
        }
        else {
            response.status_code = 404;
            response.body = R"({"error": "Endpoint not found"})";
        }
        
    } catch (const json::exception& e) {
        response.status_code = 400;
        response.body = R"({"error": "Invalid JSON in request"})";
    } catch (const std::exception& e) {
        response.status_code = 500;
        response.body = R"({"error": "Internal server error"})";
    }
}

void HttpServer::handleExecuteTask(const json& request_data, HttpResponse& response) {
    try {
        std::string user_input = request_data.value("input", "");
        if (user_input.empty()) {
            response.status_code = 400;
            response.body = R"({"error": "Missing 'input' field"})";
            return;
        }
        
        std::string mode = request_data.value("mode", "agent");
        
        json result;
        
        if (mode == "chatbot") {
            // In chatbot mode, use a different prompt that focuses on conversation
            std::string chatbot_prompt = "You are a helpful AI assistant. The user will ask you questions or make requests. "
                                       "Respond conversationally and helpfully, but do not provide executable commands or scripts. "
                                       "If the user asks you to perform a task that would require system access, explain what you would do "
                                       "but mention that you're in chatbot mode and cannot execute commands. "
                                       "Always be friendly, informative, and helpful.\n\n"
                                       "User: " + user_input;
            
            // Call Gemini API with chatbot-specific prompt
            json gemini_response = callGemini(api_key, chatbot_prompt);
            
            result["response_type"] = "text";
            result["content"] = gemini_response.value("content", "I understand your request, but I'm currently in chatbot mode. I can provide information and suggestions, but I cannot execute commands or perform system tasks. How else can I help you?");
            
            // Store in history as a text conversation
            context_manager->addToHistory(user_input, result["content"], "chatbot_response", true);
        } else {
            // Agent mode - existing functionality
            std::string contextual_prompt = context_manager->getContextualPrompt(user_input);
            json gemini_response = callGemini(api_key, contextual_prompt);
            
            result["gemini_response"] = gemini_response;
            
            if (gemini_response.contains("type") && gemini_response["type"] == "powershell_script") {
                bool auto_execute = request_data.value("auto_execute", false);
                
                // Check if the command involves dangerous operations
                bool isDangerous = false;
                if (gemini_response.contains("script")) {
                    for (const auto& cmd : gemini_response["script"]) {
                        std::string command = cmd.get<std::string>();
                        std::string lower_cmd = command;
                        std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
                        
                        // Check for dangerous operations
                        std::vector<std::string> dangerous_operations = {
                            "shutdown", "restart", "reboot", "format", "delete", "remove-item", 
                            "del ", "rmdir", "rm ", "net user", "reg delete", "diskpart", 
                            "fdisk", "cipher", "sdelete", "takeown", "icacls"
                        };
                        
                        for (const auto& dangerous : dangerous_operations) {
                            if (lower_cmd.find(dangerous) != std::string::npos) {
                                isDangerous = true;
                                break;
                            }
                        }
                        if (isDangerous) break;
                    }
                }
                
                TaskPlan plan = task_planner->planTask(user_input, gemini_response);
                
                // Generate conversational response
                std::vector<std::string> confirmationMessages = {
                    "Perfect! I understand what you need. Should I go ahead and execute this for you?",
                    "Got it! I can handle that task. Would you like me to proceed?", 
                    "I know exactly what to do! Ready to execute when you give me the green light.",
                    "Understood! I've prepared everything. Shall I take care of this now?",
                    "Great request! I'm ready to handle this task. Just give me confirmation to proceed.",
                    "I've got this covered! Should I start working on it right away?",
                    "Perfect timing! I can execute this task for you. Ready when you are!",
                    "Excellent! I've analyzed your request and I'm prepared to help. Proceed?"
                };
                
                srand(static_cast<unsigned int>(time(nullptr)));
                std::string conversational_response = confirmationMessages[rand() % confirmationMessages.size()];
                
                result["response_type"] = "confirmation";
                result["message"] = conversational_response;
                result["can_execute"] = true;
                result["task_summary"] = plan.objective;
                result["internal_plan_id"] = plan.plan_id;
                
                // Auto-execute only if explicitly requested AND the operation is safe
                if ((auto_execute || executor->getExecutionMode() == ExecutionMode::AUTONOMOUS) && !isDangerous) {
                    std::vector<std::string> executionMessages = {
                        "Absolutely! I'm on it right now. Working on your request...",
                        "You got it! Processing your task at this very moment...",
                        "Perfect! I'm handling this for you right away...",
                        "On my way! Executing your request now...",
                        "Consider it done! Working on this task immediately...",
                        "Right away! Processing your request as we speak...",
                        "I'm on the case! Executing your task now...",
                        "Fantastic! Taking care of this for you right now..."
                    };
                    
                    std::string execution_message = executionMessages[rand() % executionMessages.size()];
                    
                    ExecutionResult exec_result = executor->executeWithPlan(plan);
                    
                    std::vector<std::string> successMessages = {
                        "All done! Your task has been completed successfully.",
                        "Perfect! I've finished executing your request.",
                        "Completed! Everything went smoothly with your task.",
                        "Success! Your request has been processed and completed.",
                        "Finished! The task is now complete and ready to go.",
                        "Done! I've successfully handled your request.",
                        "Complete! Your task has been executed perfectly.",
                        "Accomplished! Everything is set up just as you requested."
                    };
                    
                    std::vector<std::string> errorMessages = {
                        "Oops! I ran into a small issue while executing your task.",
                        "Hmm, something didn't go as planned. Let me explain what happened.",
                        "I encountered a problem while working on your request.",
                        "There was a hiccup during execution. Here's what went wrong.",
                        "Unfortunately, I hit a snag while processing your task.",
                        "I ran into some trouble while executing your request.",
                        "Something went awry during the execution process.",
                        "I faced an unexpected issue while handling your task."
                    };
                    
                    json exec_json;
                    exec_json["success"] = exec_result.success;
                    if (exec_result.success) {
                        exec_json["message"] = successMessages[rand() % successMessages.size()];
                    } else {
                        exec_json["message"] = errorMessages[rand() % errorMessages.size()];
                        if (!exec_result.error_message.empty()) {
                            exec_json["details"] = exec_result.error_message;
                        }
                    }
                    exec_json["execution_time"] = exec_result.execution_time;
                    
                    result["execution_result"] = exec_json;
                    result["response_type"] = "completed";
                    
                    context_manager->addToHistory(user_input, execution_message, gemini_response.dump(), exec_result.success);
                }
            } else {
                result["response_type"] = "text";
                result["content"] = gemini_response.value("content", "No valid response");
                context_manager->addToHistory(user_input, gemini_response.dump(), "text_response", true);
            }
        }
        
        response.body = result.dump();
        
    } catch (const std::exception& e) {
        response.status_code = 500;
        json error_response;
        error_response["error"] = "Execution failed: " + std::string(e.what());
        response.body = error_response.dump();
    }
}

void HttpServer::handleGetHistory(const json& request_data, HttpResponse& response) {
    json history_response;
    history_response["history"] = json::array(); // Placeholder
    history_response["session_id"] = "current_session";
    response.body = history_response.dump();
}

void HttpServer::handleGetSystemInfo(const json& request_data, HttpResponse& response) {
    json system_info;
    system_info["execution_mode"] = static_cast<int>(executor->getExecutionMode());
    system_info["system_state"] = context_manager->getSystemState();
    system_info["user_preferences"] = context_manager->getUserPreferences();
    response.body = system_info.dump();
}

void HttpServer::handleUpdatePreferences(const json& request_data, HttpResponse& response) {
    try {
        for (const auto& item : request_data.items()) {
            context_manager->updateUserPreference(item.key(), item.value());
        }
        response.body = R"({"success": true})";
    } catch (const std::exception& e) {
        response.status_code = 400;
        response.body = R"({"error": "Failed to update preferences"})";
    }
}

void HttpServer::handleGetActiveProcesses(const json& request_data, HttpResponse& response) {
    json processes_response;
    processes_response["processes"] = executor->getActiveProcesses();
    response.body = processes_response.dump();
}

void HttpServer::handleRollback(const json& request_data, HttpResponse& response) {
    executor->rollbackLastAction();
    response.body = R"({"success": true, "message": "Rollback initiated"})";
}

void HttpServer::handleGetSuggestions(const json& request_data, HttpResponse& response) {
    json suggestions = executor->getSuggestedImprovements();
    response.body = suggestions.dump();
}

void HttpServer::handleVoiceInput(const json& request_data, HttpResponse& response) {
    response.body = R"({"error": "Voice input not yet implemented"})";
}

void HttpServer::handleImageInput(const json& request_data, HttpResponse& response) {
    response.body = R"({"error": "Image input not yet implemented"})";
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int hex = std::stoi(str.substr(i + 1, 2), nullptr, 16);
            result += static_cast<char>(hex);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::map<std::string, std::string> HttpServer::parseQueryString(const std::string& query) {
    std::map<std::string, std::string> params;
    std::istringstream iss(query);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, eq_pos));
            std::string value = urlDecode(pair.substr(eq_pos + 1));
            params[key] = value;
        }
    }
    
    return params;
}
