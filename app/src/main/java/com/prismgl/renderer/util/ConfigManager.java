package com.prismgl.renderer.util;

import android.content.Context;
import android.content.SharedPreferences;
import com.google.gson.Gson;

public class ConfigManager {
    private static final String PREFS_NAME = "prismgl_config";
    private static final String KEY_TARGET_FPS = "target_fps";
    private static final String KEY_RESOLUTION_SCALE = "resolution_scale";
    private static final String KEY_SHADER_CACHE = "shader_cache";
    private static final String KEY_ASYNC_LOADING = "async_loading";
    private static final String KEY_VULKAN = "vulkan";
    private static final String KEY_DEBUG_MODE = "debug_mode";
    
    private SharedPreferences prefs;
    private Gson gson;
    
    public ConfigManager(Context context) {
        prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        gson = new Gson();
    }
    
    public int getTargetFPS() {
        return prefs.getInt(KEY_TARGET_FPS, 60);
    }
    
    public void setTargetFPS(int fps) {
        prefs.edit().putInt(KEY_TARGET_FPS, fps).apply();
    }
    
    public float getResolutionScale() {
        return prefs.getFloat(KEY_RESOLUTION_SCALE, 1.5f);
    }
    
    public void setResolutionScale(float scale) {
        prefs.edit().putFloat(KEY_RESOLUTION_SCALE, scale).apply();
    }
    
    public boolean isShaderCacheEnabled() {
        return prefs.getBoolean(KEY_SHADER_CACHE, true);
    }
    
    public void setShaderCacheEnabled(boolean enabled) {
        prefs.edit().putBoolean(KEY_SHADER_CACHE, enabled).apply();
    }
    
    public boolean isAsyncLoadingEnabled() {
        return prefs.getBoolean(KEY_ASYNC_LOADING, true);
    }
    
    public void setAsyncLoadingEnabled(boolean enabled) {
        prefs.edit().putBoolean(KEY_ASYNC_LOADING, enabled).apply();
    }
    
    public boolean isVulkanEnabled() {
        return prefs.getBoolean(KEY_VULKAN, true);
    }
    
    public void setVulkanEnabled(boolean enabled) {
        prefs.edit().putBoolean(KEY_VULKAN, enabled).apply();
    }
    
    public boolean isDebugModeEnabled() {
        return prefs.getBoolean(KEY_DEBUG_MODE, false);
    }
    
    public void setDebugModeEnabled(boolean enabled) {
        prefs.edit().putBoolean(KEY_DEBUG_MODE, enabled).apply();
    }
    
    public void reset() {
        prefs.edit().clear().apply();
    }
}
