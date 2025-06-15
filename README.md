# Windows AI Agent

An advanced AI-powered Windows automation agent that uses Google's Gemini API to understand natural language commands and execute them safely on Windows systems.

## 🚀 Features

- **Natural Language Processing**: Communicate with your computer using plain English
- **Context Memory & Learning**: Remembers previous interactions and learns from executions
- **Advanced Task Planning**: Breaks down complex tasks into manageable steps
- **Multi-Modal Input Support**: Voice input and screenshot analysis capabilities
- **Safe Execution Environment**: Built-in safety checks and confirmation prompts
- **Session Management**: Saves and restores conversation history
- **Multiple Execution Modes**: Safe, Interactive, and Autonomous modes

## 📋 Prerequisites

- Windows 10/11
- CMake 3.10 or higher
- C++17 compatible compiler (Visual Studio 2017+ or MinGW)
- Google Gemini API key

## 🛠️ Installation

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/windows-ai-agent.git
cd windows-ai-agent
```

### 2. Install Dependencies

Using vcpkg (recommended):

```bash
vcpkg install nlohmann-json curl
```

### 3. Configure API Key

Copy the example configuration file and add your Gemini API key:

```bash
copy config.example.json config.json
copy config_advanced.example.json config_advanced.json
```

Edit `config.json` and `config_advanced.json` to add your Google Gemini API key:

```json
{
  "api_key": "YOUR_ACTUAL_GEMINI_API_KEY_HERE"
}
```

### 4. Build the Project

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 🚀 Usage

### Basic AI Agent

```bash
./windows_ai_agent.exe
```

### Advanced AI Agent (Recommended)

```bash
./windows_ai_agent_advanced.exe
```

## 🎮 Commands

### Basic Commands

- Type your request in natural language
- `quit`, `exit`, `q`, or `:quit` - Exit the application

### Advanced Commands (Advanced Agent Only)

- `:voice` - Enable voice input
- `:screenshot` - Analyze current screen
- `:history` - Show conversation history
- `:mode safe/interactive/autonomous` - Change execution mode

## ⚙️ Configuration

### Execution Modes

- **Safe Mode**: Asks for confirmation before any execution
- **Interactive Mode**: Shows execution plan and asks for confirmation
- **Autonomous Mode**: Executes commands automatically (use with caution)

### Safety Features

- Command filtering and blocking
- Execution timeouts
- Output size limits
- Sandbox mode support

## 🔧 Development

### Project Structure

```
src/
├── main.cpp                 # Basic AI agent
├── main_advanced.cpp        # Advanced AI agent with full features
├── gemini.cpp/.h           # Gemini API integration
├── executor.cpp/.h         # Basic script execution
├── context_manager.cpp/.h  # Context memory and history
├── task_planner.cpp/.h     # Task planning and analysis
├── advanced_executor.cpp/.h# Advanced execution with safety
└── multimodal_handler.cpp/.h# Voice and image input
```

### Building

The project uses CMake for building. Key dependencies:

- nlohmann-json for JSON parsing
- libcurl for HTTP requests to Gemini API

## 🛡️ Security

- Never commit your actual API keys to version control
- The agent includes safety checks for dangerous commands
- Use Safe or Interactive mode for untrusted operations
- Review execution plans before approval

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## ⚠️ Disclaimer

This software can execute system commands on your computer. Use with caution and always review commands before execution. The authors are not responsible for any damage caused by misuse of this software.

## 🆘 Support

If you encounter any issues or have questions:

1. Check the [Issues](https://github.com/yourusername/windows-ai-agent/issues) page
2. Create a new issue with detailed information about your problem
3. Include your configuration (without API keys) and error messages

## 🙏 Acknowledgments

- Google Gemini API for natural language processing
- nlohmann/json for JSON parsing
- libcurl for HTTP communication
