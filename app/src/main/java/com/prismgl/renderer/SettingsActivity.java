package com.prismgl.renderer;

import android.app.Activity;
import android.os.Bundle;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Switch;
import com.prismgl.renderer.util.ConfigManager;

public class SettingsActivity extends Activity {
    private SeekBar fpsSeekBar;
    private SeekBar resolutionSeekBar;
    private Switch shaderCacheSwitch;
    private Switch asyncLoadingSwitch;
    private Switch vulkanSwitch;
    private TextView fpsValueText;
    private TextView resolutionValueText;
    
    private ConfigManager configManager;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);
        
        configManager = new ConfigManager(this);
        
        initViews();
        loadSettings();
    }
    
    private void initViews() {
        fpsSeekBar = findViewById(R.id.fpsSeekBar);
        resolutionSeekBar = findViewById(R.id.resolutionSeekBar);
        shaderCacheSwitch = findViewById(R.id.shaderCacheSwitch);
        asyncLoadingSwitch = findViewById(R.id.asyncLoadingSwitch);
        vulkanSwitch = findViewById(R.id.vulkanSwitch);
        fpsValueText = findViewById(R.id.fpsValueText);
        resolutionValueText = findViewById(R.id.resolutionValueText);
        
        fpsSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                fpsValueText.setText(progress + " FPS");
                configManager.setTargetFPS(progress);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        
        resolutionSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                float scale = 0.5f + (progress / 100.0f) * 1.5f;
                resolutionValueText.setText(String.format("%.1fx", scale));
                configManager.setResolutionScale(scale);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        
        shaderCacheSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            configManager.setShaderCacheEnabled(isChecked);
        });
        
        asyncLoadingSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            configManager.setAsyncLoadingEnabled(isChecked);
        });
        
        vulkanSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            configManager.setVulkanEnabled(isChecked);
        });
    }
    
    private void loadSettings() {
        fpsSeekBar.setProgress(configManager.getTargetFPS());
        fpsValueText.setText(configManager.getTargetFPS() + " FPS");
        
        int resProgress = (int)((configManager.getResolutionScale() - 0.5f) / 1.5f * 100);
        resolutionSeekBar.setProgress(resProgress);
        resolutionValueText.setText(String.format("%.1fx", configManager.getResolutionScale()));
        
        shaderCacheSwitch.setChecked(configManager.isShaderCacheEnabled());
        asyncLoadingSwitch.setChecked(configManager.isAsyncLoadingEnabled());
        vulkanSwitch.setChecked(configManager.isVulkanEnabled());
    }
}
