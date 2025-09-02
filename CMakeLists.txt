cmake_minimum_required(VERSION 3.30)

project(vke)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON CACHE BOOL "" FORCE)


find_package(Vulkan REQUIRED)

file(GLOB LIB_CPPS "libs/*.c*")

file(GLOB_RECURSE VKE_SRC_FILES "src/*.cpp")

set(VKE_INCLUDE "include")

add_library(vke 
    ${VKE_SRC_FILES}
    ${LIB_CPPS}
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SHADERC REQUIRED shaderc)

# Set position-independent code for the target
set_target_properties(vke PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(vke PUBLIC
    ${Vulkan_LIBRARIES}
    ${SHADERC_LIBRARIES}
)

target_include_directories(vke PUBLIC 
    ${Vulkan_INCLUDE_DIRS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ 
    ${VKE_INCLUDE} 
    ${SHADERC_INCLUDE_DIRS} 
    PRIVATE
    pch/
)


# Precompiled headers
target_precompile_headers(vke PRIVATE 
    "$<$<COMPILE_LANGUAGE:CXX>:pch.hpp>"
)


target_include_directories(vke INTERFACE 
    ${VKE_INCLUDE} 
)


if(CMAKE_BUILD_TYPE STREQUAL "DebugOnlyEngine")
    target_compile_options(vke PRIVATE -O3 -g)
endif()