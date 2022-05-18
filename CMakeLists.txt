cmake_minimum_required(VERSION 3.12)
project(emscripten_webgl)
set(CMAKE_CXX_STANDARD 20)

if(NOT EMSCRIPTEN)
    find_package(OpenGL REQUIRED)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(glfw-3.2.1)
    add_subdirectory(glad)
endif()

include_directories(glfw-3.2.1/include)

add_executable(emscripten_webgl main.cpp)
if(NOT EMSCRIPTEN)
    target_link_libraries(emscripten_webgl glfw glad ${OPENGL_LIBRARIES})
endif()

if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s USE_WEBGL2=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s ASSERTIONS=1 -s USE_SDL=2")
    set_target_properties(emscripten_webgl
            PROPERTIES SUFFIX ".html"
            LINK_FLAGS " --bind -s USE_GLFW=3 -s WASM=1 -g4")
endif()