cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
project(replife)

add_executable(replife src/main.cpp)

add_subdirectory(third_party/imgui)
add_subdirectory(third_party/SDL)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

target_link_libraries(replife PRIVATE imgui "SDL3::SDL3-static")

target_compile_definitions(
	replife PRIVATE 
	RLE_DIR="../../patterns_rle"
	SHADER_DIR="../../shaders"
	SRC_DIR="../../src"
)