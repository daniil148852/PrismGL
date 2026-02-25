package com.prismgl.renderer;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import com.prismgl.renderer.service.RendererService;
import com.prismgl.renderer.util.ConfigManager;
import java.io.File;

public class MainActivity extends Activity {
    private TextView statusText;
    private TextView deviceInfoText;
    private Button installButton;
    private Button settingsButton;
    
    private ConfigManager configManager;
    private RendererService rendererService;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        configManager = new ConfigManager(this);
        rendererService = new RendererService(this);
        
        initViews();
        loadDeviceInfo();
        checkInstallation();
    }
    
    private void initViews() {
        statusText = findViewById(R.id.statusText);
        deviceInfoText = findViewById(R.id.deviceInfoText);
        installButton = findViewById(R.id.installButton);
        settingsButton = findViewById(R.id.settingsButton);
        
        installButton.setOnClickListener(v -> installRenderer());
        settingsButton.setOnClickListener(v -> openSettings());
    }
    
    private void loadDeviceInfo() {
        StringBuilder info = new StringBuilder();
        info.append("Device: ").append(android.os.Build.DEVICE).append("\n");
        info.append("Model: ").append(android.os.Build.MODEL).append("\n");
        info.append("GPU: ").append(rendererService.getGPUInfo()).append("\n");
        info.append("API: ").append(android.os.Build.VERSION.SDK_INT).append("\n");
        deviceInfoText.setText(info.toString());
    }
    
    private void checkInstallation() {
        File rendererDir = new File("/sdcard/PrismGL");
        if (rendererDir.exists()) {
            statusText.setText("Status: Installed and Ready");
            installButton.setText("Reinstall");
        } else {
            statusText.setText("Status: Not Installed");
            installButton.setText("Install Renderer");
        }
    }
    
    private void installRenderer() {
        boolean success = rendererService.installRenderer();
        if (success) {
            Toast.makeText(this, "Renderer installed successfully!", Toast.LENGTH_SHORT).show();
            statusText.setText("Status: Installed and Ready");
        } else {
            Toast.makeText(this, "Failed to install renderer", Toast.LENGTH_SHORT).show();
        }
    }
    
    private void openSettings() {
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivity(intent);
    }
}
