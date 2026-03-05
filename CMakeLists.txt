cmake_minimum_required(VERSION 3.14...3.24)
project(Cubed LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(Wayland REQUIRED wayland-client wayland-egl)
find_package(glfw3 REQUIRED)   


add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

include(FetchContent)

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
    src/tools/shader_tools.cpp
    src/tools/log.cpp
    src/camera.cpp
   
)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${EGL_LIBRARIES}              
        ${Wayland_LIBRARIES}
        glfw                                   
        glad
        glm::glm
        OpenGL::GL
        soil2
)


target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${EGL_INCLUDE_DIRS}
        ${Wayland_INCLUDE_DIRS}
)


target_compile_options(${PROJECT_NAME} PRIVATE ${EGL_CFLAGS_OTHER} ${Wayland_CFLAGS_OTHER})



add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/src/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "Copying shaders to output directory"
)