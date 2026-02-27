package com.prismgl.renderer;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;

/**
 * Content provider for launcher integration.
 * Minecraft launchers (PojavLauncher, Zalith, etc.) query this provider
 * to discover the renderer and access its native library and configuration.
 */
public class RendererProvider extends ContentProvider {

    private static final String TAG = "PrismGL-Provider";
    private static final String AUTHORITY = "com.prismgl.renderer.provider";

    @Override
    public boolean onCreate() {
        Log.i(TAG, "PrismGL renderer provider created");
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {
        String path = uri.getPath();

        if ("/renderer_info".equals(path)) {
            MatrixCursor cursor = new MatrixCursor(new String[]{
                    "name", "version", "library", "gl_version", "description"
            });
            cursor.addRow(new Object[]{
                    "PrismGL",
                    "1.0.0",
                    "libPrismGL.so",
                    "4.6",
                    "High-performance OpenGL 4.x to GLES 3.x renderer"
            });
            return cursor;
        }

        return null;
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        String path = uri.getPath();

        if ("/library".equals(path)) {
            String libDir = getContext().getApplicationInfo().nativeLibraryDir;
            File libFile = new File(libDir, "libPrismGL.so");
            if (libFile.exists()) {
                return ParcelFileDescriptor.open(libFile, ParcelFileDescriptor.MODE_READ_ONLY);
            }
            throw new FileNotFoundException("libPrismGL.so not found");
        }

        if ("/config".equals(path)) {
            File configFile = new File(getContext().getFilesDir(), "config.json");
            if (configFile.exists()) {
                return ParcelFileDescriptor.open(configFile, ParcelFileDescriptor.MODE_READ_ONLY);
            }
            throw new FileNotFoundException("config.json not found");
        }

        throw new FileNotFoundException("Unknown URI: " + uri);
    }

    @Override
    public String getType(Uri uri) {
        return "application/octet-stream";
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) { return null; }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) { return 0; }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) { return 0; }
}
