/*
 * PrismGL JNI Bridge
 * Connects Java/Kotlin Android app to native C library
 */

#include "prismgl.h"

#include <jni.h>
#include <string.h>
#include <android/log.h>

#define LOG_TAG "PrismGL-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

JNIEXPORT jboolean JNICALL
Java_com_prismgl_renderer_PrismGLNative_nativeInit(JNIEnv* env, jclass clazz, jstring cache_dir) {
    const char* dir = (*env)->GetStringUTFChars(env, cache_dir, NULL);
    bool result = prismgl_init(dir);
    (*env)->ReleaseStringUTFChars(env, cache_dir, dir);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_prismgl_renderer_PrismGLNative_nativeShutdown(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    prismgl_shutdown();
}

JNIEXPORT jstring JNICALL
Java_com_prismgl_renderer_PrismGLNative_nativeGetGPUName(JNIEnv* env, jclass clazz) {
    (void)clazz;
    const char* name = prismgl_get_gpu_name();
    return (*env)->NewStringUTF(env, name ? name : "Unknown");
}

JNIEXPORT jint JNICALL
Java_com_prismgl_renderer_PrismGLNative_nativeDetectGPU(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    return prismgl_detect_gpu();
}

JNIEXPORT void JNICALL
Java_com_prismgl_renderer_PrismGLNative_nativeSetResolutionScale(JNIEnv* env, jclass clazz, jfloat scale) {
    (void)env;
    (void)clazz;
    prismgl_set_resolution_scale(scale);
}

JNIEXPORT jfloat JNICALL
Java_com_prismgl_renderer_PrismGLNative_nativeGetResolutionScale(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    return prismgl_get_resolution_scale();
}

JNIEXPORT void JNICALL
Java_com_prismgl_renderer_PrismGLNative_nativeSetConfig(JNIEnv* env, jclass clazz,
    jboolean shaderCache, jboolean drawCallBatching, jboolean adaptiveRes,
    jboolean asyncTexture, jboolean vulkanBackend, jfloat resScale) {
    (void)env;
    (void)clazz;

    PrismGLConfig* config = prismgl_get_config();
    config->shader_cache_enabled = shaderCache;
    config->draw_call_batching = drawCallBatching;
    config->adaptive_resolution = adaptiveRes;
    config->async_texture_loading = asyncTexture;
    config->vulkan_backend = vulkanBackend;
    config->resolution_scale = resScale;
}

JNIEXPORT jlong JNICALL
Java_com_prismgl_renderer_PrismGLNative_nativeGetProcAddress(JNIEnv* env, jclass clazz, jstring name) {
    const char* func_name = (*env)->GetStringUTFChars(env, name, NULL);
    void* addr = prismgl_get_proc_address(func_name);
    (*env)->ReleaseStringUTFChars(env, name, func_name);
    return (jlong)(intptr_t)addr;
}
