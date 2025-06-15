cmake_minimum_required(VERSION 3.10)
project(windows_ai_agent)
set(CMAKE_CXX_STANDARD 17)

# Use our locally built libcurl
set(CURL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libcurl_install/include")
set(CURL_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/libcurl_install/lib/libcurl.a")

add_executable(windows_ai_agent
    main.cpp
    gemini.cpp
    executor.cpp
)

# Add advanced AI agent executable
add_executable(windows_ai_agent_advanced
    main_advanced.cpp
    gemini.cpp
    executor.cpp
    context_manager.cpp
    task_planner.cpp
    advanced_executor.cpp
    multimodal_handler.cpp
)

target_include_directories(windows_ai_agent PRIVATE ${CURL_INCLUDE_DIR})
target_compile_definitions(windows_ai_agent PRIVATE CURL_STATICLIB)
target_link_libraries(windows_ai_agent PRIVATE ${CURL_LIBRARY})

# Configure advanced AI agent
target_include_directories(windows_ai_agent_advanced PRIVATE ${CURL_INCLUDE_DIR})
target_compile_definitions(windows_ai_agent_advanced PRIVATE CURL_STATICLIB)
target_link_libraries(windows_ai_agent_advanced PRIVATE ${CURL_LIBRARY})

# Link Windows libraries that libcurl needs
if(WIN32)
    target_link_libraries(windows_ai_agent PRIVATE ws2_32 wldap32 crypt32 winmm bcrypt)
    target_link_libraries(windows_ai_agent_advanced PRIVATE ws2_32 wldap32 crypt32 winmm bcrypt)
endif()