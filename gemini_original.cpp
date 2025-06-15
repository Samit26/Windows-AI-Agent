#include "gemini.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

json callGemini(const std::string &api_key, const std::string &user_prompt) {
    CURL* curl = curl_easy_init();
    if(!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string url = "https://generativelanguage.googleapis.com/v1/models/text-bison-001:generateText?key=" + api_key;
    json request_body = {
        {"prompt", {{"text",
            "You are a Windows AI assistant.\n"
            "When a user gives you any natural language command, convert it into a safe, working PowerShell script that can be run on a local Windows PC.\n"
            "Respond ONLY in this JSON format with keys 'type' and 'script':\n"
            "{ \"type\": \"powershell_script\", \"script\": [ ... ] }\n\n"
            "User: " + user_prompt
        }}}
    };

    std::string response_string;
    std::string request_str = request_body.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    curl_easy_cleanup(curl);

    json response_json = json::parse(response_string);
    // Navigate into the response structure
    if(response_json.contains("candidates") && !response_json["candidates"].empty()) {
        std::string output_text = response_json["candidates"][0]["output"].get<std::string>();
        return json::parse(output_text);
    } else {
        throw std::runtime_error("Unexpected response format from Gemini");
    }
}