package com.prismgl.renderer;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Main activity for PrismGL renderer plugin.
 * Allows configuration and installation of the renderer for Minecraft launchers.
 */
public class MainActivity extends Activity {

    private static final String TAG = "PrismGL";
    private static final String PREFS_NAME = "PrismGLPrefs";

    private TextView tvStatus;
    private TextView tvGPU;
    private TextView tvResScale;
    private CheckBox cbShaderCache;
    private CheckBox cbDrawCallBatching;
    private CheckBox cbAdaptiveRes;
    private CheckBox cbAsyncTexture;
    private SeekBar sbResScale;
    private Button btnInstall;
    private Button btnApply;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initViews();
        loadPreferences();
        setupListeners();

        // Install renderer files on first launch
        installRendererFiles();
    }

    private void initViews() {
        tvStatus = findViewById(R.id.tv_status);
        tvGPU = findViewById(R.id.tv_gpu);
        tvResScale = findViewById(R.id.tv_res_scale);
        cbShaderCache = findViewById(R.id.cb_shader_cache);
        cbDrawCallBatching = findViewById(R.id.cb_draw_call_batching);
        cbAdaptiveRes = findViewById(R.id.cb_adaptive_res);
        cbAsyncTexture = findViewById(R.id.cb_async_texture);
        sbResScale = findViewById(R.id.sb_res_scale);
        btnInstall = findViewById(R.id.btn_install);
        btnApply = findViewById(R.id.btn_apply);
    }

    private void loadPreferences() {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        cbShaderCache.setChecked(prefs.getBoolean("shader_cache", true));
        cbDrawCallBatching.setChecked(prefs.getBoolean("draw_call_batching", true));
        cbAdaptiveRes.setChecked(prefs.getBoolean("adaptive_res", true));
        cbAsyncTexture.setChecked(prefs.getBoolean("async_texture", true));
        sbResScale.setProgress(prefs.getInt("res_scale", 100));
        updateResScaleText(prefs.getInt("res_scale", 100));
    }

    private void savePreferences() {
        SharedPreferences.Editor editor = getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit();
        editor.putBoolean("shader_cache", cbShaderCache.isChecked());
        editor.putBoolean("draw_call_batching", cbDrawCallBatching.isChecked());
        editor.putBoolean("adaptive_res", cbAdaptiveRes.isChecked());
        editor.putBoolean("async_texture", cbAsyncTexture.isChecked());
        editor.putInt("res_scale", sbResScale.getProgress());
        editor.apply();
    }

    private void setupListeners() {
        sbResScale.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                // Minimum 25%
                if (progress < 25) {
                    seekBar.setProgress(25);
                    progress = 25;
                }
                updateResScaleText(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        btnInstall.setOnClickListener(v -> {
            installRendererFiles();
            Toast.makeText(this, "Renderer installed! Restart your launcher.", Toast.LENGTH_LONG).show();
        });

        btnApply.setOnClickListener(v -> {
            savePreferences();
            applyConfig();
            Toast.makeText(this, "Settings applied!", Toast.LENGTH_SHORT).show();
        });
    }

    private void updateResScaleText(int progress) {
        tvResScale.setText("Resolution Scale: " + progress + "%");
    }

    private void installRendererFiles() {
        try {
            // Create PrismGL directory in shared storage
            File prismDir = new File(Environment.getExternalStorageDirectory(), "PrismGL");
            if (!prismDir.exists()) {
                prismDir.mkdirs();
            }

            // Write config.json for launcher detection
            File configFile = new File(prismDir, "config.json");
            RendererConfig config = new RendererConfig();
            config.name = "PrismGL";
            config.version = BuildConfig.VERSION_NAME;
            config.description = "High-performance OpenGL 4.x to GLES 3.x renderer";
            config.library = "libPrismGL.so";
            config.shaderCacheEnabled = cbShaderCache.isChecked();
            config.drawCallBatching = cbDrawCallBatching.isChecked();
            config.adaptiveResolution = cbAdaptiveRes.isChecked();
            config.resolutionScale = sbResScale.getProgress() / 100.0f;
            config.writeToFile(configFile);

            // Copy native library to shared location
            copyNativeLib(prismDir);

            tvStatus.setText("Status: Installed");
            tvStatus.setTextColor(0xFF00CC00);

            Log.i(TAG, "Renderer files installed to " + prismDir.getAbsolutePath());
        } catch (Exception e) {
            Log.e(TAG, "Failed to install renderer files", e);
            tvStatus.setText("Status: Installation failed");
            tvStatus.setTextColor(0xFFCC0000);
        }
    }

    private void copyNativeLib(File destDir) {
        String abi = android.os.Build.SUPPORTED_ABIS[0];
        String libName = "libPrismGL.so";

        File destFile = new File(destDir, libName);

        // Get the native lib from the app's own lib directory
        String appLibDir = getApplicationInfo().nativeLibraryDir;
        File srcFile = new File(appLibDir, libName);

        if (srcFile.exists()) {
            try {
                copyFile(srcFile, destFile);
                Log.i(TAG, "Copied " + libName + " for ABI " + abi);
            } catch (IOException e) {
                Log.e(TAG, "Failed to copy native library", e);
            }
        } else {
            Log.w(TAG, "Native library not found at " + srcFile.getAbsolutePath());
        }
    }

    private void copyFile(File src, File dst) throws IOException {
        java.io.FileInputStream in = new java.io.FileInputStream(src);
        FileOutputStream out = new FileOutputStream(dst);
        byte[] buf = new byte[8192];
        int len;
        while ((len = in.read(buf)) > 0) {
            out.write(buf, 0, len);
        }
        in.close();
        out.close();
    }

    private void applyConfig() {
        float scale = sbResScale.getProgress() / 100.0f;
        PrismGLNative.nativeSetConfig(
                cbShaderCache.isChecked(),
                cbDrawCallBatching.isChecked(),
                cbAdaptiveRes.isChecked(),
                cbAsyncTexture.isChecked(),
                false, // vulkan backend
                scale
        );

        // Update config.json too
        try {
            File prismDir = new File(Environment.getExternalStorageDirectory(), "PrismGL");
            File configFile = new File(prismDir, "config.json");
            if (prismDir.exists()) {
                RendererConfig config = new RendererConfig();
                config.name = "PrismGL";
                config.version = BuildConfig.VERSION_NAME;
                config.description = "High-performance OpenGL 4.x to GLES 3.x renderer";
                config.library = "libPrismGL.so";
                config.shaderCacheEnabled = cbShaderCache.isChecked();
                config.drawCallBatching = cbDrawCallBatching.isChecked();
                config.adaptiveResolution = cbAdaptiveRes.isChecked();
                config.resolutionScale = scale;
                config.writeToFile(configFile);
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to update config", e);
        }
    }
}
