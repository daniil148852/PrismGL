package com.prismgl.renderer.service;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.opengl.GLRenderer;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class RendererService {
    private Context context;
    private long nativeRenderer;
    
    static {
        System.loadLibrary("prismgl");
    }
    
    public RendererService(Context context) {
        this.context = context;
    }
    
    public native void nativeInitialize(int targetFPS, boolean shaderCache, boolean asyncLoading, boolean vulkan);
    public native void nativeShutdown();
    public native void nativeSetResolutionScale(float scale);
    public native void nativeSetTargetFPS(int fps);
    public native String nativeGetDeviceProfile();
    public native float nativeGetFrameTime();
    
    public boolean installRenderer() {
        try {
            File rendererDir = new File("/sdcard/PrismGL");
            if (!rendererDir.exists()) {
                rendererDir.mkdirs();
            }
            
            File configFile = new File(rendererDir, "config.json");
            String config = createConfigJson();
            FileOutputStream fos = new FileOutputStream(configFile);
            fos.write(config.getBytes());
            fos.close();
            
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }
    
    private String createConfigJson() {
        return "{\n" +
            "  \"name\": \"PrismGL\",\n" +
            "  \"version\": \"1.0.0\",\n" +
            "  \"description\": \"High-performance OpenGL renderer for Minecraft Java Edition\",\n" +
            "  \"supported_versions\": [\"1.20\", \"1.19\", \"1.18\"],\n" +
            "  \"required_mods\": [\"Sodium\", \"Iris\"],\n" +
            "  \"features\": {\n" +
            "    \"shader_cache\": true,\n" +
            "    \"adaptive_rendering\": true,\n" +
            "    \"vulkan_support\": true,\n" +
            "    \"instancing\": true,\n" +
            "    \"bindless_textures\": true\n" +
            "  }\n" +
            "}";
    }
    
    public String getGPUInfo() {
        String renderer = android.os.Build.HARDWARE;
        return renderer != null ? renderer : "Unknown";
    }
    
    public void initialize(int targetFPS, boolean shaderCache, boolean asyncLoading, boolean vulkan) {
        nativeInitialize(targetFPS, shaderCache, asyncLoading, vulkan);
    }
    
    public void shutdown() {
        nativeShutdown();
    }
    
    public void setResolutionScale(float scale) {
        nativeSetResolutionScale(scale);
    }
    
    public void setTargetFPS(int fps) {
        nativeSetTargetFPS(fps);
    }
    
    public String getDeviceProfile() {
        return nativeGetDeviceProfile();
    }
    
    public float getFrameTime() {
        return nativeGetFrameTime();
    }
}
