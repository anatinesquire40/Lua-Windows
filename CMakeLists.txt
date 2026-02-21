cmake_minimum_required(VERSION 3.16)
project("LuIbexWin" LANGUAGES CXX ASM_MASM)

file(GLOB CPP_SOURCES "Src/*.cpp")
file(GLOB HEADER_SOURCES "Include/*.h")
set(PRECOMPILED_HEADER Include/luibexwin.h)
add_library(luibexwin SHARED ${HEADER_SOURCES} ${CPP_SOURCES})

target_compile_features(luibexwin PRIVATE cxx_std_20)
target_precompile_headers(luibexwin PRIVATE ${PRECOMPILED_HEADER})

target_link_libraries(luibexwin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lua/lib/lua.lib")
target_include_directories(luibexwin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lua/include")