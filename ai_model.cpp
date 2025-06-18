#include "ai_model.h"
#include <iostream>
#include <string>
#include <memory>
#include <curl/curl.h>

#ifndef NO_CURL
// Callback function to write response data
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t total_size = size * nmemb;
    output->append((char *)contents, total_size);
    return total_size;
}
#endif

json callAIModel(const std::string &api_key, const std::string &user_prompt)
{
#ifdef NO_CURL
    std::cout << "⚠️  HTTP functionality disabled - libcurl not available" << std::endl;
    std::cout << "📝 User prompt: " << user_prompt << std::endl;
    // Return a mock response for testing without libcurl
    json mock_response = {
        {"choices", {{{"message", {{"content", "{\n  \"type\": \"powershell_script\",\n  \"script\": [\"echo 'Mock response - libcurl not available'\"],\n  \"explanation\": \"This is a mock response because libcurl is not available\",\n  \"confidence\": 0.1\n}"}}}}}}};

    return mock_response;
#else
    CURL *curl;
    CURLcode res;
    std::string response_data;

    // Initialize curl
    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize curl" << std::endl;
        return json::object();
    } // Create the request URL for OpenRouter DeepSeek API
    std::string url = "https://openrouter.ai/api/v1/chat/completions";    // Create enhanced prompt for DeepSeek R1 that handles step-by-step task breakdown
    std::string enhanced_prompt = "You are an advanced Windows automation assistant powered by DeepSeek R1. Analyze the user's task and determine if it requires:\n"
                                  "1. Simple PowerShell commands (for file operations, calculations, opening apps)\n"
                                  "2. Vision-guided UI automation (for complex interactions requiring screen analysis)\n\n"
                                  
                                  "For SIMPLE tasks (file operations, calculations, launching apps), respond with:\n"
                                  "{\n"
                                  "  \"type\": \"powershell_script\",\n"
                                  "  \"script\": [\"command1\", \"command2\"],\n"
                                  "  \"explanation\": \"What this accomplishes\",\n"
                                  "  \"confidence\": 0.95\n"
                                  "}\n\n"
                                  
                                  "For COMPLEX tasks requiring UI interaction (typing in specific apps, clicking buttons, navigating interfaces), respond with:\n"
                                  "{\n"
                                  "  \"type\": \"vision_task\",\n"
                                  "  \"initial_action\": \"Start-Process 'appname'\",\n"
                                  "  \"target_app\": \"application_name\",\n"
                                  "  \"objective\": \"Clear description of what to accomplish\",\n"
                                  "  \"explanation\": \"Why this needs vision guidance\",\n"
                                  "  \"confidence\": 0.85\n"
                                  "}\n\n"
                                  
                                  "CRITICAL GUIDELINES:\n"
                                  "- Use only standard Windows applications and PowerShell commands\n"
                                  "- For typing tasks in specific applications: use vision_task type\n"
                                  "- For file operations, calculations, opening apps: use powershell_script type\n"
                                  "- Never hardcode paths or assume non-standard software is installed\n"
                                  "- Be precise about the objective for vision tasks\n\n"
                                  
                                  "User task: " + user_prompt;
    json request_body = {
        {"model", "deepseek/deepseek-r1-0528-qwen3-8b:free"},
        {"messages", {{{"role", "user"}, {"content", enhanced_prompt}}}}};

    std::string json_string = request_body.dump();

    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    // Set headers
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth_header = "Authorization: Bearer " + api_key;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    res = curl_easy_perform(curl);

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return json::object();
    }

    // Parse the response
    try
    {
        json response = json::parse(response_data);
        // Extract the text from OpenRouter/DeepSeek response structure
        if (response.contains("choices") && !response["choices"].empty())
        {
            auto &choice = response["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content"))
            {
                std::string deepseek_text = choice["message"]["content"];                // Handle both powershell_script and vision_task types
                try
                {
                    json parsed = json::parse(deepseek_text);
                    
                    // Validate response format
                    if (parsed.contains("type"))
                    {
                        std::string type = parsed["type"];
                        if (type == "powershell_script" || type == "vision_task")
                        {
                            return parsed;
                        }
                    }
                    
                    // If no valid type, treat as text response
                    return json{{"type", "text"}, {"content", deepseek_text}};
                }
                catch (const json::parse_error &)
                {
                    // If direct parsing fails, try to extract JSON from markdown code blocks
                    size_t json_start = deepseek_text.find("```json");
                    if (json_start != std::string::npos)
                    {
                        json_start += 7; // Skip "```json"
                        size_t json_end = deepseek_text.find("```", json_start);
                        if (json_end != std::string::npos)
                        {
                            std::string json_content = deepseek_text.substr(json_start, json_end - json_start);
                            // Trim whitespace
                            json_content.erase(0, json_content.find_first_not_of(" \t\n\r"));
                            json_content.erase(json_content.find_last_not_of(" \t\n\r") + 1);
                            try
                            {
                                return json::parse(json_content);
                            }
                            catch (const json::parse_error &)
                            {
                                // If still can't parse, fall through to text response
                            }
                        }
                    }
                    // If it's not JSON, wrap it as a simple text response
                    return json{{"type", "text"}, {"content", deepseek_text}};
                }
            }
        }
        std::cerr << "Unexpected response format from DeepSeek API" << std::endl;
        std::cerr << "Full response: " << response.dump(2) << std::endl;
        return json::object();
    }
    catch (const json::parse_error &e)
    {
        std::cerr << "Failed to parse DeepSeek response: " << e.what() << std::endl;
        std::cerr << "Raw response: " << response_data << std::endl;
        return json::object();
    }
#endif // NO_CURL
}

// Vision-specific AI model call that returns vision action JSON
json callVisionAIModel(const std::string &api_key, const std::string &vision_prompt)
{
#ifdef NO_CURL
    std::cout << "⚠️  HTTP functionality disabled - libcurl not available" << std::endl;
    std::cout << "📝 Vision prompt: " << vision_prompt << std::endl;
    // Return a mock vision response for testing without libcurl
    json mock_response = {
        {"choices", {{{"message", {{"content", "{\n  \"action_type\": \"wait\",\n  \"target_description\": \"Mock action\",\n  \"value\": \"1000\",\n  \"explanation\": \"Mock response - libcurl not available\",\n  \"confidence\": 0.1\n}"}}}}}}};

    return mock_response;
#else
    CURL *curl;
    CURLcode res;
    std::string response_data;

    // Initialize curl
    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize curl for vision AI call" << std::endl;
        return json::object();
    }

    // Create the request URL for OpenRouter DeepSeek API
    std::string url = "https://openrouter.ai/api/v1/chat/completions"; // Create concise vision-specific system prompt
    std::string vision_system_prompt = "You are a Windows UI automation assistant. Analyze screen descriptions and return the next action as JSON.\n\n"
                                       "CRITICAL: Always end with valid JSON. Use this exact format:\n"
                                       "{\n"
                                       "  \"action_type\": \"click|type|scroll|wait|complete\",\n"
                                       "  \"target_description\": \"element to interact with\",\n"
                                       "  \"value\": \"text to type or scroll direction\",\n"
                                       "  \"explanation\": \"brief action description\",\n"
                                       "  \"confidence\": 0.8\n"
                                       "}\n\n"
                                       "Actions: click (UI elements), type (text input), scroll (up/down/left/right), wait (milliseconds), complete (task done).\n"
                                       "Think briefly, then provide the JSON. If you run out of tokens, prioritize the JSON output.";
    json request_body = {
        {"model", "deepseek/deepseek-r1-0528-qwen3-8b:free"},
        {"messages", {{{"role", "system"}, {"content", vision_system_prompt}}, {{"role", "user"}, {"content", vision_prompt}}}},
        {"temperature", 0.0}, // Zero temperature for maximum consistency
        {"max_tokens", 2500}  // Higher token limit to avoid cutoff
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
    std::string auth_header = "Authorization: Bearer " + api_key;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    res = curl_easy_perform(curl);

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed for vision AI: " << curl_easy_strerror(res) << std::endl;
        return json::object();
    } // Parse the response - handle DeepSeek R1's reasoning format
    try
    {
        json response = json::parse(response_data);

        // Enhanced parsing for DeepSeek R1 format
        if (response.contains("choices") && !response["choices"].empty())
        {
            auto &choice = response["choices"][0];
            if (choice.contains("message"))
            {
                auto &message = choice["message"];
                std::string full_text = "";

                // DeepSeek R1 format: check both reasoning and content fields
                if (message.contains("reasoning") && !message["reasoning"].get<std::string>().empty())
                {
                    full_text = message["reasoning"].get<std::string>();
                }

                if (message.contains("content") && !message["content"].get<std::string>().empty())
                {
                    if (!full_text.empty())
                        full_text += "\n\n";
                    full_text += message["content"].get<std::string>();
                }

                // If we have text, try to extract JSON from it
                if (!full_text.empty())
                {
                    // Method 1: Look for final JSON object
                    size_t last_brace = full_text.rfind("}");
                    if (last_brace != std::string::npos)
                    {
                        // Work backwards to find matching opening brace
                        int brace_count = 1;
                        size_t json_start = last_brace;

                        for (int i = last_brace - 1; i >= 0 && brace_count > 0; i--)
                        {
                            if (full_text[i] == '}')
                                brace_count++;
                            else if (full_text[i] == '{')
                            {
                                brace_count--;
                                if (brace_count == 0)
                                    json_start = i;
                            }
                        }

                        if (brace_count == 0)
                        {
                            std::string json_str = full_text.substr(json_start, last_brace - json_start + 1);
                            try
                            {
                                json extracted = json::parse(json_str);
                                // Wrap it in the expected response format
                                json formatted_response = {
                                    {"choices", {{{"message", {{"content", json_str}}}}}}};
                                return formatted_response;
                            }
                            catch (const json::parse_error &)
                            {
                                // If parsing fails, continue to fallback
                            }
                        }
                    }

                    // Method 2: Look for JSON in markdown blocks
                    size_t json_block_start = full_text.find("```json");
                    if (json_block_start != std::string::npos)
                    {
                        json_block_start += 7;
                        size_t json_block_end = full_text.find("```", json_block_start);
                        if (json_block_end != std::string::npos)
                        {
                            std::string json_str = full_text.substr(json_block_start, json_block_end - json_block_start);
                            // Trim whitespace
                            json_str.erase(0, json_str.find_first_not_of(" \t\n\r"));
                            json_str.erase(json_str.find_last_not_of(" \t\n\r") + 1);

                            try
                            {
                                json extracted = json::parse(json_str);
                                // Wrap it in the expected response format
                                json formatted_response = {
                                    {"choices", {{{"message", {{"content", json_str}}}}}}};
                                return formatted_response;
                            }
                            catch (const json::parse_error &)
                            {
                                // Continue to fallback
                            }
                        }
                    }
                }
            }
        } // If all parsing methods fail, generate a fallback JSON
        std::cout << "⚠️  JSON extraction failed, generating fallback action..." << std::endl;

        // Create a simple wait action as fallback
        json fallback_response = {
            {"choices", {{{"message", {{"content", "{\n  \"action_type\": \"wait\",\n  \"target_description\": \"interface\",\n  \"value\": \"2000\",\n  \"explanation\": \"Fallback action - JSON parsing failed\",\n  \"confidence\": 0.2\n}"}}}}}}};

        return fallback_response;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to parse vision AI response: " << e.what() << std::endl;
        std::cerr << "Raw response: " << response_data << std::endl;
        return json::object();
    }
#endif
}

// Dynamic Intent Analysis Functions - Replace Hardcoded Logic with AI
json callIntentAI(const std::string &api_key, const std::string &user_request)
{
#ifdef NO_CURL
    std::cout << "⚠️  HTTP functionality disabled - libcurl not available" << std::endl;
    // Return mock response for testing
    json mock_response = {
        {"is_vision_task", true},
        {"requires_app_launch", true},
        {"target_application", "notepad.exe"},
        {"app_name", "notepad"},
        {"requires_typing", true},
        {"text_to_type", "Hello"},
        {"requires_interaction", false},
        {"confidence", 0.8}};
    return mock_response;
#else
    CURL *curl;
    CURLcode res;
    std::string response_data;

    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize curl for intent analysis" << std::endl;
        return json::object();
    }

    std::string url = "https://openrouter.ai/api/v1/chat/completions";

    std::string intent_prompt =
        "You are an expert Windows automation assistant that analyzes user requests to determine the exact actions needed. "
        "Analyze the following user request and respond with a JSON object containing the intent analysis.\n\n"

        "Your response must be in this EXACT JSON format:\n"
        "{\n"
        "  \"is_vision_task\": boolean,\n"
        "  \"requires_app_launch\": boolean,\n"
        "  \"target_application\": \"app.exe or null\",\n"
        "  \"app_name\": \"friendly_name or null\",\n"
        "  \"requires_typing\": boolean,\n"
        "  \"text_to_type\": \"text or null\",\n"
        "  \"requires_interaction\": boolean,\n"
        "  \"interaction_target\": \"element_description or null\",\n"
        "  \"requires_navigation\": boolean,\n"
        "  \"navigation_target\": \"url_or_location or null\",\n"
        "  \"task_type\": \"web|messaging|file|system|text|calculation|other\",\n"
        "  \"confidence\": 0.0-1.0\n"
        "}\n\n"

        "Guidelines:\n"
        "- is_vision_task: true if requires UI interaction, screen analysis, or visual elements\n"
        "- For web tasks: set target_application to browser (msedge.exe or chrome.exe)\n"
        "- For messaging: detect apps like WhatsApp, Discord, etc.\n"
        "- Extract quoted text for typing: \"Hello World\" -> text_to_type: \"Hello World\"\n"
        "- Be intelligent about application detection from context\n"
        "- Set confidence based on clarity of the request\n\n"

        "User request: " +
        user_request;

    json request_body = {
        {"model", "deepseek/deepseek-r1-0528-qwen3-8b:free"},
        {"messages", {{{"role", "user"}, {"content", intent_prompt}}}}};

    std::string json_string = request_body.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth_header = "Authorization: Bearer " + api_key;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "Intent analysis API call failed: " << curl_easy_strerror(res) << std::endl;
        return json::object();
    }

    try
    {
        json response = json::parse(response_data);
        if (response.contains("choices") && !response["choices"].empty())
        {
            auto &choice = response["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content"))
            {
                std::string content = choice["message"]["content"];

                // Try to extract JSON from the response
                size_t json_start = content.find_first_of('{');
                size_t json_end = content.find_last_of('}');

                if (json_start != std::string::npos && json_end != std::string::npos)
                {
                    std::string json_str = content.substr(json_start, json_end - json_start + 1);
                    try
                    {
                        return json::parse(json_str);
                    }
                    catch (const json::parse_error &e)
                    {
                        std::cerr << "Failed to parse intent JSON: " << e.what() << std::endl;
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to parse intent response: " << e.what() << std::endl;
    }

    return json::object();
#endif
}

json callVisionAI(const std::string &api_key, const std::string &task, const std::string &screen_description, const std::vector<std::string> &available_elements)
{
#ifdef NO_CURL
    std::cout << "⚠️  HTTP functionality disabled - libcurl not available" << std::endl;
    // Return mock response for testing
    json mock_response = {
        {"action_type", "type"},
        {"target_description", "text input area"},
        {"value", "Hello"},
        {"explanation", "Mock vision AI response"},
        {"confidence", 0.7}};
    return mock_response;
#else
    CURL *curl;
    CURLcode res;
    std::string response_data;

    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize curl for vision AI" << std::endl;
        return json::object();
    }

    std::string url = "https://openrouter.ai/api/v1/chat/completions";

    // Build elements list
    std::string elements_str = "";
    for (size_t i = 0; i < available_elements.size() && i < 20; ++i) // Limit to first 20 elements
    {
        elements_str += std::to_string(i + 1) + ". " + available_elements[i] + "\n";
    }

    std::string vision_prompt =
        "You are an expert computer vision assistant that analyzes screen state and determines the best action to take. "
        "Given a task, current screen description, and available UI elements, decide the optimal next action.\n\n"

        "Your response must be in this EXACT JSON format:\n"
        "{\n"
        "  \"action_type\": \"click|type|wait|scroll|key\",\n"
        "  \"target_description\": \"description_of_target_element\",\n"
        "  \"value\": \"text_to_type_or_key_or_null\",\n"
        "  \"explanation\": \"why_this_action\",\n"
        "  \"confidence\": 0.0-1.0\n"
        "}\n\n"

        "Task to accomplish: " +
        task + "\n\n"
               "Current screen state: " +
        screen_description + "\n\n"
                             "Available UI elements:\n" +
        elements_str + "\n\n"

                       "Guidelines:\n"
                       "- Choose the most appropriate element from the list for the task\n"
                       "- Prefer application-specific text areas over system elements\n"
                       "- Avoid clicking on taskbar, system tray, or search boxes unless specifically needed\n"
                       "- For typing tasks, find the main content area of the active application\n"
                       "- Set confidence based on how well the available elements match the task\n"
                       "- If no good elements are available, suggest 'wait' action\n";

    json request_body = {
        {"model", "deepseek/deepseek-r1-0528-qwen3-8b:free"},
        {"messages", {{{"role", "user"}, {"content", vision_prompt}}}}};

    std::string json_string = request_body.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth_header = "Authorization: Bearer " + api_key;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "Vision AI API call failed: " << curl_easy_strerror(res) << std::endl;
        return json::object();
    }

    try
    {
        json response = json::parse(response_data);
        if (response.contains("choices") && !response["choices"].empty())
        {
            auto &choice = response["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content"))
            {
                std::string content = choice["message"]["content"];

                // Try to extract JSON from the response
                size_t json_start = content.find_first_of('{');
                size_t json_end = content.find_last_of('}');

                if (json_start != std::string::npos && json_end != std::string::npos)
                {
                    std::string json_str = content.substr(json_start, json_end - json_start + 1);
                    try
                    {
                        return json::parse(json_str);
                    }
                    catch (const json::parse_error &e)
                    {
                        std::cerr << "Failed to parse vision AI JSON: " << e.what() << std::endl;
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to parse vision AI response: " << e.what() << std::endl;
    }

    return json::object();
#endif
}
