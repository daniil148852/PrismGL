#include <jni.h>
#include <string>
#include <memory>
#include "renderer/PrismGLRenderer.h"

static std::unique_ptr<PrismGL::Renderer> g_renderer;

extern "C" {

JNIEXPORT void JNICALL
Java_com_prismgl_renderer_service_RendererService_nativeInitialize(
        JNIEnv* env,
        jobject thiz,
        jint targetFPS,
        jboolean shaderCache,
        jboolean asyncLoading,
        jboolean vulkan) {
    
    PrismGL::RenderConfig config;
    config.targetFPS = targetFPS;
    config.shaderCacheEnabled = shaderCache;
    config.asyncTextureLoading = asyncLoading;
    config.preferredBackend = vulkan ? PrismGL::RenderBackend::VULKAN_ANGLE : PrismGL::RenderBackend::AUTO;
    config.debugMode = false;
    
    g_renderer = std::make_unique<PrismGL::Renderer>();
    g_renderer->initialize(config);
}

JNIEXPORT void JNICALL
Java_com_prismgl_renderer_service_RendererService_nativeShutdown(
        JNIEnv* env,
        jobject thiz) {
    if (g_renderer) {
        g_renderer->shutdown();
        g_renderer.reset();
    }
}

JNIEXPORT void JNICALL
Java_com_prismgl_renderer_service_RendererService_nativeSetResolutionScale(
        JNIEnv* env,
        jobject thiz,
        jfloat scale) {
    if (g_renderer) {
        g_renderer->setResolutionScale(scale);
    }
}

JNIEXPORT void JNICALL
Java_com_prismgl_renderer_service_RendererService_nativeSetTargetFPS(
        JNIEnv* env,
        jobject thiz,
        jint fps) {
    if (g_renderer) {
        g_renderer->setTargetFPS(fps);
    }
}

JNIEXPORT jstring JNICALL
Java_com_prismgl_renderer_service_RendererService_nativeGetDeviceProfile(
        JNIEnv* env,
        jobject thiz) {
    if (g_renderer) {
        std::string profile = g_renderer->getDeviceProfile();
        return env->NewStringUTF(profile.c_str());
    }
    return env->NewStringUTF("unknown");
}

JNIEXPORT jfloat JNICALL
Java_com_prismgl_renderer_service_RendererService_nativeGetFrameTime(
        JNIEnv* env,
        jobject thiz) {
    if (g_renderer) {
        return g_renderer->getMetrics().frameTime;
    }
    return 0.0f;
}

}
