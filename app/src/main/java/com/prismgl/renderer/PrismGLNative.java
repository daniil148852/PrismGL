package com.prismgl.renderer;

/**
 * JNI bridge to the native PrismGL renderer library.
 * Provides Java-accessible methods for all native functionality.
 */
public class PrismGLNative {

    static {
        System.loadLibrary("PrismGL");
    }

    /**
     * Initialize the PrismGL renderer with the given cache directory.
     * @param cacheDir Path to store shader cache and config files
     * @return true if initialization was successful
     */
    public static native boolean nativeInit(String cacheDir);

    /**
     * Shutdown the PrismGL renderer and free all resources.
     */
    public static native void nativeShutdown();

    /**
     * Get the detected GPU name string.
     * @return GPU renderer string (e.g., "Adreno (TM) 660")
     */
    public static native String nativeGetGPUName();

    /**
     * Detect GPU vendor.
     * @return GPU vendor ID (0=unknown, 1=Adreno, 2=Mali, 3=PowerVR, 4=Xclipse, 5=Tegra)
     */
    public static native int nativeDetectGPU();

    /**
     * Set the rendering resolution scale factor.
     * @param scale Value between 0.25 and 1.0
     */
    public static native void nativeSetResolutionScale(float scale);

    /**
     * Get the current resolution scale factor.
     * @return Current resolution scale (0.25 - 1.0)
     */
    public static native float nativeGetResolutionScale();

    /**
     * Configure the renderer settings.
     */
    public static native void nativeSetConfig(
            boolean shaderCache,
            boolean drawCallBatching,
            boolean adaptiveRes,
            boolean asyncTexture,
            boolean vulkanBackend,
            float resScale
    );

    /**
     * Get the address of an OpenGL function by name.
     * Used by the launcher to resolve GL function pointers.
     * @param name The OpenGL function name (e.g., "glDrawArrays")
     * @return Function pointer address, or 0 if not found
     */
    public static native long nativeGetProcAddress(String name);
}
