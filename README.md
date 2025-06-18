# 🤖 Windows AI Agent

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![React](https://img.shields.io/badge/React-19-blue.svg)](https://reactjs.org/)
[![Electron](https://img.shields.io/badge/Electron-Latest-47848f.svg)](https://electronjs.org/)

An advanced AI-powered Windows automation agent that uses a powerful Large Language Model (currently DeepSeek R1 via OpenRouter API) to understand natural language commands, formulate complex multi-step plans (including dynamic content generation), and execute them on Windows systems using PowerShell and vision-guided UI automation. Features both a powerful C++ backend and a modern Electron-based frontend interface.

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

- **Natural Language Processing**: Communicate with your computer using plain English.
- **Dynamic Task Planning**: LLM generates complex, multi-step plans in JSON format, including sequences of PowerShell scripts, vision-guided actions, and content generation steps.
- **Orchestrated Content Generation**: Supports tasks requiring text generation by the LLM as an intermediate step (e.g., writing stories, emails, code snippets) and then using that content in subsequent actions.
- **Vision-Guided UI Automation**: Interacts with UI elements based on screen analysis. (Note: Core screen analysis is via Windows GDI; advanced element detection and OCR are placeholder functionalities requiring further development, potentially with OpenCV and Tesseract).
- **Multi-Modal Input Support**: Screenshot analysis is a core part of vision tasks. Voice input is currently a placeholder and requires integration with a speech-to-text service.
- **Safe Execution Environment**: Built-in safety checks and confirmation prompts for potentially risky operations.
- **Multiple Execution Modes**: Safe, Interactive, and Autonomous modes to control the agent's behavior.
- **HTTP API with Vision Endpoints**: Includes `/api/vision/analyzeScreen` for screen analysis and `/api/vision/executeAction` for vision-guided commands.

### User Interfaces

- **Command Line Interface**: Traditional CLI for power users and automation
- **Modern Web UI**: Beautiful Electron-based desktop application with glassmorphism design
- **HTTP API**: RESTful API for integration with other applications
- **Real-time Communication**: Live updates and interactive feedback

## 🏗️ Architecture

### Core Components

| Component                                 | Purpose                                                                 | Language         | Key Features                                                                 |
| ----------------------------------------- | ----------------------------------------------------------------------- | ---------------- | ---------------------------------------------------------------------------- |
| **AI Agent** (`main_advanced.cpp`)        | Main application logic, CLI, and overall orchestration.                 | C++17            | Initializes components, handles user input loop, manages execution modes.    |
| **Frontend App** (`frontend/`)            | Modern desktop UI for interaction.                                      | React + Electron | Chat interface, settings, real-time updates (connects to HTTP Server).       |
| **HTTP Server** (`http_server.cpp`)       | Provides a RESTful API for the frontend and external integrations.      | C++17            | Uses `httplib.h`. Endpoints for task execution, vision analysis, etc.        |
| **AI Model** (`ai_model.cpp`)             | Interface to the Large Language Model (LLM).                            | C++17            | Constructs prompts, calls OpenRouter API (DeepSeek R1), parses responses.    |
| **Task Planner** (`task_planner.cpp`)     | Interprets LLM-generated plans and orchestrates execution steps.        | C++17            | Handles multi-step plans, content generation requests, and sequences tasks.  |
| **Advanced Executor** (`advanced_executor.cpp`) | Executes planned tasks, manages safety, and integrates vision.    | C++17            | PowerShell execution, calls VisionGuidedExecutor, safety checks.             |
| **Vision-Guided Executor** (`vision_guided_executor.cpp`) | Orchestrates vision-based UI automation tasks.          | C++17            | Uses VisionProcessor for screen analysis and UI interaction.                 |
| **Vision Processor** (`vision_processor.cpp`) | Handles screen capture, basic UI element detection, and OS interaction. | C++17            | GDI for screenshots, Windows API for clicks/typing. (OpenCV optional).     |
| **Multimodal Handler** (`multimodal_handler.cpp`) | Manages different input types (text, voice placeholder, image analysis). | C++17            | Integrates VisionProcessor for screen analysis. Voice/Image analysis are placeholders. |

### Communication Flow

```
┌─────────────────┐      HTTP API      ┌──────────────────┐
│   Frontend UI   │ ◄────────────────► │   HTTP Server    │
│   (Electron)    │                    │   (Port 8080)    │
└─────────────────┘                    └──────────────────┘
                                              │ ▲
                                              │ │ (API Calls)
                                              ▼ │
                                     ┌──────────────────┐
                                     │  Advanced Agent  │
                                     │ (main_advanced.cpp)│
                                     └──────────────────┘
                                       │    │        ▲
                  (Task Execution)   │    │        │ (LLM Results)
                                     ▼    │        │
                          ┌─────────────────┴───────────┐
                          │ TaskPlanner, AdvancedExecutor │
                          │ VisionGuidedExecutor,       │
                          │ MultiModalHandler           │
                          └─────────────────┬───────────┘
                                     │    │        ▲
              (Screen Analysis, UI)  │    │        │ (Prompts)
                                     ▼    │        │
                            ┌─────────────────┴───────────┐
                            │ VisionProcessor, AIModel    │
                            └─────────────────┬───────────┘
                                              │
                                              ▼
                                     ┌──────────────────┐
                                     │ OpenRouter API   │
                                     │ (DeepSeek R1 LLM)│
                                     └──────────────────┘
```
(The communication flow diagram is updated to reflect the new components and interactions.)

## 📋 Prerequisites

### Backend (C++)

- Windows 10/11
- CMake 3.10 or higher
- C++17 compatible compiler (Visual Studio 2017+ or MinGW)
- OpenRouter API key (with access to a model like DeepSeek R1)

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
(Note: OpenCV can be optionally installed via `vcpkg install opencv4` if you wish to enable its features in `VisionProcessor`, but it's not required for core functionality as the code uses it conditionally.)

#### Frontend Dependencies (Optional)

```bash
cd frontend
npm install
```

### 3. Configure API Key

Create a file named `config_advanced.json` in the root directory of the project. If an example file like `config_advanced.example.json` exists, you can copy and rename it.

Edit `config_advanced.json` to add your OpenRouter API key and other settings:

```json
{
  "api_key": "YOUR_OPENROUTER_API_KEY_HERE",
  "server_mode": false, // Set to true to enable HTTP server for frontend
  "execution_mode": "interactive", // "safe", "interactive", or "autonomous"
  "enable_voice": false, // Voice input is currently a placeholder
  "enable_image_analysis": false // Image analysis for non-screenshot inputs is placeholder
}
```
(Ensure the project now exclusively uses `config_advanced.json` for configuration.)

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

### Command Line Interface

```bash
./windows_ai_agent_advanced.exe
```

./windows_ai_agent_advanced.exe

```

### Option 3: HTTP API Integration

Start the backend in server mode and integrate with the RESTful API:

**Base URL:** `http://localhost:8080`

**Available Endpoints:**

- `POST /api/execute` - Execute AI tasks based on natural language input.
  - Request Body: `{ "input": "your task description", "mode": "agent" }` (mode can be "agent" or "chatbot")
  - Response: JSON with execution results or AI's textual response.
- `GET /api/system-info` - Get system information (e.g., current execution mode).
- `POST /api/preferences` - Update user preferences (e.g., execution mode).
- `GET /api/processes` - Get active processes (placeholder, current implementation might be basic).
- `POST /api/rollback` - Rollback last action (placeholder).
- `GET /api/suggestions` - Get AI suggestions for improvements (placeholder).
- `POST /api/voice` - Voice input processing (placeholder, requires speech-to-text integration).
- `POST /api/image` - Image analysis for uploaded images (placeholder, requires vision model integration).
- `POST /api/vision/analyzeScreen` - Triggers a detailed screen analysis.
  - Request Body: (Empty)
  - Response: JSON containing `screenshot_path`, `application_name`, `window_title`, `overall_description`, and an array of detected `elements` with their properties (coordinates, type, text, confidence).
- `POST /api/vision/executeAction` - Executes a vision-guided action based on a natural language description.
  - Request Body: `{ "action_description": "click the button labeled 'Submit'" }`
  - Response: JSON `ExecutionResult` (success, output, error_message, metadata including vision steps).

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

- `:voice` - Enable voice input (currently a placeholder, requires speech-to-text setup).
- `:screenshot` - Captures and analyzes the current screen, printing a description.
- `:mode safe/interactive/autonomous` - Change the agent's execution mode.

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
│ ├── main_advanced.cpp         # AI agent CLI, server mode logic, main orchestration
│ ├── ai_model.cpp/.h           # Interface to LLM (OpenRouter/DeepSeek R1)
│ ├── task_planner.cpp/.h       # Interprets LLM plans, orchestrates multi-step tasks & content generation
│ ├── advanced_executor.cpp/.h  # Executes tasks, manages safety, integrates vision execution
│ ├── vision_guided_executor.cpp/.h # Orchestrates vision-based UI automation sequences
│ ├── vision_processor.cpp/.h   # Screen capture, basic UI element detection, OS interaction
│ ├── multimodal_handler.cpp/.h # Manages different input types (text, voice placeholder, image analysis)
│ └── http_server.cpp/.h        # REST API server using httplib.h
├── Frontend (Electron + React)
│ ├── electron/ # Electron main process
│ ├── src/ # React application source
│ │ ├── components/ # React components
│ │ ├── AIAgentService.js # API client service
│ │ └── App.jsx # Main application component
│ ├── package.json # Node.js dependencies
│ └── vite.config.js # Vite build configuration
├── Configuration
│ ├── config_advanced.json # Agent configuration
│ └── vcpkg.json # C++ dependencies
└── Build System
├── CMakeLists.txt # CMake configuration
├── Makefile # Alternative build system
└── build/ # Build artifacts

````

### Technology Stack

#### Backend

- **Language**: C++17
- **Build System**: CMake
- **Dependencies**:
  - nlohmann-json for JSON parsing.
  - libcurl for HTTP requests to the OpenRouter API.
  - httplib.h for the C++ HTTP server.
  - Native Windows APIs for system interaction (GDI for basic screenshots, input simulation, window management).
  - OpenCV (optional, for advanced image processing features in `VisionProcessor` if enabled and installed).
- **Architecture**: Modular design with components for AI interaction, planning, execution (PowerShell, Vision), and multimodal input handling.

#### Frontend

- **Framework**: Electron + React 19
- **Build Tool**: Vite 6
- **UI Library**: Lucide React (icons)
- **Styling**: Modern CSS with glassmorphism effects
- **Communication**: Axios for HTTP requests to backend
- **Development**: Hot reload, TypeScript-ready

### Building

The project uses CMake for the C++ backend and npm for the frontend.

#### Backend Dependencies (managed via `vcpkg.json`)

- **nlohmann-json**: For JSON parsing and manipulation.
- **libcurl**: For making HTTP requests to the OpenRouter API.
- **httplib.h**: (Typically included as a header-only library, but if managed via vcpkg, list here). The project now uses this for its HTTP server.
- **OpenCV**: (Optional) For advanced vision processing features. Not included in the default `vcpkg.json` but can be added if needed.

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
````

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

1. **Search Issues**: Check the [Issues](https://github.com/yourusername/windows-ai-agent/issues) page for similar problems.
2. **Create an Issue**: Open a new issue with detailed information:
   - Your Windows version and system specifications.
   - Whether you're using CLI or frontend interface.
   - Backend/frontend versions and `config_advanced.json` settings (excluding API keys).
   - Complete error messages and logs.
   - Steps to reproduce the issue.

### Getting Help

- **Frontend Issues**: Include browser console logs and Electron version.
- **Backend Issues**: Include console output, CMake output, and compilation errors if applicable.
- **API Issues**: Verify your OpenRouter API key and network connectivity.
- **Performance Issues**: Include system resource usage and task complexity.

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
- ✅ Voice input (placeholder) and screenshot analysis capabilities.
- ✅ New `generate_content_and_execute` and `multi_step_plan` task types for complex planning.
- ✅ HTTP API now uses `httplib.h` and includes vision endpoints (`/api/vision/analyzeScreen`, `/api/vision/executeAction`).
- ✅ Removed `ContextManager` and `Executor` components, streamlining focus on `AdvancedExecutor` and LLM-driven planning.

## 🙏 Acknowledgments

- **OpenRouter API (and models like DeepSeek R1)** for providing access to powerful LLMs for natural language processing, planning, and AI capabilities.
- **nlohmann/json** for robust JSON parsing and manipulation.
- **libcurl** for reliable HTTP communication.
- **cpp-httplib/httplib.h** for the C++ HTTP server implementation.
- **Electron** for cross-platform desktop application framework.
- **React** for modern, reactive user interface components.
- **Vite** for fast development and optimized builds.
- **Lucide React** for beautiful, consistent iconography.

### Special Thanks

- The open-source community for invaluable libraries and tools.
- Beta testers who provided feedback during development.
- Contributors who helped improve documentation and code quality.

---

**Made with ❤️ for Windows power users and automation enthusiasts**
