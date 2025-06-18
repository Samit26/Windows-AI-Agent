#include "context_manager.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <iomanip>

ContextManager::ContextManager() {
    startNewSession();
}

void ContextManager::startNewSession() {
    // Generate session ID based on timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "session_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    session_id = ss.str();
    
    conversation_history.clear();
    
    // Load persistent user preferences if they exist
    loadUserPreferences();
}

void ContextManager::loadSession(const std::string& session_id) {
    this->session_id = session_id;
    
    std::filesystem::create_directories("sessions");
    std::string filepath = "sessions/" + session_id + ".json";
    
    std::ifstream file(filepath);
    if (file.is_open()) {
        json session_data;
        file >> session_data;
        
        if (session_data.contains("history")) {
            conversation_history.clear();
            for (const auto& entry : session_data["history"]) {
                ConversationEntry conv_entry;
                conv_entry.timestamp = entry.value("timestamp", "");
                conv_entry.user_input = entry.value("user_input", "");
                conv_entry.ai_response = entry.value("ai_response", "");
                conv_entry.action_taken = entry.value("action_taken", "");
                conv_entry.success = entry.value("success", false);
                conversation_history.push_back(conv_entry);
            }
        }
        
        if (session_data.contains("preferences")) {
            user_preferences = session_data["preferences"];
        }
        
        if (session_data.contains("system_state")) {
            system_state = session_data["system_state"];
        }
    }
}

void ContextManager::saveSession() {
    std::filesystem::create_directories("sessions");
    std::string filepath = "sessions/" + session_id + ".json";
    
    json session_data;
    session_data["session_id"] = session_id;
    session_data["preferences"] = user_preferences;
    session_data["system_state"] = system_state;
    
    json history_array = json::array();
    for (const auto& entry : conversation_history) {
        json hist_entry;
        hist_entry["timestamp"] = entry.timestamp;
        hist_entry["user_input"] = entry.user_input;
        hist_entry["ai_response"] = entry.ai_response;
        hist_entry["action_taken"] = entry.action_taken;
        hist_entry["success"] = entry.success;
        history_array.push_back(hist_entry);
    }
    session_data["history"] = history_array;
    
    std::ofstream file(filepath);
    file << session_data.dump(2);
}

void ContextManager::addToHistory(const std::string& user_input, const std::string& ai_response, 
                                 const std::string& action, bool success) {
    ConversationEntry entry;
    
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    entry.timestamp = ss.str();
    
    entry.user_input = user_input;
    entry.ai_response = ai_response;
    entry.action_taken = action;
    entry.success = success;
    
    conversation_history.push_back(entry);
    
    // Auto-save after each interaction
    saveSession();
}

std::string ContextManager::getContextualPrompt(const std::string& user_input)
{
    std::string contextual_prompt = "You are an advanced Windows AI assistant with memory and learning capabilities.\n\n";

    if (!user_preferences.empty())
    {
        contextual_prompt += "USER PREFERENCES:\n" + user_preferences.dump(2) + "\n\n";
    }

    if (!conversation_history.empty())
    {
        contextual_prompt += "RECENT CONVERSATION HISTORY:\n";
        int start_idx = std::max(0, (int)conversation_history.size() - 3);
        for (int i = start_idx; i < conversation_history.size(); i++)
        {
            const auto& entry = conversation_history[i];
            contextual_prompt += "User: " + entry.user_input + "\n";
            contextual_prompt += "Action: " + entry.action_taken + "\n";
            contextual_prompt += "Success: " + std::string(entry.success ? "Yes" : "No") + "\n\n";
        }
    }

    if (!system_state.empty())
    {
        contextual_prompt += "CURRENT SYSTEM STATE:\n" + system_state.dump(2) + "\n\n";
    }

    contextual_prompt += "CURRENT USER REQUEST: " + user_input + "\n\n";

    contextual_prompt += "Based on the context above, provide a helpful response. Your response must be in this exact JSON format:\n\n"
                        "{\n"
                        "  \"type\": \"powershell_script\",\n"
                        "  \"script\": [\n"
                        "    \"command1\",\n"
                        "    \"command2\"\n"
                        "  ],\n"
                        "  \"explanation\": \"Brief explanation of what this will do\",\n"
                        "  \"confidence\": 0.95\n"
                        "}\n\n"
                        "IMPORTANT GUIDELINES:\n"
                        "1. Use only built-in Windows commands and standard applications\n"
                        "2. For opening applications, use: Start-Process 'appname'\n"
                        "3. For websites, use: Start-Process 'https://url'\n"
                        "4. For calculations, use PowerShell's built-in math\n"
                        "5. For complex tasks, explain limitations and suggest alternatives\n"
                        "6. Include confidence score (0.0-1.0) based on certainty\n";

    return contextual_prompt;
}

void ContextManager::updateUserPreference(const std::string& key, const json& value) {
    user_preferences[key] = value;
    saveSession();
}

json ContextManager::getUserPreferences() {
    return user_preferences;
}

void ContextManager::updateSystemState(const std::string& key, const json& value) {
    system_state[key] = value;
    saveSession();
}

json ContextManager::getSystemState() {
    return system_state;
}

void ContextManager::trimOldHistory(int max_entries) {
    if (conversation_history.size() > max_entries) {
        conversation_history.erase(conversation_history.begin(), 
                                 conversation_history.end() - max_entries);
        saveSession();
    }
}

std::string ContextManager::getRecentContext(int num_entries) {
    std::string context = "";
    int start_idx = std::max(0, (int)conversation_history.size() - num_entries);
    for (int i = start_idx; i < conversation_history.size(); i++) {
        const auto& entry = conversation_history[i];
        context += "[" + entry.timestamp + "] User: " + entry.user_input + " | Action: " + entry.action_taken + "\n";
    }
    return context;
}

void ContextManager::loadUserPreferences() {
    std::ifstream file("user_preferences.json");
    if (file.is_open()) {
        try {
            file >> user_preferences;
        } catch (const std::exception& e) {
            user_preferences = json::object();
        }
    } else {
        user_preferences = json::object();
    }
}
