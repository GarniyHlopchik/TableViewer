cmake_minimum_required(VERSION 3.15)
project(WinAPIProject CXX)

# Write compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

 # TableViewer exe ----------

set(TABLEVIEWER_SOURCES
    win_classes/dialogue_window.cpp
    win_classes/window.cpp
    table_view/table_model.cpp
    table_view/table_window.cpp
    saveload.cpp
    main.cpp
)

# Create the executable target
add_executable(table_viewer WIN32 ${TABLEVIEWER_SOURCES})

# Add UNICODE and _UNICODE definitions
target_compile_definitions(table_viewer PRIVATE UNICODE _UNICODE)

target_include_directories(table_viewer PRIVATE win_classes)
target_include_directories(table_viewer PRIVATE table_view)

# Link the required Windows API libraries
target_link_libraries(table_viewer PRIVATE gdi32 comctl32 comdlg32)


if(MSVC)
    target_compile_options(table_viewer PRIVATE /utf-8)
endif()
