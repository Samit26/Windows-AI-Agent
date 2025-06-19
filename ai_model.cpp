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
    // TODO: Reinforce in the prompt that if the AI decides on a structured command,
    // the JSON output should be the ONLY content in its response and must be valid JSON.
    // For example, add: "If returning JSON, ensure it is the sole content of your response and strictly adheres to the defined schema."
    std::string enhanced_prompt = "You are an advanced Windows automation assistant. Analyze the user's task and respond with a JSON object describing the plan. Possible types of responses:\n\n"
                                  "1. For SIMPLE tasks (file operations, calculations, launching apps):\n"
                                  "   {\n"
                                  "     \"type\": \"powershell_script\",\n"
                                  "     \"script\": [\"command1\", \"command2\"],\n"
                                  "     \"explanation\": \"What this accomplishes\",\n"
                                  "     \"confidence\": 0.95\n"
                                  "   }\n\n"
                                  "2. For COMPLEX tasks requiring UI interaction (typing, clicking, navigating interfaces):\n"
                                  "   {\n"
                                  "     \"type\": \"vision_task\",\n"
                                  "     \"initial_action\": \"Optional: PowerShell command to start an app (e.g., 'Start-Process \\\"notepad.exe\\\"')\",\n"
                                  "     \"target_app\": \"Optional: Friendly name of the target application (e.g., 'Notepad')\",\n"
                                  "     \"objective\": \"Clear description of what to accomplish using vision (e.g., 'Click the File menu, then click Save')\",\n"
                                  "     \"explanation\": \"Why this needs vision guidance\",\n"
                                  "     \"confidence\": 0.85\n"
                                  "   }\n\n"
                                  "3. For tasks that require GENERATING TEXT/CONTENT and then USING IT (e.g., writing an email, then typing or saving it):\n"
                                  "   {\n"
                                  "     \"type\": \"generate_content_and_execute\",\n"
                                  "     \"content_generation_prompt\": \"Prompt for an LLM to generate the desired content (e.g., 'Write a short story about a robot learning to paint')\",\n"
                                  "     \"subsequent_action\": { // Object defining what to do with the generated content\n"
                                  "       \"type\": \"vision_task\", // Can also be \"powershell_script\", or a new \"save_to_file_action\"\n"
                                  "       \"objective\": \"Objective for the action using the generated content (e.g., 'Type the generated story into the active window')\",\n"
                                  "       \"target_app\": \"Optional: target application context (e.g., 'Microsoft Word')\",\n"
                                  "       // ... other parameters relevant to subsequent_action.type ...\n"
                                  "       // Example for a potential \"save_to_file_action\" (hypothetical, not yet implemented by agent):\n"
                                  "       // \"filename_prompt\": \"Suggest a filename for the content (e.g., 'robot_story.txt')\",\n"
                                  "       // \"overwrite\": false\n"
                                  "     },\n"
                                  "     \"explanation\": \"Brief explanation of this combined step (generate and use).\",\n"
                                  "     \"confidence\": 0.80\n"
                                  "   }\n\n"
                                  "4. For tasks involving MULTIPLE DISTINCT ACTIONS (e.g., open an app, then generate content and type it, then save a file):\n"
                                  "   {\n"
                                  "     \"type\": \"multi_step_plan\",\n"
                                  "     \"steps\": [\n"
                                  "       { /* Step 1: e.g., {\"type\": \"powershell_script\", \"script\": [\"Start-Process notepad.exe\"], ...} */ },\n"
                                  "       { /* Step 2: e.g., {\"type\": \"generate_content_and_execute\", ...} to write and type a story */ },\n"
                                  "       { /* Step 3: e.g., {\"type\": \"vision_task\", \"objective\": \"Click File, then Save As...\"} */ }\n"
                                  "       // Each step in the 'steps' array should be one of the valid single-step JSON structures (powershell_script, vision_task, generate_content_and_execute).\n"
                                  "     ],\n"
                                  "     \"explanation\": \"Overall explanation of the multi-step plan.\",\n"
                                  "     \"confidence\": 0.75\n"
                                  "   }\n\n"
                                  "CRITICAL GUIDELINES:\n"
                                  "- ALWAYS respond with a single, valid JSON object. Your entire response must be this JSON object.\n"
                                  "- Do NOT include any text or explanation outside of the JSON structure.\n"
                                  "- For `powershell_script`, use only standard Windows applications and PowerShell commands.\n"
                                  "- For `vision_task`, be precise about the `objective`.\n"
                                  "- For `generate_content_and_execute`, the `content_generation_prompt` should be specific for an LLM, and `subsequent_action` must be a valid action type.\n"
                                  "- For `multi_step_plan`, each item in `steps` must be a complete and valid JSON action definition.\n"
                                  "- Never hardcode user-specific paths. Use general methods.\n\n"
                                  "User task: " +
                                  user_prompt;
    json request_body = {
        {"model", "deepseek/deepseek-r1-0528-qwen3-8b:free"}, // Consider updating model if needed for complex planning
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
                    std::string response_main_type = parsed_json["type"].get<std::string>();
                    if (response_main_type == "powershell_script" ||
                        response_main_type == "vision_task" ||
                        response_main_type == "generate_content_and_execute" ||
                        response_main_type == "multi_step_plan")
                    {
                        return parsed_json; // These are valid structured responses
                    }
                    // If type is something else, but it's still valid JSON, it might be an error or unexpected.
                    std::cerr << "callAIModel: Received valid JSON but with an unrecognized primary type: "
                              << response_main_type << ". Response: " << parsed_json.dump(2) << std::endl;
                }
                else if (parsed_json.empty())
                {
                    std::cerr << "callAIModel: extractJsonFromString returned empty JSON. Original text: " << deepseek_text << std::endl;
                }
                else
                { // Not empty, but no "type" field
                    std::cerr << "callAIModel: Parsed JSON is missing 'type' field. Original text: " << deepseek_text
                              << ". Parsed JSON: " << parsed_json.dump(2) << std::endl;
                }
                // Fallback: if not a recognized structured type, if "type" is missing, or if JSON parsing failed (parsed_json is empty)
                return json{{"type", "text"}, {"content", deepseek_text}};
            }
        }
        std::cerr << "Unexpected response format from AI API (missing choices or content field)" << std::endl;
        std::cerr << "Full API response: " << response.dump(2) << std::endl;
        return json::object(); // Return empty JSON if the expected structure (choices[0].message.content) is not there
    }
    catch (const json::parse_error &e) // This catch block is for the initial parsing of the *entire* API response
    {
        std::cerr << "Failed to parse the main AI API response JSON: " << e.what() << std::endl;
        std::cerr << "Raw API response data: " << response_data << std::endl;
        return json::object(); // Return empty JSON on parsing failure
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

// Function to get plain text responses from the LLM, suitable for content generation
std::string callLLMForTextGeneration(const std::string &api_key, const std::string &text_generation_prompt)
{
    CURL *curl;
    CURLcode res;
    std::string response_data;

    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize curl for text generation" << std::endl;
        return "Error: Curl initialization failed.";
    }

    std::string url = "https://openrouter.ai/api/v1/chat/completions";

    // System prompt tailored for direct text generation
    std::string system_prompt_text_gen = "You are a helpful AI assistant. Please directly respond to the following request for text generation. Provide only the generated text as your response, without any additional explanations, conversational filler, or JSON formatting.";

    json request_body = {
        {"model", "deepseek/deepseek-r1-0528-qwen3-8b:free"}, // Or any other suitable model for text generation
        {"messages", json::array({{{"role", "system"}, {"content", system_prompt_text_gen}},
                                  {{"role", "user"}, {"content", text_generation_prompt}}})},
        {"temperature", 0.7} // Adjust temperature for creativity as needed
    };

    std::string json_string = request_body.dump();

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth_header = "Authorization: Bearer " + api_key;
    headers = curl_slist_append(headers, auth_header.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed for text generation: " << curl_easy_strerror(res) << std::endl;
        return "Error: LLM call failed (" + std::string(curl_easy_strerror(res)) + ")";
    }

    try
    {
        json response_json = json::parse(response_data);
        if (response_json.contains("choices") && !response_json["choices"].empty())
        {
            const auto &choice = response_json["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content"))
            {
                return choice["message"]["content"].get<std::string>();
            }
        }
        std::cerr << "Error: Unexpected JSON structure in LLM response for text generation. Full response: " << response_json.dump(2) << std::endl;
        return "Error: Could not extract content from LLM response.";
    }
    catch (const json::parse_error &e)
    {
        std::cerr << "Error: Failed to parse LLM response JSON for text generation: " << e.what() << std::endl;
        std::cerr << "Raw response data: " << response_data << std::endl;
        return "Error: Failed to parse LLM response.";
    }
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
