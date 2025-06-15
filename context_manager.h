#ifndef CONTEXT_MANAGER_H
#define CONTEXT_MANAGER_H

#include "include/json.hpp"
#include <string>
#include <vector>
#include <chrono>

using json = nlohmann::json;

struct ConversationEntry {
    std::string timestamp;
    std::string user_input;
    std::string ai_response;
    std::string action_taken;
    bool success;
};

class ContextManager {
private:
    std::vector<ConversationEntry> conversation_history;
    json user_preferences;
    json system_state;
    std::string session_id;
    
public:
    ContextManager();
    
    // Session management
    void startNewSession();
    void loadSession(const std::string& session_id);
    void saveSession();
    
    // Context operations
    void addToHistory(const std::string& user_input, const std::string& ai_response, 
                     const std::string& action, bool success);
    std::string getContextualPrompt(const std::string& user_input);
    
    // Preferences and learning
    void updateUserPreference(const std::string& key, const json& value);
    json getUserPreferences();
    
    // System state tracking
    void updateSystemState(const std::string& key, const json& value);
    json getSystemState();
      // Memory management
    void trimOldHistory(int max_entries = 50);
    std::string getRecentContext(int num_entries = 5);
    
private:
    void loadUserPreferences();
};

#endif // CONTEXT_MANAGER_H
