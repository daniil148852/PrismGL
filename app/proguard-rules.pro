# PrismGL ProGuard Rules

# Keep JNI methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep PrismGL classes
-keep class com.prismgl.renderer.** { *; }

# Keep content provider
-keep class com.prismgl.renderer.RendererProvider { *; }
