#include "ai_model.h"
#include <iostream>
#include <string>
#include <memory>
#include <curl/curl.h>

// Callback function to write response data
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t total_size = size * nmemb;
    output->append((char *)contents, total_size);
    return total_size;
}

// TODO: Unit Test: Add unit tests for extractJsonFromString with various valid and invalid JSON strings (direct parse, markdown, malformed, empty).
// Helper function to extract JSON from a string
json extractJsonFromString(const std::string &text_response)
{
    try
    {
        // Attempt direct parsing
        return json::parse(text_response);
    }
    catch (const json::parse_error &e)
    {
        // Log the direct parsing error
        std::cerr << "Direct JSON parsing failed: " << e.what() << std::endl;
        std::cerr << "Raw response for direct parsing failure: " << text_response << std::endl;

        // If direct parsing fails, try to extract JSON from markdown code blocks
        size_t json_start = text_response.find("```json");
        if (json_start != std::string::npos)
        {
            json_start += 7; // Skip "```json"
            size_t json_end = text_response.find("```", json_start);
            if (json_end != std::string::npos)
            {
                std::string json_content = text_response.substr(json_start, json_end - json_start);
                // Trim whitespace
                json_content.erase(0, json_content.find_first_not_of(" \t\n\r"));
                json_content.erase(json_content.find_last_not_of(" \t\n\r") + 1);
                try
                {
                    return json::parse(json_content);
                }
                catch (const json::parse_error &e_markdown)
                {
                    // Log the markdown parsing error
                    std::cerr << "Markdown JSON parsing failed: " << e_markdown.what() << std::endl;
                    std::cerr << "Raw content from markdown for parsing failure: " << json_content << std::endl;
                }
            }
        }
    }
    // If all attempts fail, log and return an empty json object
    std::cerr << "Failed to extract JSON from response: " << text_response << std::endl;
    return json::object();
}

// TODO: Unit Test: Add integration tests for these functions, mocking curl calls and verifying prompt construction and response parsing.
json callAIModel(const std::string &api_key, const std::string &user_prompt)
{
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
    std::string url = "https://openrouter.ai/api/v1/chat/completions";
    // TODO: Reinforce in the prompt that if the AI decides on a structured command (powershell_script or vision_task),
    // the JSON output should be the ONLY content in its response and must be valid JSON.
    // For example, add: "If returning JSON, ensure it is the sole content of your response."
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
                std::string deepseek_text = choice["message"]["content"];
                json parsed_json = extractJsonFromString(deepseek_text);

                if (!parsed_json.empty() && parsed_json.contains("type"))
                {
                    std::string type = parsed_json["type"];
                    if (type == "powershell_script" || type == "vision_task")
                    {
                        return parsed_json;
                    }
                }
                // If JSON is empty, parsing failed, or type is not valid, treat as text response
                // Also log the original text if parsing failed or type was invalid.
                if (parsed_json.empty()){
                    std::cerr << "Failed to parse JSON from callAIModel, returning as text. Original text: " << deepseek_text << std::endl;
                } else if (!parsed_json.contains("type")) {
                    std::cerr << "Parsed JSON in callAIModel lacks 'type' field, returning as text. Original text: " << deepseek_text << std::endl;
                } else {
                     std::cerr << "Parsed JSON in callAIModel has invalid 'type', returning as text. Original text: " << deepseek_text << std::endl;
                }
                return json{{"type", "text"}, {"content", deepseek_text}};
            }
        }
        std::cerr << "Unexpected response format from DeepSeek API" << std::endl;
        std::cerr << "Full response: " << response.dump(2) << std::endl;
        return json::object();
    }
    catch (const json::parse_error &e) // This catch block is for the initial parsing of the whole API response
    {
        std::cerr << "Failed to parse the main API response: " << e.what() << std::endl;
        std::cerr << "Raw API response: " << response_data << std::endl;
        return json::object();
    }
}

// TODO: Unit Test: Add integration tests for these functions, mocking curl calls and verifying prompt construction and response parsing.
// Vision-specific AI model call that returns vision action JSON
json callVisionAIModel(const std::string &api_key, const std::string &vision_prompt)
{
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
    std::string url = "https://openrouter.ai/api/v1/chat/completions";
    // TODO: The prompt already asks for "ONLY a JSON object" and "Always end with valid JSON".
    // Review if this can be made stricter or if alternative phrasings could improve LLM adherence.
    // For example, "Your entire response must be a single, valid JSON object, with no surrounding text or explanations."
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
                    json extracted_json = extractJsonFromString(full_text);
                    // Check if the extracted JSON is valid and contains the expected structure
                    if (!extracted_json.empty() && extracted_json.contains("action_type"))
                    {
                        // The AI is expected to return ONLY the JSON object for the action.
                        // So, we return the extracted_json directly.
                        return extracted_json;
                    }
                    // Log if full_text was present but JSON extraction failed or was invalid
                    std::cerr << "JSON extraction from full_text in callVisionAIModel failed or result was invalid." << std::endl;
                    // extractJsonFromString already logs the full_text if parsing fails.
                }
            }
        }

        // If parsing fails, content is missing, or extracted JSON is not valid, generate a fallback JSON
        std::cout << "⚠️  JSON processing failed in callVisionAIModel, generating fallback action..." << std::endl;
        // The raw full_text (if available and parsing attempted) would have been logged by extractJsonFromString.

        json fallback_action = {
            {"action_type", "wait"},
            {"target_description", "interface"},
            {"value", "2000"},
            {"explanation", "Fallback action - JSON processing failed"},
            {"confidence", 0.2}};
        // This function, unlike callAIModel, is expected to return the action JSON directly, not wrapped.
        return fallback_action;
    }
    catch (const std::exception &e) // This catch block is for the initial parsing of the whole API response
    {
        std::cerr << "Failed to parse the main vision API response: " << e.what() << std::endl;
        std::cerr << "Raw vision API response: " << response_data << std::endl;
        // Return a fallback action in case of major API response parsing failure
        json fallback_action = {
            {"action_type", "wait"},
            {"target_description", "interface"},
            {"value", "2000"},
            {"explanation", "Fallback action - Main API response parsing failed"},
            {"confidence", 0.1}};
        return fallback_action;
    }
}

// TODO: Unit Test: Add integration tests for these functions, mocking curl calls and verifying prompt construction and response parsing.
// Dynamic Intent Analysis Functions - Replace Hardcoded Logic with AI
json callIntentAI(const std::string &api_key, const std::string &user_request)
{
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

    // TODO: Review the effectiveness of this prompt. Consider adding a line like:
    // "Ensure the entire response is a single, valid JSON object with no additional text or explanations."
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
                json extracted_json = extractJsonFromString(content);
                // If extraction fails, extractJsonFromString logs and returns empty json::object(),
                // which is the desired behavior for callIntentAI on failure.
                // If content was present but parsing failed, extractJsonFromString already logged it.
                return extracted_json;
            }
        }
        std::cerr << "Unexpected response format from Intent AI API" << std::endl;
        // Log the raw response if the structure is unexpected.
        std::cerr << "Full Intent AI response: " << response.dump(2) << std::endl;
    }
    catch (const std::exception &e) // This catch block is for the initial parsing of the whole API response
    {
        std::cerr << "Failed to parse the main Intent API response: " << e.what() << std::endl;
        std::cerr << "Raw Intent API response: " << response_data << std::endl;
    }

    return json::object(); // Default return if other paths fail
}

json callVisionAI(const std::string &api_key, const std::string &task, const std::string &screen_description, const std::vector<std::string> &available_elements)
{
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

                // This function `callVisionAI` seems to have a different JSON structure expectation
                // than `callVisionAIModel`. It expects the direct action JSON.
                std::string content = choice["message"]["content"];
                json extracted_json = extractJsonFromString(content);
                // If extraction fails, extractJsonFromString logs and returns empty json::object()
                // If content was present but parsing failed, extractJsonFromString already logged it.
                return extracted_json; // Return the parsed JSON or an empty object on failure
            }
        }
        std::cerr << "Unexpected response format from Vision AI API (callVisionAI)" << std::endl;
        // Log the raw response if the structure is unexpected.
        std::cerr << "Full Vision AI response (callVisionAI): " << response.dump(2) << std::endl;
    }
    catch (const std::exception &e) // This catch block is for the initial parsing of the whole API response
    {
        std::cerr << "Failed to parse the main Vision AI API response (callVisionAI): " << e.what() << std::endl;
        std::cerr << "Raw Vision AI response (callVisionAI): " << response_data << std::endl;
    }

    return json::object(); // Default return if other paths fail
}
