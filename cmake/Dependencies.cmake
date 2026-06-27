include(FetchContent)

# System packages
find_package(OpenGL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)
find_package(zstd REQUIRED)
if (UNIX AND NOT APPLE)
    find_package(Freetype REQUIRED)
    find_package(glfw3 REQUIRED)   
endif()


# Third-party libraries
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
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)


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
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

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
    set(_BUILD_SHARED_LIBS_SAVED ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS ON)

    FetchContent_Declare(
    onetbb                           
    GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
    GIT_TAG v2023.0.0
    )
    
    set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
    set(TBB_TEST OFF CACHE BOOL "Build TBB tests" FORCE)
    FetchContent_MakeAvailable(onetbb)

    set(BUILD_SHARED_LIBS ${_BUILD_SHARED_LIBS_SAVED})
    unset(_BUILD_SHARED_LIBS_SAVED)

endif()

