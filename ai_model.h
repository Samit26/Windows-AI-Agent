#ifndef AI_MODEL_H
#define AI_MODEL_H

#include "include/json.hpp"
#include <string>

using json = nlohmann::json;

// Sends the user prompt to DeepSeek via OpenRouter and returns the parsed JSON response
json callAIModel(const std::string &api_key, const std::string &user_prompt);

// Vision-specific AI model call that returns vision action JSON format
json callVisionAIModel(const std::string &api_key, const std::string &vision_prompt);

// Dynamic intent analysis - replaces hardcoded task parsing with AI
json callIntentAI(const std::string &api_key, const std::string &user_request);

// Dynamic vision analysis - AI-driven element selection and action planning
json callVisionAI(const std::string &api_key, const std::string &task, const std::string &screen_description, const std::vector<std::string> &available_elements);

#endif // AI_MODEL_H