package com.prismgl.renderer;

import android.util.Log;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.IOException;

/**
 * Manages the PrismGL renderer configuration file (config.json).
 * This file is read by Minecraft launchers (PojavLauncher, Zalith, Amethyst)
 * to detect and configure the renderer plugin.
 */
public class RendererConfig {

    private static final String TAG = "PrismGL-Config";

    public String name = "PrismGL";
    public String version = "1.0.0";
    public String description = "High-performance OpenGL 4.x to GLES 3.x renderer";
    public String library = "libPrismGL.so";
    public String glVersion = "4.6";
    public String glslVersion = "460";
    public boolean shaderCacheEnabled = true;
    public boolean drawCallBatching = true;
    public boolean adaptiveResolution = true;
    public boolean asyncTextureLoading = true;
    public float resolutionScale = 1.0f;

    /**
     * Write config to JSON file.
     */
    public void writeToFile(File file) {
        try {
            JSONObject json = new JSONObject();
            json.put("name", name);
            json.put("version", version);
            json.put("description", description);
            json.put("library", library);

            JSONObject glConfig = new JSONObject();
            glConfig.put("version", glVersion);
            glConfig.put("glsl_version", glslVersion);
            json.put("opengl", glConfig);

            JSONObject optimizations = new JSONObject();
            optimizations.put("shader_cache", shaderCacheEnabled);
            optimizations.put("draw_call_batching", drawCallBatching);
            optimizations.put("adaptive_resolution", adaptiveResolution);
            optimizations.put("async_texture_loading", asyncTextureLoading);
            optimizations.put("resolution_scale", resolutionScale);
            json.put("optimizations", optimizations);

            JSONObject compatibility = new JSONObject();
            compatibility.put("min_gles_version", "3.2");
            compatibility.put("supported_launchers", new org.json.JSONArray()
                    .put("PojavLauncher")
                    .put("Zalith Launcher")
                    .put("Amethyst Launcher"));
            compatibility.put("supported_mods", new org.json.JSONArray()
                    .put("Sodium")
                    .put("Iris Shaders")
                    .put("Create")
                    .put("JourneyMap")
                    .put("OptiFine"));
            json.put("compatibility", compatibility);

            FileOutputStream fos = new FileOutputStream(file);
            fos.write(json.toString(2).getBytes("UTF-8"));
            fos.close();

            Log.i(TAG, "Config written to " + file.getAbsolutePath());
        } catch (Exception e) {
            Log.e(TAG, "Failed to write config", e);
        }
    }

    /**
     * Read config from JSON file.
     */
    public static RendererConfig readFromFile(File file) {
        RendererConfig config = new RendererConfig();
        try {
            FileInputStream fis = new FileInputStream(file);
            byte[] data = new byte[(int) file.length()];
            fis.read(data);
            fis.close();

            JSONObject json = new JSONObject(new String(data, "UTF-8"));
            config.name = json.optString("name", config.name);
            config.version = json.optString("version", config.version);
            config.description = json.optString("description", config.description);
            config.library = json.optString("library", config.library);

            JSONObject glConfig = json.optJSONObject("opengl");
            if (glConfig != null) {
                config.glVersion = glConfig.optString("version", config.glVersion);
                config.glslVersion = glConfig.optString("glsl_version", config.glslVersion);
            }

            JSONObject opts = json.optJSONObject("optimizations");
            if (opts != null) {
                config.shaderCacheEnabled = opts.optBoolean("shader_cache", true);
                config.drawCallBatching = opts.optBoolean("draw_call_batching", true);
                config.adaptiveResolution = opts.optBoolean("adaptive_resolution", true);
                config.asyncTextureLoading = opts.optBoolean("async_texture_loading", true);
                config.resolutionScale = (float) opts.optDouble("resolution_scale", 1.0);
            }

            Log.i(TAG, "Config loaded from " + file.getAbsolutePath());
        } catch (Exception e) {
            Log.e(TAG, "Failed to read config", e);
        }
        return config;
    }
}
