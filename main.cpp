#include <iostream>
#include <fstream>
#include "include/json.hpp"
#include "gemini.h"
#include "executor.h"

using json = nlohmann::json;

int main() {
    try {
        // Load configuration
        std::ifstream config_file("config.json");
        if(!config_file.is_open()) {
            std::cerr << "Could not open config.json" << std::endl;
            return 1;
        }
        json config;
        config_file >> config;
        std::string api_key = config["api_key"].get<std::string>();

        // Read user prompt
        std::cout << "Enter your task: ";
        std::string user_prompt;
        std::getline(std::cin, user_prompt);

        // Call Gemini API
        json response = callGemini(api_key, user_prompt);

        if(response.contains("type") && response["type"] == "powershell_script" && response.contains("script")) {
            executeScript(response["script"]);
        } else {
            std::cerr << "Unexpected response type or missing 'script' field" << std::endl;
            std::cerr << response.dump(2) << std::endl;
        }
    } catch(const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
