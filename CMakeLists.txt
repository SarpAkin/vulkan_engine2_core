cmake_minimum_required(VERSION 3.20)

project(vke)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Vulkan REQUIRED)

file(GLOB LIB_CPPS "libs/*.c*")

file(GLOB_RECURSE VKE_SRC_FILES "src/*.cpp")

set(VKE_INCLUDE "include")

add_library(vke 
    ${VKE_SRC_FILES}
    ${LIB_CPPS}
)

target_compile_options(vke PUBLIC -fPIC)

target_link_libraries(vke PUBLIC
    ${Vulkan_LIBRARIES}

)



target_include_directories(vke PUBLIC ${Vulkan_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/libs/ ${VKE_INCLUDE})
target_include_directories(vke INTERFACE ${VKE_INCLUDE})

# target_compile_options(vke PUBLIC
#     -DGLM_FORCE_DEPTH_ZERO_TO_ONE
#     -DGLM_FORCE_RADIANS
# )

