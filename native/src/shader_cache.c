/*
 * PrismGL Shader Cache
 * Binary shader caching for fast reload
 */

#include "prismgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <android/log.h>

#define LOG_TAG "PrismGL-ShaderCache"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define MAX_CACHE_ENTRIES 2048
#define CACHE_FILE_EXT ".pglbin"

typedef struct {
    uint64_t hash;
    GLuint program;
    char filepath[512];
    bool loaded;
} ShaderCacheEntry;

static ShaderCacheEntry g_cache[MAX_CACHE_ENTRIES];
static int g_cache_count = 0;
static char g_cache_dir[512] = {0};
static bool g_cache_initialized = false;

/* FNV-1a hash for shader source */
uint64_t prismgl_hash_shader_source(const char* vertex_src, const char* fragment_src) {
    uint64_t hash = 14695981039346656037ULL; /* FNV offset basis */
    const uint64_t prime = 1099511628211ULL;

    if (vertex_src) {
        const char* p = vertex_src;
        while (*p) {
            hash ^= (uint64_t)(unsigned char)(*p);
            hash *= prime;
            p++;
        }
    }

    /* Separator */
    hash ^= 0xFF;
    hash *= prime;

    if (fragment_src) {
        const char* p = fragment_src;
        while (*p) {
            hash ^= (uint64_t)(unsigned char)(*p);
            hash *= prime;
            p++;
        }
    }

    return hash;
}

static void cache_build_path(char* path, size_t path_size, uint64_t hash) {
    snprintf(path, path_size, "%s/%016llx%s",
             g_cache_dir, (unsigned long long)hash, CACHE_FILE_EXT);
}

bool prismgl_shader_cache_init(const char* cache_dir) {
    if (g_cache_initialized) return true;

    strncpy(g_cache_dir, cache_dir, sizeof(g_cache_dir) - 1);

    /* Create cache directory if it doesn't exist */
    struct stat st;
    if (stat(g_cache_dir, &st) != 0) {
        if (mkdir(g_cache_dir, 0755) != 0) {
            LOGE("Failed to create shader cache directory: %s", g_cache_dir);
            return false;
        }
    }

    /* Create shader subdirectory */
    char shader_dir[600];
    snprintf(shader_dir, sizeof(shader_dir), "%s/shaders", g_cache_dir);
    if (stat(shader_dir, &st) != 0) {
        mkdir(shader_dir, 0755);
    }
    strncpy(g_cache_dir, shader_dir, sizeof(g_cache_dir) - 1);

    /* Scan existing cache files */
    DIR* dir = opendir(g_cache_dir);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL && g_cache_count < MAX_CACHE_ENTRIES) {
            if (strstr(entry->d_name, CACHE_FILE_EXT)) {
                uint64_t hash = 0;
                if (sscanf(entry->d_name, "%llx", (unsigned long long*)&hash) == 1) {
                    g_cache[g_cache_count].hash = hash;
                    g_cache[g_cache_count].loaded = false;
                    g_cache[g_cache_count].program = 0;
                    cache_build_path(g_cache[g_cache_count].filepath,
                                   sizeof(g_cache[g_cache_count].filepath), hash);
                    g_cache_count++;
                }
            }
        }
        closedir(dir);
    }

    g_cache_initialized = true;
    LOGI("Shader cache initialized with %d entries at %s", g_cache_count, g_cache_dir);
    return true;
}

void prismgl_shader_cache_shutdown(void) {
    if (!g_cache_initialized) return;

    /* Programs are owned by GL context, don't delete them here */
    g_cache_count = 0;
    g_cache_initialized = false;
    LOGI("Shader cache shutdown");
}

GLuint prismgl_shader_cache_get(uint64_t hash) {
    if (!g_cache_initialized) return 0;

    for (int i = 0; i < g_cache_count; i++) {
        if (g_cache[i].hash == hash) {
            if (g_cache[i].loaded) {
                return g_cache[i].program;
            }

            /* Load binary from disk */
            FILE* f = fopen(g_cache[i].filepath, "rb");
            if (!f) {
                LOGW("Cache file missing: %s", g_cache[i].filepath);
                return 0;
            }

            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);

            if (size <= (long)sizeof(GLenum)) {
                fclose(f);
                return 0;
            }

            /* Read format */
            GLenum format;
            if (fread(&format, sizeof(GLenum), 1, f) != 1) {
                fclose(f);
                return 0;
            }

            long binary_size = size - sizeof(GLenum);
            void* binary = malloc(binary_size);
            if (!binary) {
                fclose(f);
                return 0;
            }

            if (fread(binary, 1, binary_size, f) != (size_t)binary_size) {
                free(binary);
                fclose(f);
                return 0;
            }
            fclose(f);

            /* Create program from binary */
            GLuint program = glCreateProgram();
            glProgramBinary(program, format, binary, binary_size);
            free(binary);

            /* Check if program loaded successfully */
            GLint link_status;
            glGetProgramiv(program, GL_LINK_STATUS, &link_status);
            if (link_status != GL_TRUE) {
                LOGW("Cached shader binary invalid (driver update?), removing");
                glDeleteProgram(program);
                remove(g_cache[i].filepath);
                /* Remove from cache array */
                g_cache[i] = g_cache[g_cache_count - 1];
                g_cache_count--;
                return 0;
            }

            g_cache[i].program = program;
            g_cache[i].loaded = true;
            LOGI("Loaded cached shader: %016llx", (unsigned long long)hash);
            return program;
        }
    }

    return 0;
}

void prismgl_shader_cache_put(uint64_t hash, GLuint program) {
    if (!g_cache_initialized) return;
    if (g_cache_count >= MAX_CACHE_ENTRIES) {
        LOGW("Shader cache full (%d entries)", g_cache_count);
        return;
    }

    /* Check if already cached */
    for (int i = 0; i < g_cache_count; i++) {
        if (g_cache[i].hash == hash) return;
    }

    /* Get program binary */
    GLint binary_length = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binary_length);
    if (binary_length <= 0) {
        LOGW("Program has no binary representation");
        return;
    }

    void* binary = malloc(binary_length);
    if (!binary) return;

    GLenum format;
    GLsizei actual_length;
    glGetProgramBinary(program, binary_length, &actual_length, &format, binary);

    /* Write to file */
    char filepath[512];
    cache_build_path(filepath, sizeof(filepath), hash);

    FILE* f = fopen(filepath, "wb");
    if (f) {
        fwrite(&format, sizeof(GLenum), 1, f);
        fwrite(binary, 1, actual_length, f);
        fclose(f);

        /* Add to cache array */
        g_cache[g_cache_count].hash = hash;
        g_cache[g_cache_count].program = program;
        g_cache[g_cache_count].loaded = true;
        strncpy(g_cache[g_cache_count].filepath, filepath,
                sizeof(g_cache[g_cache_count].filepath) - 1);
        g_cache_count++;

        LOGI("Cached shader: %016llx (%d bytes)",
             (unsigned long long)hash, actual_length);
    } else {
        LOGE("Failed to write shader cache file: %s", filepath);
    }

    free(binary);
}
