# 🤖 Windows AI Agent

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![React](https://img.shields.io/badge/React-19-blue.svg)](https://reactjs.org/)
[![Electron](https://img.shields.io/badge/Electron-Latest-47848f.svg)](https://electronjs.org/)

An advanced AI-powered Windows automation agent that uses Google's Gemini API to understand natural language commands and execute them safely on Windows systems. Features both a powerful C++ backend and a modern Electron-based frontend interface.

## ⚡ Quick Start

### For End Users (Recommended)

1. **Download** the latest release from [Releases](https://github.com/yourusername/windows-ai-agent/releases)
2. **Extract** and run `setup.exe`
3. **Configure** your Google Gemini API key in the settings
4. **Launch** the desktop app and start chatting with your AI assistant!

### For Developers

1. **Clone:** `git clone https://github.com/yourusername/windows-ai-agent.git`
2. **Backend:** `mkdir build && cd build && cmake .. && cmake --build .`
3. **Frontend:** `cd frontend && npm install && npm run dev`
4. **Configure:** Add your Gemini API key to `config_advanced.json`
5. **Run:** Start backend in server mode, then launch frontend

## 🚀 Features

### Core AI Capabilities

- **Natural Language Processing**: Communicate with your computer using plain English
- **Context Memory & Learning**: Remembers previous interactions and learns from executions
- **Advanced Task Planning**: Breaks down complex tasks into manageable steps
- **Multi-Modal Input Support**: Voice input and screenshot analysis capabilities
- **Safe Execution Environment**: Built-in safety checks and confirmation prompts
- **Session Management**: Saves and restores conversation history
- **Multiple Execution Modes**: Safe, Interactive, and Autonomous modes

### User Interfaces

- **Command Line Interface**: Traditional CLI for power users and automation
- **Modern Web UI**: Beautiful Electron-based desktop application with glassmorphism design
- **HTTP API**: RESTful API for integration with other applications
- **Real-time Communication**: Live updates and interactive feedback

## 🏗️ Architecture

### Core Components

| Component                                | Purpose                         | Language         | Key Features                              |
| ---------------------------------------- | ------------------------------- | ---------------- | ----------------------------------------- |
| **Basic Agent** (`main.cpp`)             | Simple CLI interface            | C++17            | Direct command execution, basic safety    |
| **Advanced Agent** (`main_advanced.cpp`) | Full-featured CLI + HTTP server | C++17            | All features, HTTP API, multi-modal       |
| **Frontend App** (`frontend/`)           | Modern desktop UI               | React + Electron | Chat interface, real-time updates         |
| **HTTP Server** (`http_server.cpp`)      | RESTful API backend             | C++17            | CORS support, JSON API endpoints          |
| **Context Manager**                      | Memory and learning             | C++17            | Session persistence, conversation history |
| **Task Planner**                         | AI task breakdown               | C++17            | Complex task analysis and planning        |
| **Executor**                             | Safe command execution          | C++17            | Multiple safety modes, rollback support   |
| **Multimodal Handler**                   | Voice/image processing          | C++17            | Speech recognition, image analysis        |

### Communication Flow

```
┌─────────────────┐    HTTP API    ┌──────────────────┐
│   Frontend UI   │ ◄────────────► │   HTTP Server    │
│   (Electron)    │                │   (Port 8080)    │
└─────────────────┘                └──────────────────┘
                                            │
                                            ▼
                                   ┌──────────────────┐
                                   │  Advanced Agent  │
                                   │     (Core AI)    │
                                   └──────────────────┘
                                            │
                                            ▼
                                   ┌──────────────────┐
                                   │   Gemini API     │
                                   │   (Google AI)    │
                                   └──────────────────┘
```

## 📋 Prerequisites

### Backend (C++)

- Windows 10/11
- CMake 3.10 or higher
- C++17 compatible compiler (Visual Studio 2017+ or MinGW)
- Google Gemini API key

### Frontend (Optional)

- Node.js 16 or higher
- npm or yarn package manager

## 🛠️ Installation

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/windows-ai-agent.git
cd windows-ai-agent
```

### 2. Install Dependencies

#### Backend Dependencies

Using vcpkg (recommended):

```bash
vcpkg install nlohmann-json curl
```

#### Frontend Dependencies (Optional)

```bash
cd frontend
npm install
```

### 3. Configure API Key

Copy the example configuration files and add your Gemini API key:

```bash
copy config.example.json config.json
copy config_advanced.example.json config_advanced.json
```

Edit `config.json` and `config_advanced.json` to add your Google Gemini API key:

```json
{
  "api_key": "YOUR_ACTUAL_GEMINI_API_KEY_HERE",
  "server_mode": false
}
```

For HTTP server mode (required for frontend), set `server_mode` to `true`:

```json
{
  "api_key": "YOUR_ACTUAL_GEMINI_API_KEY_HERE",
  "server_mode": true,
  "execution_mode": "safe",
  "enable_voice": false,
  "enable_image_analysis": false
}
```

### 4. Build the Project

#### Backend

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

#### Frontend (Optional)

```bash
cd frontend
npm run build
```

## 🚀 Usage

### Option 1: Modern Desktop Application (Recommended)

1. **Start the backend in server mode:**

   ```bash
   ./windows_ai_agent_advanced.exe
   ```

   The backend will start an HTTP server on port 8080.

2. **Launch the frontend:**
   ```bash
   cd frontend
   npm run dev
   ```
   This will open the modern Electron-based desktop application.

### Option 2: Command Line Interface

#### Basic AI Agent

```bash
./windows_ai_agent.exe
```

#### Advanced AI Agent

```bash
./windows_ai_agent_advanced.exe
```

### Option 3: HTTP API Integration

Start the backend in server mode and integrate with the RESTful API:

**Base URL:** `http://localhost:8080`

**Available Endpoints:**

- `POST /api/execute` - Execute AI tasks
- `GET /api/history` - Get conversation history
- `GET /api/system-info` - Get system information
- `POST /api/preferences` - Update user preferences
- `GET /api/processes` - Get active processes
- `POST /api/rollback` - Rollback last action
- `GET /api/suggestions` - Get AI suggestions
- `POST /api/voice` - Voice input processing
- `POST /api/image` - Image analysis

## 🎮 Commands

### Frontend Interface Commands

The modern desktop application provides:

- **Chat Interface**: Natural language conversation with the AI
- **Task History**: View previous conversations and executions
- **System Monitoring**: Real-time system information
- **Settings Panel**: Configure execution modes and preferences
- **Voice Input**: Click-to-speak functionality (when enabled)
- **Screenshot Analysis**: Drag-and-drop image analysis

### Command Line Interface

#### Basic Commands

- Type your request in natural language
- `quit`, `exit`, `q`, or `:quit` - Exit the application

#### Advanced Commands (CLI Only)

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
├── Backend (C++)
│   ├── main.cpp                 # Basic AI agent
│   ├── main_advanced.cpp        # Advanced AI agent with HTTP server
│   ├── gemini.cpp/.h           # Gemini API integration
│   ├── executor.cpp/.h         # Basic script execution
│   ├── context_manager.cpp/.h  # Context memory and history
│   ├── task_planner.cpp/.h     # Task planning and analysis
│   ├── advanced_executor.cpp/.h# Advanced execution with safety
│   ├── multimodal_handler.cpp/.h# Voice and image input
│   └── http_server.cpp/.h      # REST API server
├── Frontend (Electron + React)
│   ├── electron/               # Electron main process
│   ├── src/                   # React application source
│   │   ├── components/        # React components
│   │   ├── AIAgentService.js  # API client service
│   │   └── App.jsx           # Main application component
│   ├── package.json          # Node.js dependencies
│   └── vite.config.js        # Vite build configuration
├── Configuration
│   ├── config.json           # Basic agent configuration
│   ├── config_advanced.json  # Advanced agent configuration
│   └── vcpkg.json           # C++ dependencies
└── Build System
    ├── CMakeLists.txt        # CMake configuration
    ├── Makefile             # Alternative build system
    └── build/               # Build artifacts
```

### Technology Stack

#### Backend

- **Language**: C++17
- **Build System**: CMake
- **Dependencies**:
  - nlohmann-json for JSON parsing
  - libcurl for HTTP requests to Gemini API
  - Native Windows APIs for system interaction
- **Architecture**: Modular design with separate components for AI, execution, context management

#### Frontend

- **Framework**: Electron + React 19
- **Build Tool**: Vite 6
- **UI Library**: Lucide React (icons)
- **Styling**: Modern CSS with glassmorphism effects
- **Communication**: Axios for HTTP requests to backend
- **Development**: Hot reload, TypeScript-ready

### Building

The project uses CMake for the C++ backend and npm for the frontend:

#### Backend Dependencies

- nlohmann-json for JSON parsing
- libcurl for HTTP requests to Gemini API

#### Frontend Dependencies

- Electron for desktop application framework
- React for UI components
- Vite for fast development and building
- Axios for API communication

## 🛡️ Security

- Never commit your actual API keys to version control
- The agent includes safety checks for dangerous commands
- Use Safe or Interactive mode for untrusted operations
- Review execution plans before approval
- HTTP server includes CORS headers for secure web communication
- Frontend validates all user inputs before sending to backend
- Session data is stored locally and encrypted when possible

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines

#### Backend (C++)

- Follow modern C++17 standards
- Use RAII for resource management
- Add comprehensive error handling
- Include unit tests where applicable
- Document complex algorithms

#### Frontend (React)

- Use functional components with hooks
- Follow React best practices
- Maintain responsive design principles
- Add TypeScript types for better code quality
- Test UI components thoroughly

### Building for Development

#### Backend Development

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

#### Frontend Development

```bash
cd frontend
npm run dev  # Starts development server with hot reload
```

## ⚠️ Disclaimer

This software can execute system commands on your computer. Use with caution and always review commands before execution. The authors are not responsible for any damage caused by misuse of this software.

**Important Safety Notes:**

- Always use Safe or Interactive mode when testing new commands
- Review the execution plan before approving any system modifications
- Keep backups of important data before running automation tasks
- The AI agent has access to system APIs and can modify files, processes, and settings
- Voice and image analysis features may process sensitive information locally

## 🆘 Support

If you encounter any issues or have questions:

1. **Check the Documentation**: Review this README and the [BUILD.md](BUILD.md) guide
2. **Search Issues**: Check the [Issues](https://github.com/yourusername/windows-ai-agent/issues) page for similar problems
3. **Create an Issue**: Open a new issue with detailed information:
   - Your Windows version and system specifications
   - Whether you're using CLI or frontend interface
   - Backend/frontend versions and configuration (without API keys)
   - Complete error messages and logs
   - Steps to reproduce the issue

### Getting Help

- **Frontend Issues**: Include browser console logs and Electron version
- **Backend Issues**: Include CMake output and compilation errors
- **API Issues**: Verify your Gemini API key and network connectivity
- **Performance Issues**: Include system resource usage and task complexity

## 🌟 Roadmap

### Upcoming Features

- [ ] Plugin system for custom commands
- [ ] Multi-language support for international users
- [ ] Advanced scheduling and automation workflows
- [ ] Integration with popular productivity apps
- [ ] Mobile companion app for remote control
- [ ] Cloud synchronization for settings and history
- [ ] Advanced AI model selection (GPT, Claude, etc.)

### Recent Updates

- ✅ Modern Electron + React frontend with glassmorphism UI
- ✅ RESTful HTTP API for external integrations
- ✅ Real-time system monitoring and process management
- ✅ Enhanced safety features and execution modes
- ✅ Voice input and screenshot analysis capabilities

## 🙏 Acknowledgments

- **Google Gemini API** for natural language processing and AI capabilities
- **nlohmann/json** for robust JSON parsing and manipulation
- **libcurl** for reliable HTTP communication
- **Electron** for cross-platform desktop application framework
- **React** for modern, reactive user interface components
- **Vite** for fast development and optimized builds
- **Lucide React** for beautiful, consistent iconography

### Special Thanks

- The open-source community for invaluable libraries and tools
- Beta testers who provided feedback during development
- Contributors who helped improve documentation and code quality

---

**Made with ❤️ for Windows power users and automation enthusiasts**

For more detailed build instructions, see [BUILD.md](BUILD.md)  
For contribution guidelines, see [CONTRIBUTING.md](CONTRIBUTING.md)  
For version history, see [CHANGELOG.md](CHANGELOG.md)
