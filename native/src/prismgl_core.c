/*
 * PrismGL Core - Main initialization and configuration
 */

#include "prismgl.h"
#include "gpu_detect.h"
#include "shader_translator.h"

#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#define LOG_TAG "PrismGL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static PrismGLConfig g_config;
static GPUInfo g_gpu_info;
static bool g_initialized = false;

bool prismgl_init(const char* cache_dir) {
    if (g_initialized) {
        LOGW("PrismGL already initialized");
        return true;
    }

    LOGI("PrismGL initializing...");

    /* Set default config */
    memset(&g_config, 0, sizeof(PrismGLConfig));
    g_config.shader_cache_enabled = true;
    g_config.draw_call_batching = true;
    g_config.adaptive_resolution = true;
    g_config.async_texture_loading = true;
    g_config.vulkan_backend = false;
    g_config.resolution_scale = 1.0f;
    g_config.max_cached_shaders = 1024;

    if (cache_dir) {
        strncpy(g_config.cache_dir, cache_dir, sizeof(g_config.cache_dir) - 1);
    }

    /* Detect GPU */
    g_gpu_info = gpu_detect();
    g_config.gpu_vendor = g_gpu_info.vendor;
    g_config.resolution_scale = g_gpu_info.recommended_resolution_scale;

    LOGI("GPU: %s (%s)", g_gpu_info.renderer_string, g_gpu_info.vendor_string);
    LOGI("GPU Tier: %d, Recommended scale: %.2f",
         g_gpu_info.tier, g_gpu_info.recommended_resolution_scale);

    /* Apply GPU-specific optimizations */
    gpu_apply_optimizations(&g_gpu_info);

    /* Initialize shader cache */
    if (g_config.shader_cache_enabled && cache_dir) {
        if (!prismgl_shader_cache_init(cache_dir)) {
            LOGW("Shader cache initialization failed, continuing without cache");
            g_config.shader_cache_enabled = false;
        }
    }

    /* Initialize shader translator */
    if (!shader_translator_init()) {
        LOGE("Shader translator initialization failed");
        return false;
    }

    g_initialized = true;
    LOGI("PrismGL initialized successfully");
    return true;
}

void prismgl_shutdown(void) {
    if (!g_initialized) return;

    LOGI("PrismGL shutting down...");

    if (g_config.shader_cache_enabled) {
        prismgl_shader_cache_shutdown();
    }

    shader_translator_shutdown();

    g_initialized = false;
    LOGI("PrismGL shutdown complete");
}

void prismgl_set_config(const PrismGLConfig* config) {
    if (config) {
        memcpy(&g_config, config, sizeof(PrismGLConfig));
    }
}

PrismGLConfig* prismgl_get_config(void) {
    return &g_config;
}

int prismgl_detect_gpu(void) {
    return g_gpu_info.vendor;
}

const char* prismgl_get_gpu_name(void) {
    return g_gpu_info.renderer_string;
}

void prismgl_apply_gpu_tweaks(int gpu_vendor) {
    g_gpu_info.vendor = (GPUVendor)gpu_vendor;
    gpu_apply_optimizations(&g_gpu_info);
}
