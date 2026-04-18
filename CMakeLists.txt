cmake_minimum_required(VERSION 3.14...3.24)
project(Cubed LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(OpenGL REQUIRED)

if (UNIX AND NOT APPLE)
    find_package(Freetype REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(EGL REQUIRED egl)
    pkg_check_modules(Wayland REQUIRED wayland-client wayland-egl)
    find_package(glfw3 REQUIRED)   
endif()

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

include(FetchContent)

if (WIN32)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4  
    )

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)         
    set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)     
    set(GLFW_STATIC ON CACHE BOOL "" FORCE)          

    FetchContent_MakeAvailable(glfw)

    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
        GIT_TAG VER-2-14-3
    )
    FetchContent_MakeAvailable(freetype)
    if(TARGET freetype)
    add_library(Freetype::Freetype ALIAS freetype)
    endif()
endif()

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3 
    
)
FetchContent_MakeAvailable(glm)
FetchContent_Declare(
    soil2
    GIT_REPOSITORY https://github.com/SpartanJ/SOIL2.git
    GIT_TAG 1.31  
    
)
FetchContent_MakeAvailable(soil2)

set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/app.cpp
    src/debug_collector.cpp
    src/camera.cpp
    src/gameplay/biome.cpp
    src/gameplay/chunk.cpp
    src/gameplay/player.cpp
    src/gameplay/tree.cpp
    src/gameplay/world.cpp
    src/input.cpp
    src/map_table.cpp
    src/renderer.cpp
    src/shader.cpp
    src/texture_manager.cpp
    src/tools/cubed_random.cpp
    src/tools/math_tools.cpp
    src/tools/shader_tools.cpp
    src/tools/font.cpp
    src/tools/log.cpp
    src/tools/perlin_noise.cpp
    src/ui/text.cpp
    src/window.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building with AddressSanitizer enabled for target: ${PROJECT_NAME}")

    target_compile_options(${PROJECT_NAME} PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer   
        -g                        
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        -fsanitize=address
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets/"
    )
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        ASSETS_PATH="./assets/"
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw                                   
        glad
        glm::glm
        OpenGL::GL
        soil2
        Freetype::Freetype
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(${PROJECT_NAME} PRIVATE tbb)
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            ${EGL_LIBRARIES}
            ${Wayland_LIBRARIES}
    )


    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${EGL_INCLUDE_DIRS}
            ${Wayland_INCLUDE_DIRS}
    )


    target_compile_options(${PROJECT_NAME} PRIVATE ${EGL_CFLAGS_OTHER} ${Wayland_CFLAGS_OTHER})
endif()
