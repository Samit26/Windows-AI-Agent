cmake_minimum_required(VERSION 3.10)
project(windows_ai_agent)
set(CMAKE_CXX_STANDARD 17)

# Include vcpkg toolchain if available
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_TOOLCHAIN_FILE})
else()
    # Set vcpkg installation path for finding packages
    set(CMAKE_PREFIX_PATH "D:/vcpkg_installed/x64-windows" ${CMAKE_PREFIX_PATH})
    message(STATUS "Using vcpkg installed packages at: D:/vcpkg_installed/x64-windows")
endif()

# Add include directory for local headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find required packages
find_package(PkgConfig QUIET)

# Try to find nlohmann_json
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json via vcpkg")
else()
    message(STATUS "Using local nlohmann_json header")
endif()

# Force MSVC compiler usage for proper OpenCV compatibility
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "MinGW/GCC compiler detected. This project requires MSVC (Visual Studio) compiler for OpenCV compatibility.")
    message(FATAL_ERROR "Please use Visual Studio 2019 build tools or configure CMake to use MSVC compiler.")
    message(FATAL_ERROR "Run: cmake -G \"Visual Studio 16 2019\" -A x64 ..")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Detected MSVC compiler - Good for OpenCV compatibility")
else()
    message(WARNING "Detected compiler: ${CMAKE_CXX_COMPILER_ID}")
    message(WARNING "This project is optimized for MSVC. Other compilers may have issues.")
endif()

# OpenCV is REQUIRED - not optional
message(STATUS "OpenCV is required for this project. Searching for OpenCV...")

# Try to find OpenCV first in the system
find_package(OpenCV QUIET)

if(NOT OpenCV_FOUND)
    # Try with your specified path
    set(OpenCV_DIR "D:/OpenCV/opencv/build")
    list(APPEND CMAKE_PREFIX_PATH "D:/OpenCV/opencv/build")
    find_package(OpenCV QUIET)
    
    if(NOT OpenCV_FOUND)
        # Try with vc16 path (Visual Studio 2019)
        set(OpenCV_DIR "D:/OpenCV/opencv/build/x64/vc16/lib")
        find_package(OpenCV QUIET)
        
        if(NOT OpenCV_FOUND)
            # Try alternative vc16 paths
            set(OpenCV_DIR "D:/OpenCV/opencv/build/x64/vc16")
            find_package(OpenCV QUIET)
        endif()
    endif()
endif()

if(OpenCV_FOUND)
    message(STATUS "SUCCESS: Found OpenCV ${OpenCV_VERSION}")
    message(STATUS "OpenCV installation path: ${OpenCV_DIR}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "OpenCV NOT FOUND!")
    message(FATAL_ERROR "Please ensure OpenCV is properly installed with Visual Studio 2019 support.")
    message(FATAL_ERROR "Expected path: D:/OpenCV/opencv/build/x64/vc16/")
    message(FATAL_ERROR "Download from: https://opencv.org/releases/ (Windows version)")
endif()

# Try to find libcurl with vcpkg support
list(APPEND CMAKE_PREFIX_PATH "D:/vcpkg_installed/x64-windows")
find_package(CURL QUIET)

if(CURL_FOUND)
    message(STATUS "Found libcurl via vcpkg: ${CURL_VERSION_STRING}")
    message(STATUS "CURL include dirs: ${CURL_INCLUDE_DIRS}")
    message(STATUS "CURL libraries: ${CURL_LIBRARIES}")
    set(CURL_INCLUDE_DIR ${CURL_INCLUDE_DIRS})
    set(CURL_LIBRARY ${CURL_LIBRARIES})
else()
    # Manual vcpkg path setup
    set(CURL_INCLUDE_DIR "D:/vcpkg_installed/x64-windows/include")
    set(CURL_LIBRARY "D:/vcpkg_installed/x64-windows/lib/libcurl.lib")
    
    if(EXISTS ${CURL_LIBRARY} AND EXISTS ${CURL_INCLUDE_DIR})
        message(STATUS "Found libcurl manually in vcpkg: ${CURL_LIBRARY}")
        set(CURL_LIBRARIES ${CURL_LIBRARY})
        set(CURL_FOUND TRUE)
    else()
        message(FATAL_ERROR "libcurl not found in vcpkg installation!")
        message(FATAL_ERROR "Expected: ${CURL_LIBRARY}")
    endif()
endif()

# Advanced AI agent executable (main and only executable)
add_executable(windows_ai_agent_advanced
    main_advanced.cpp
    ai_model.cpp
    task_planner.cpp
    advanced_executor.cpp
    multimodal_handler.cpp
    http_server.cpp
    vision_processor.cpp
    vision_guided_executor.cpp
)

# Configure the AI agent executable with required OpenCV and libcurl
target_include_directories(windows_ai_agent_advanced PRIVATE ${OpenCV_INCLUDE_DIRS})
target_compile_definitions(windows_ai_agent_advanced PRIVATE OPENCV_AVAILABLE)
target_link_libraries(windows_ai_agent_advanced PRIVATE ${OpenCV_LIBS})
message(STATUS "OpenCV features enabled and linked successfully")

# Link libcurl 
target_include_directories(windows_ai_agent_advanced PRIVATE ${CURL_INCLUDE_DIR})
if(WIN32)
    target_compile_definitions(windows_ai_agent_advanced PRIVATE CURL_STATICLIB)
endif()
target_link_libraries(windows_ai_agent_advanced PRIVATE ${CURL_LIBRARIES})

# Link Windows libraries that libcurl and the application need
target_link_libraries(windows_ai_agent_advanced PRIVATE ws2_32 wldap32 crypt32 winmm bcrypt gdi32 user32 psapi)

message(STATUS "libcurl features enabled and linked successfully")