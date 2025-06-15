#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "include/json.hpp"
#include <string>

using json = nlohmann::json;

// Executes a PowerShell script represented as a JSON array of commands
void executeScript(const json &script_array);

#endif // EXECUTOR_H