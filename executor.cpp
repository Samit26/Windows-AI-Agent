#include "executor.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>

void executeScript(const json &script_array) {
    std::filesystem::create_directories("scripts");
    std::ofstream script_file("scripts/temp.ps1");
    if(!script_file.is_open()) {
        throw std::runtime_error("Failed to create PowerShell script file");
    }

    for(const auto &line : script_array) {
        script_file << line.get<std::string>() << std::endl;
    }
    script_file.close();    // Execute the PowerShell script
    std::string script_path = std::filesystem::absolute("scripts/temp.ps1").string();
    std::string command = "powershell.exe -ExecutionPolicy Bypass -File \"" + script_path + "\"";
    int result = system(command.c_str());
    if(result != 0) {
        std::cerr << "PowerShell script execution failed with code " << result << std::endl;
    }
}