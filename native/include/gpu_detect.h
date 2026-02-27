/*
 * PrismGL GPU Detection and Optimization
 * Device-specific tweaks for Adreno, Mali, PowerVR, etc.
 */

#ifndef GPU_DETECT_H
#define GPU_DETECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GPU_VENDOR_UNKNOWN = 0,
    GPU_VENDOR_QUALCOMM_ADRENO = 1,
    GPU_VENDOR_ARM_MALI = 2,
    GPU_VENDOR_IMAGINATION_POWERVR = 3,
    GPU_VENDOR_SAMSUNG_XCLIPSE = 4,
    GPU_VENDOR_NVIDIA_TEGRA = 5
} GPUVendor;

typedef enum {
    GPU_TIER_LOW = 0,
    GPU_TIER_MID = 1,
    GPU_TIER_HIGH = 2,
    GPU_TIER_ULTRA = 3
} GPUTier;

typedef struct {
    GPUVendor vendor;
    GPUTier tier;
    char vendor_string[256];
    char renderer_string[256];
    char version_string[256];
    int gl_major;
    int gl_minor;
    int max_texture_size;
    int max_texture_units;
    int max_vertex_attribs;
    int max_uniform_components;
    bool supports_compute_shaders;
    bool supports_tessellation;
    bool supports_geometry_shaders;
    bool supports_astc;
    bool supports_etc2;
    bool supports_pvrtc;
    float recommended_resolution_scale;
} GPUInfo;

/* Detect GPU and populate GPUInfo */
GPUInfo gpu_detect(void);

/* Get optimized settings for detected GPU */
void gpu_apply_optimizations(const GPUInfo* info);

/* Get recommended resolution scale based on GPU tier */
float gpu_get_recommended_scale(const GPUInfo* info);

/* Check extension support */
bool gpu_has_extension(const char* extension);

#ifdef __cplusplus
}
#endif

#endif /* GPU_DETECT_H */
