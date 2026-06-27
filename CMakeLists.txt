cmake_minimum_required(VERSION 3.21)
project(Cubed LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_CONFIGURATION_TYPES
   AND NOT CMAKE_BUILD_TYPE)

    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)

endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH 
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
)

include(Dependencies)

add_subdirectory(third_party/glad)

add_subdirectory(third_party/imgui)

add_executable(${PROJECT_NAME})

add_subdirectory(src)

file(GLOB_RECURSE PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/*.proto
)

protobuf_generate(
    TARGET ${PROJECT_NAME}           
    LANGUAGE cpp         
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/proto
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        $<$<CONFIG:Debug>:-g>
        #$<$<CONFIG:Debug>:-fsanitize=address>
        #$<$<CONFIG:Debug>:-fsanitize=thread>

        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wpedantic>

        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_link_options(${PROJECT_NAME}
    PRIVATE
        #$<$<CONFIG:Debug>:-fsanitize=address>
        #$<$<CONFIG:Debug>:-fsanitize=thread>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ASIO_STANDALONE
        ASIO_NO_DEPRECATED
        $<$<CONFIG:Debug>:DEBUG_MODE>
        $<$<CXX_COMPILER_ID:MSVC>:
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        >
)

if(CMAKE_CONFIGURATION_TYPES)
    # Visual Studio / Xcode multi-configuration generator
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            ASSETS_PATH="$<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/assets/>$<$<NOT:$<CONFIG:Debug>>:./assets/>"
    )
else()
    # Ninja / Makefiles single-configuration generator
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${PROJECT_NAME}
            PRIVATE
                ASSETS_PATH="${PROJECT_SOURCE_DIR}/assets/"
        )
    else()
        target_compile_definitions(${PROJECT_NAME}
            PRIVATE
                ASSETS_PATH="./assets/"
        )
    endif()
endif()


target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party/asio/include

        ${PROJECT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}/src

)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw                                   
        glad
        glm::glm
        OpenGL::GL
        soil2
        Freetype::Freetype
        tomlplusplus::tomlplusplus
        imgui
        tbb
        protobuf::libprotobuf
        absl::log
        absl::check
        zstd::zstd
        $<$<PLATFORM_ID:Windows>:ws2_32>

)

if(WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()