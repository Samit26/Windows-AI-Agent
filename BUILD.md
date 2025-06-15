# Windows AI Agent - Build Instructions

## Prerequisites

### Required Software

- **CMake 3.10+**: Download from [cmake.org](https://cmake.org/download/)
- **C++ Compiler**:
  - Visual Studio 2017+ (recommended for Windows)
  - Or MinGW-w64
- **vcpkg** (recommended for dependencies): [Installation Guide](https://vcpkg.io/en/getting-started.html)

### Required Dependencies

- `nlohmann-json`: JSON parsing library
- `libcurl`: HTTP client library for API communication

## Quick Start

### Option 1: Using vcpkg (Recommended)

1. **Install vcpkg**:

   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

2. **Install dependencies**:

   ```powershell
   .\vcpkg install nlohmann-json:x64-windows
   .\vcpkg install curl:x64-windows
   ```

3. **Configure CMake with vcpkg**:

   ```powershell
   cd path\to\windows-ai-agent
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=path\to\vcpkg\scripts\buildsystems\vcpkg.cmake
   ```

4. **Build**:
   ```powershell
   cmake --build . --config Release
   ```

### Option 2: Manual Dependency Management

If you prefer to manage dependencies manually, the project includes configuration for building with locally installed libcurl.

## Configuration

1. **Copy configuration templates**:

   ```powershell
   copy config.example.json config.json
   copy config_advanced.example.json config_advanced.json
   ```

2. **Get a Gemini API key**:

   - Visit [Google AI Studio](https://makersuite.google.com/app/apikey)
   - Create a new API key
   - Replace `YOUR_GEMINI_API_KEY_HERE` in your config files

3. **Update configuration files** with your API key and preferred settings.

## Build Targets

The project builds two executables:

- **`windows_ai_agent.exe`**: Basic version with core functionality
- **`windows_ai_agent_advanced.exe`**: Full-featured version with context memory, task planning, and multimodal support

## Troubleshooting

### Common Issues

1. **CMake can't find dependencies**:

   - Ensure vcpkg is properly integrated
   - Check that the toolchain file path is correct
   - Verify dependencies are installed for the correct architecture

2. **Linker errors with libcurl**:

   - Make sure you're linking against the static version
   - Check that all required Windows libraries are linked (ws2_32, wldap32, etc.)

3. **JSON parsing errors**:
   - Verify nlohmann-json is properly installed
   - Check include paths in CMakeLists.txt

### Build Verification

After successful build, test the executables:

```powershell
# Test basic version
.\windows_ai_agent.exe

# Test advanced version
.\windows_ai_agent_advanced.exe
```

Both should start and prompt for your task input.

## Development Setup

For development, consider:

1. **IDE Setup**: Visual Studio or VS Code with C++ extensions
2. **Debug Builds**: Use `-DCMAKE_BUILD_TYPE=Debug` for debugging
3. **Code Style**: Follow the existing code style in the project

## Performance Notes

- Release builds are significantly faster
- The advanced version uses more memory due to context management
- First API call may be slower due to SSL handshake
