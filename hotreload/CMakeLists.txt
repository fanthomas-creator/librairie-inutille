cmake_minimum_required(VERSION 3.16)
project(hotreload VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(HR_BUILD_EXAMPLES "Build examples" ON)
option(HR_BUILD_SHARED   "Build as shared library" ON)

set(HR_SOURCES
    src/core/hr_engine.c
    src/core/hr_watcher.c
    src/core/hr_differ.c
    src/core/hr_loader.c
    src/core/hr_patcher.c
    src/core/hr_symbols.c
    src/adapters/hr_adapter_c.c
    src/adapters/hr_adapter_cpp.c
    src/adapters/hr_adapter_rust.c
    src/adapters/hr_adapter_zig.c
    src/adapters/hr_adapter_go.c
    src/adapters/hr_adapter_registry.c
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND HR_SOURCES src/platform/hr_platform_linux.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND HR_SOURCES src/platform/hr_platform_mac.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND HR_SOURCES src/platform/hr_platform_windows.c)
endif()

if(HR_BUILD_SHARED)
    add_library(hotreload SHARED ${HR_SOURCES})
    target_compile_definitions(hotreload PRIVATE HR_BUILD_DLL)
else()
    add_library(hotreload STATIC ${HR_SOURCES})
endif()

target_include_directories(hotreload PUBLIC include)
target_include_directories(hotreload PRIVATE src)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(hotreload PRIVATE dl)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(hotreload PRIVATE dl)
endif()

if(MSVC)
    target_compile_options(hotreload PRIVATE /W4)
else()
    target_compile_options(hotreload PRIVATE -Wall -Wextra -fvisibility=hidden)
endif()

if(HR_BUILD_EXAMPLES)
    add_subdirectory(examples/demo_c)
endif()

install(TARGETS hotreload
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)
install(FILES include/hotreload.h DESTINATION include)
