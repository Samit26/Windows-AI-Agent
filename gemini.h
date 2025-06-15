#ifndef GEMINI_H
#define GEMINI_H

#include "include/json.hpp"
#include <string>

using json = nlohmann::json;

// Sends the user prompt to Gemini and returns the parsed JSON response
json callGemini(const std::string &api_key, const std::string &user_prompt);

#endif // GEMINI_H