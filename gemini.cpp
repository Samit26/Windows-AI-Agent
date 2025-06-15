#include "gemini.h"
#include <iostream>
#include <curl/curl.h>
#include <string>
#include <memory>

// Callback function to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

json callGemini(const std::string &api_key, const std::string &user_prompt) {
    CURL *curl;
    CURLcode res;
    std::string response_data;
    
    // Initialize curl
    curl = curl_easy_init();
    if(!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
        return json::object();
    }
      // Create the request URL
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash-001:generateContent?key=" + api_key;    // Create the JSON payload with system instructions
    std::string enhanced_prompt = "You are a Windows automation assistant. The user will give you a task, and you must respond with PowerShell commands to accomplish it. Your response must be in this exact JSON format:\n\n"
                                 "{\n"
                                 "  \"type\": \"powershell_script\",\n" 
                                 "  \"script\": [\n"
                                 "    \"command1\",\n"
                                 "    \"command2\"\n"
                                 "  ]\n"
                                 "}\n\n"
                                 "IMPORTANT GUIDELINES:\n"
                                 "1. Only use built-in Windows commands, PowerShell cmdlets, and standard Windows applications\n"
                                 "2. For opening applications, use: Start-Process 'appname' (e.g., calc, notepad, chrome)\n"
                                 "3. For websites, use: Start-Process 'https://url'\n"
                                 "4. For calculations, use PowerShell's built-in math: Write-Host \"The result is: $((69+70))\"\n"
                                 "5. For complex tasks like messaging apps, explain limitations and suggest manual alternatives\n"
                                 "6. Never assume third-party modules or apps are installed unless they're standard Windows components\n"
                                 "7. If a task cannot be automated with standard Windows tools, provide helpful guidance instead\n\n"
                                 "Examples:\n"
                                 "- Calculator: {\"type\": \"powershell_script\", \"script\": [\"Start-Process 'calc'\"]}\n"
                                 "- Math: {\"type\": \"powershell_script\", \"script\": [\"Write-Host \\\"The result is: $((69+70))\\\"\"]}\n"
                                 "- Website: {\"type\": \"powershell_script\", \"script\": [\"Start-Process 'https://example.com'\"]}\n\n"
                                 "User task: " + user_prompt;
    
    json request_body = {
        {"contents", {{
            {"parts", {{
                {"text", enhanced_prompt}
            }}}
        }}}
    };
    
    std::string json_string = request_body.dump();
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    
    // Set headers
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Perform the request
    res = curl_easy_perform(curl);
    
    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if(res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return json::object();
    }
    
    // Parse the response
    try {
        json response = json::parse(response_data);
        
        // Extract the text from Gemini's response structure
        if(response.contains("candidates") && !response["candidates"].empty()) {
            auto& candidate = response["candidates"][0];
            if(candidate.contains("content") && candidate["content"].contains("parts") && !candidate["content"]["parts"].empty()) {
                std::string gemini_text = candidate["content"]["parts"][0]["text"];
                  // Try to parse Gemini's response as JSON (for structured commands)
                try {
                    // First, try to parse directly as JSON
                    return json::parse(gemini_text);
                } catch(const json::parse_error&) {
                    // If direct parsing fails, try to extract JSON from markdown code blocks
                    size_t json_start = gemini_text.find("```json");
                    if(json_start != std::string::npos) {
                        json_start += 7; // Skip "```json"
                        size_t json_end = gemini_text.find("```", json_start);
                        if(json_end != std::string::npos) {
                            std::string json_content = gemini_text.substr(json_start, json_end - json_start);
                            // Trim whitespace
                            json_content.erase(0, json_content.find_first_not_of(" \t\n\r"));
                            json_content.erase(json_content.find_last_not_of(" \t\n\r") + 1);
                            try {
                                return json::parse(json_content);
                            } catch(const json::parse_error&) {
                                // If still can't parse, fall through to text response
                            }
                        }
                    }
                    // If it's not JSON, wrap it as a simple text response
                    return json{{"type", "text"}, {"content", gemini_text}};
                }
            }
        }
          std::cerr << "Unexpected response format from Gemini API" << std::endl;
        std::cerr << "Full response: " << response.dump(2) << std::endl;
        return json::object();
        
    } catch(const json::parse_error& e) {
        std::cerr << "Failed to parse Gemini response: " << e.what() << std::endl;
        std::cerr << "Raw response: " << response_data << std::endl;
        return json::object();
    }
}
