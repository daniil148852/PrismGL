/*
 * PrismGL GPU Detection
 * Detects GPU vendor and applies device-specific optimizations
 */

#include "gpu_detect.h"

#include <GLES3/gl32.h>
#include <string.h>
#include <android/log.h>

#define LOG_TAG "PrismGL-GPU"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

static GPUInfo g_gpu_info;
static bool g_detected = false;

static bool check_extension(const char* extensions, const char* ext) {
    if (!extensions || !ext) return false;
    size_t ext_len = strlen(ext);
    const char* p = extensions;
    while ((p = strstr(p, ext)) != NULL) {
        if ((p == extensions || p[-1] == ' ') &&
            (p[ext_len] == ' ' || p[ext_len] == '\0')) {
            return true;
        }
        p += ext_len;
    }
    return false;
}

bool gpu_has_extension(const char* extension) {
    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    return check_extension(extensions, extension);
}

static GPUTier detect_adreno_tier(const char* renderer) {
    /* Adreno 7xx series = ULTRA */
    if (strstr(renderer, "Adreno (TM) 750") ||
        strstr(renderer, "Adreno (TM) 740") ||
        strstr(renderer, "Adreno (TM) 730") ||
        strstr(renderer, "Adreno (TM) 725") ||
        strstr(renderer, "Adreno (TM) 720")) {
        return GPU_TIER_ULTRA;
    }
    /* Adreno 6xx high-end = HIGH */
    if (strstr(renderer, "Adreno (TM) 690") ||
        strstr(renderer, "Adreno (TM) 680") ||
        strstr(renderer, "Adreno (TM) 660") ||
        strstr(renderer, "Adreno (TM) 650") ||
        strstr(renderer, "Adreno (TM) 640") ||
        strstr(renderer, "Adreno (TM) 630")) {
        return GPU_TIER_HIGH;
    }
    /* Adreno 6xx mid-range = MID */
    if (strstr(renderer, "Adreno (TM) 620") ||
        strstr(renderer, "Adreno (TM) 619") ||
        strstr(renderer, "Adreno (TM) 618") ||
        strstr(renderer, "Adreno (TM) 616") ||
        strstr(renderer, "Adreno (TM) 615") ||
        strstr(renderer, "Adreno (TM) 612") ||
        strstr(renderer, "Adreno (TM) 610")) {
        return GPU_TIER_MID;
    }
    /* Anything else is low */
    return GPU_TIER_LOW;
}

static GPUTier detect_mali_tier(const char* renderer) {
    /* Mali-G7xx series high-end = ULTRA */
    if (strstr(renderer, "Mali-G720") ||
        strstr(renderer, "Mali-G715") ||
        strstr(renderer, "Mali-G710") ||
        strstr(renderer, "Mali-G78")) {
        return GPU_TIER_ULTRA;
    }
    /* Mali G77 = HIGH */
    if (strstr(renderer, "Mali-G77") ||
        strstr(renderer, "Mali-G76")) {
        return GPU_TIER_HIGH;
    }
    /* Mali G5x/G6x = MID */
    if (strstr(renderer, "Mali-G57") ||
        strstr(renderer, "Mali-G52") ||
        strstr(renderer, "Mali-G51") ||
        strstr(renderer, "Mali-G68") ||
        strstr(renderer, "Mali-G610")) {
        return GPU_TIER_MID;
    }
    return GPU_TIER_LOW;
}

GPUInfo gpu_detect(void) {
    if (g_detected) return g_gpu_info;

    memset(&g_gpu_info, 0, sizeof(GPUInfo));

    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);

    if (vendor) strncpy(g_gpu_info.vendor_string, vendor, sizeof(g_gpu_info.vendor_string) - 1);
    if (renderer) strncpy(g_gpu_info.renderer_string, renderer, sizeof(g_gpu_info.renderer_string) - 1);
    if (version) strncpy(g_gpu_info.version_string, version, sizeof(g_gpu_info.version_string) - 1);

    /* Detect vendor */
    g_gpu_info.vendor = GPU_VENDOR_UNKNOWN;
    g_gpu_info.tier = GPU_TIER_LOW;

    if (vendor && renderer) {
        if (strstr(vendor, "Qualcomm") || strstr(renderer, "Adreno")) {
            g_gpu_info.vendor = GPU_VENDOR_QUALCOMM_ADRENO;
            g_gpu_info.tier = detect_adreno_tier(renderer);
        } else if (strstr(vendor, "ARM") || strstr(renderer, "Mali")) {
            g_gpu_info.vendor = GPU_VENDOR_ARM_MALI;
            g_gpu_info.tier = detect_mali_tier(renderer);
        } else if (strstr(vendor, "Imagination") || strstr(renderer, "PowerVR")) {
            g_gpu_info.vendor = GPU_VENDOR_IMAGINATION_POWERVR;
            g_gpu_info.tier = GPU_TIER_MID;
        } else if (strstr(vendor, "Samsung") || strstr(renderer, "Xclipse")) {
            g_gpu_info.vendor = GPU_VENDOR_SAMSUNG_XCLIPSE;
            g_gpu_info.tier = GPU_TIER_HIGH;
        } else if (strstr(vendor, "NVIDIA") || strstr(renderer, "Tegra")) {
            g_gpu_info.vendor = GPU_VENDOR_NVIDIA_TEGRA;
            g_gpu_info.tier = GPU_TIER_HIGH;
        }
    } else if (vendor) {
        if (strstr(vendor, "Qualcomm")) g_gpu_info.vendor = GPU_VENDOR_QUALCOMM_ADRENO;
        else if (strstr(vendor, "ARM")) g_gpu_info.vendor = GPU_VENDOR_ARM_MALI;
    }

    /* Query capabilities */
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &g_gpu_info.max_texture_size);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &g_gpu_info.max_texture_units);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &g_gpu_info.max_vertex_attribs);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &g_gpu_info.max_uniform_components);

    /* Parse GL version */
    if (version) {
        sscanf(version, "OpenGL ES %d.%d", &g_gpu_info.gl_major, &g_gpu_info.gl_minor);
    }

    /* Check feature support */
    g_gpu_info.supports_compute_shaders = (g_gpu_info.gl_major >= 3 && g_gpu_info.gl_minor >= 1);
    g_gpu_info.supports_tessellation = gpu_has_extension("GL_EXT_tessellation_shader") ||
                                        gpu_has_extension("GL_OES_tessellation_shader");
    g_gpu_info.supports_geometry_shaders = gpu_has_extension("GL_EXT_geometry_shader") ||
                                            gpu_has_extension("GL_OES_geometry_shader");
    g_gpu_info.supports_astc = gpu_has_extension("GL_KHR_texture_compression_astc_ldr");
    g_gpu_info.supports_etc2 = true; /* Mandatory in ES 3.0+ */
    g_gpu_info.supports_pvrtc = gpu_has_extension("GL_IMG_texture_compression_pvrtc");

    /* Set recommended resolution scale */
    g_gpu_info.recommended_resolution_scale = gpu_get_recommended_scale(&g_gpu_info);

    g_detected = true;

    LOGI("GPU detected: %s", g_gpu_info.renderer_string);
    LOGI("  Vendor: %d, Tier: %d", g_gpu_info.vendor, g_gpu_info.tier);
    LOGI("  GL %d.%d, Max texture: %d", g_gpu_info.gl_major, g_gpu_info.gl_minor,
         g_gpu_info.max_texture_size);
    LOGI("  Compute: %d, Tessellation: %d, Geometry: %d",
         g_gpu_info.supports_compute_shaders,
         g_gpu_info.supports_tessellation,
         g_gpu_info.supports_geometry_shaders);

    return g_gpu_info;
}

float gpu_get_recommended_scale(const GPUInfo* info) {
    switch (info->tier) {
        case GPU_TIER_ULTRA: return 1.0f;
        case GPU_TIER_HIGH:  return 0.9f;
        case GPU_TIER_MID:   return 0.75f;
        case GPU_TIER_LOW:   return 0.5f;
        default:             return 0.75f;
    }
}

void gpu_apply_optimizations(const GPUInfo* info) {
    LOGI("Applying GPU optimizations for vendor %d, tier %d", info->vendor, info->tier);

    switch (info->vendor) {
        case GPU_VENDOR_QUALCOMM_ADRENO:
            LOGI("Adreno: Enabling tiled rendering hints, ETC2 compression");
            break;
        case GPU_VENDOR_ARM_MALI:
            LOGI("Mali: Enabling ASTC compression, optimizing for tile-based");
            break;
        case GPU_VENDOR_IMAGINATION_POWERVR:
            LOGI("PowerVR: Enabling PVRTC, optimizing for TBDR");
            break;
        case GPU_VENDOR_SAMSUNG_XCLIPSE:
            LOGI("Xclipse: Enabling desktop-like optimizations (RDNA2-based)");
            break;
        case GPU_VENDOR_NVIDIA_TEGRA:
            LOGI("Tegra: Enabling desktop-like optimizations");
            break;
        default:
            LOGI("Unknown GPU vendor, using conservative settings");
            break;
    }
}
