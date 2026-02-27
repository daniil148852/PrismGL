cmake_minimum_required(VERSION 3.18.1)

project(PrismGL VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -fomit-frame-pointer -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -DDEBUG")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")

set(PRISMGL_SOURCES
    src/prismgl_core.c
    src/shader_cache.c
    src/shader_translator.c
    src/gpu_detect.c
    src/gl_wrapper.c
    src/proc_address.c
    src/jni_bridge.c
)

add_library(PrismGL SHARED ${PRISMGL_SOURCES})

target_include_directories(PrismGL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(PrismGL
    android
    log
    GLESv3
    EGL
    dl
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(PrismGL PROPERTIES
        LINK_FLAGS "-Wl,--gc-sections -Wl,--strip-all"
    )
endif()
