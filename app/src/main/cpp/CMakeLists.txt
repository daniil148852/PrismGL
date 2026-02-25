cmake_minimum_required(VERSION 3.22.1)

project("PrismGL")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/renderer)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/jni)

add_library(prismgl SHARED
    jni/PrismGLJNI.cpp
    renderer/PrismGLRenderer.cpp
    renderer/ShaderManager.cpp
    renderer/TextureManager.cpp
    renderer/DrawCallOptimizer.cpp
    renderer/AdaptiveRenderer.cpp
    gl/GLWrapper.cpp
    gl/GLESToGLConverter.cpp
    gl/VulkanBridge.cpp
    util/Logger.cpp
    util/FileCache.cpp
)

find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)
find_library(Vulkan-lib vulkan)

target_link_libraries(prismgl
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib}
    ${Vulkan-lib}
    android
)

target_compile_options(prismgl PRIVATE
    -Wall -Wextra
    -O3 -ffast-math
    -funroll-loops
)
