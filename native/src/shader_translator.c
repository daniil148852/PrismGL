/*
 * PrismGL Shader Translator
 * Translates desktop GLSL 1.50-4.60 to GLSL ES 3.20
 */

#include "shader_translator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <android/log.h>

#define LOG_TAG "PrismGL-Shader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define MAX_SHADER_SIZE (256 * 1024)

static bool g_translator_initialized = false;

bool shader_translator_init(void) {
    if (g_translator_initialized) return true;
    g_translator_initialized = true;
    LOGI("Shader translator initialized");
    return true;
}

void shader_translator_shutdown(void) {
    g_translator_initialized = false;
}

int shader_detect_version(const char* source) {
    if (!source) return 0;
    const char* version_str = strstr(source, "#version");
    if (!version_str) return 110;
    int version = 0;
    sscanf(version_str, "#version %d", &version);
    return version;
}

/* Simple string replacement utility */
static char* str_replace_all(const char* source, const char* find, const char* replace) {
    if (!source || !find || !replace) return NULL;
    size_t find_len = strlen(find);
    size_t replace_len = strlen(replace);
    if (find_len == 0) return strdup(source);

    int count = 0;
    const char* p = source;
    while ((p = strstr(p, find)) != NULL) {
        count++;
        p += find_len;
    }
    if (count == 0) return strdup(source);

    size_t new_len = strlen(source) + (size_t)count * (replace_len - find_len + 1);
    char* result = (char*)malloc(new_len + 1);
    if (!result) return NULL;

    char* dst = result;
    p = source;
    while (*p) {
        if (strncmp(p, find, find_len) == 0) {
            memcpy(dst, replace, replace_len);
            dst += replace_len;
            p += find_len;
        } else {
            *dst++ = *p++;
        }
    }
    *dst = '\0';
    return result;
}

char* shader_patch_extensions(const char* source) {
    char* result = strdup(source);
    if (!result) return NULL;

    /* Table of extension replacements */
    static const char* ext_replacements[][2] = {
        { "#extension GL_ARB_explicit_attrib_location : enable",
          "/* ARB_explicit_attrib_location: native in ES 3.x */" },
        { "#extension GL_ARB_explicit_attrib_location : require",
          "/* ARB_explicit_attrib_location: native in ES 3.x */" },
        { "#extension GL_ARB_explicit_uniform_location : enable",
          "/* ARB_explicit_uniform_location: emulated */" },
        { "#extension GL_ARB_explicit_uniform_location : require",
          "/* ARB_explicit_uniform_location: emulated */" },
        { "#extension GL_ARB_shader_texture_lod : enable",
          "/* ARB_shader_texture_lod: use textureLod in ES */" },
        { "#extension GL_ARB_conservative_depth : enable",
          "/* ARB_conservative_depth: not available in ES */" },
        { "#extension GL_ARB_texture_gather : enable",
          "#extension GL_EXT_texture_gather : enable" },
        { "#extension GL_ARB_gpu_shader5 : enable",
          "/* GL_ARB_gpu_shader5: partially emulated */" },
        { "#extension GL_ARB_gpu_shader5 : require",
          "/* GL_ARB_gpu_shader5: partially emulated */" },
        { "#extension GL_ARB_uniform_buffer_object : enable",
          "/* ARB_uniform_buffer_object: native in ES 3.x */" },
        { "#extension GL_ARB_separate_shader_objects : enable",
          "/* ARB_separate_shader_objects: native in ES 3.1+ */" },
        { "#extension GL_ARB_shading_language_420pack : enable",
          "/* ARB_shading_language_420pack: native in ES 3.x */" },
        { "#extension GL_ARB_shading_language_420pack : require",
          "/* ARB_shading_language_420pack: native in ES 3.x */" },
        { "#extension GL_ARB_enhanced_layouts : enable",
          "/* ARB_enhanced_layouts: partially emulated */" },
        { "#extension GL_ARB_shader_image_load_store : enable",
          "/* ARB_shader_image_load_store: native in ES 3.1+ */" },
        { "#extension GL_ARB_shader_storage_buffer_object : enable",
          "/* ARB_shader_storage_buffer_object: native in ES 3.1+ */" },
        { "#extension GL_ARB_compute_shader : enable",
          "/* ARB_compute_shader: native in ES 3.1+ */" },
        { "#extension GL_ARB_tessellation_shader : enable",
          "#extension GL_EXT_tessellation_shader : enable" },
        { "#extension GL_ARB_geometry_shader4 : enable",
          "#extension GL_EXT_geometry_shader : enable" },
        { "#extension GL_ARB_draw_instanced : enable",
          "/* ARB_draw_instanced: native in ES 3.0+ */" },
        { "#extension GL_ARB_depth_clamp : enable",
          "/* ARB_depth_clamp: emulated */" },
        { "#extension GL_ARB_clip_control : enable",
          "/* ARB_clip_control: emulated */" },
        { "#extension GL_ARB_seamless_cube_map : enable",
          "/* ARB_seamless_cube_map: always on in ES */" },
        { NULL, NULL }
    };

    for (int i = 0; ext_replacements[i][0] != NULL; i++) {
        char* tmp = str_replace_all(result, ext_replacements[i][0], ext_replacements[i][1]);
        if (tmp) {
            free(result);
            result = tmp;
        }
    }

    return result;
}

char* shader_patch_precision(const char* source, GLenum shader_type) {
    const char* precision_header;
    if (shader_type == GL_FRAGMENT_SHADER) {
        precision_header =
            "precision highp float;\n"
            "precision highp int;\n"
            "precision highp sampler2D;\n"
            "precision highp sampler3D;\n"
            "precision highp samplerCube;\n"
            "precision highp sampler2DArray;\n"
            "precision highp sampler2DShadow;\n"
            "precision highp samplerCubeShadow;\n"
            "precision highp sampler2DArrayShadow;\n"
            "precision highp isampler2D;\n"
            "precision highp isampler3D;\n"
            "precision highp isamplerCube;\n"
            "precision highp usampler2D;\n"
            "precision highp usampler3D;\n"
            "precision highp usamplerCube;\n"
            "precision highp image2D;\n"
            "precision highp iimage2D;\n"
            "precision highp uimage2D;\n";
    } else {
        precision_header =
            "precision highp float;\n"
            "precision highp int;\n";
    }

    size_t header_len = strlen(precision_header);
    size_t source_len = strlen(source);
    char* result = (char*)malloc(header_len + source_len + 2);
    if (!result) return NULL;

    /* Find end of #version line to insert precision after it */
    const char* version_end = strstr(source, "\n");
    if (version_end && strstr(source, "#version") == source) {
        size_t prefix_len = (size_t)(version_end - source) + 1;
        memcpy(result, source, prefix_len);
        memcpy(result + prefix_len, precision_header, header_len);
        memcpy(result + prefix_len + header_len, source + prefix_len, source_len - prefix_len);
        result[header_len + source_len] = '\0';
    } else {
        memcpy(result, precision_header, header_len);
        memcpy(result + header_len, source, source_len);
        result[header_len + source_len] = '\0';
    }

    return result;
}

char* shader_patch_samplers(const char* source) {
    char* result = strdup(source);
    if (!result) return NULL;

    /* sampler1D -> sampler2D (1D textures emulated as 2D) */
    char* tmp = str_replace_all(result, "sampler1D", "sampler2D");
    if (tmp) { free(result); result = tmp; }

    /* isampler1D -> isampler2D */
    tmp = str_replace_all(result, "isampler1D", "isampler2D");
    if (tmp) { free(result); result = tmp; }

    /* usampler1D -> usampler2D */
    tmp = str_replace_all(result, "usampler1D", "usampler2D");
    if (tmp) { free(result); result = tmp; }

    return result;
}

char* shader_patch_builtins(const char* source) {
    char* result = strdup(source);
    if (!result) return NULL;

    /* Replace old-style texture functions with modern versions */
    static const char* builtin_replacements[][2] = {
        { "texture2D(",      "texture(" },
        { "texture3D(",      "texture(" },
        { "textureCube(",    "texture(" },
        { "texture2DProj(",  "textureProj(" },
        { "texture2DLod(",   "textureLod(" },
        { "texture3DLod(",   "textureLod(" },
        { "textureCubeLod(", "textureLod(" },
        { "shadow2D(",       "texture(" },
        { "shadow2DProj(",   "textureProj(" },
        { "texture2DGrad(",  "textureGrad(" },
        { NULL, NULL }
    };

    for (int i = 0; builtin_replacements[i][0] != NULL; i++) {
        char* tmp = str_replace_all(result, builtin_replacements[i][0], builtin_replacements[i][1]);
        if (tmp) { free(result); result = tmp; }
    }

    /* Handle noperspective qualifier (not in ES) */
    char* tmp2 = str_replace_all(result, "noperspective ", "/* noperspective */ ");
    if (tmp2) { free(result); result = tmp2; }

    tmp2 = str_replace_all(result, "noperspective\n", "/* noperspective */\n");
    if (tmp2) { free(result); result = tmp2; }

    return result;
}

ShaderTranslation shader_translate(const char* source, GLenum shader_type) {
    ShaderTranslation result;
    memset(&result, 0, sizeof(result));
    result.success = false;
    result.target_version = 320;

    if (!source || strlen(source) == 0) {
        snprintf(result.error_msg, sizeof(result.error_msg), "Empty shader source");
        return result;
    }

    if (strlen(source) > MAX_SHADER_SIZE) {
        snprintf(result.error_msg, sizeof(result.error_msg), "Shader too large");
        return result;
    }

    result.original_version = shader_detect_version(source);

    /* If already ES, skip translation */
    if (strstr(source, "#version 320 es") || strstr(source, "#version 310 es") ||
        strstr(source, "#version 300 es")) {
        result.translated_source = strdup(source);
        result.success = true;
        return result;
    }

    LOGI("Translating shader from GLSL %d to GLSL ES 320", result.original_version);

    /* Step 1: Replace version directive */
    char* working = NULL;
    char version_old[64];
    snprintf(version_old, sizeof(version_old), "#version %d", result.original_version);

    char version_old_core[64];
    snprintf(version_old_core, sizeof(version_old_core), "#version %d core", result.original_version);

    char version_old_compat[64];
    snprintf(version_old_compat, sizeof(version_old_compat), "#version %d compatibility", result.original_version);

    if (strstr(source, version_old_core)) {
        working = str_replace_all(source, version_old_core, "#version 320 es");
    } else if (strstr(source, version_old_compat)) {
        working = str_replace_all(source, version_old_compat, "#version 320 es");
    } else if (strstr(source, version_old)) {
        working = str_replace_all(source, version_old, "#version 320 es");
    } else {
        size_t len = strlen(source) + 32;
        working = (char*)malloc(len);
        if (working) {
            snprintf(working, len, "#version 320 es\n%s", source);
        }
    }

    if (!working) {
        snprintf(result.error_msg, sizeof(result.error_msg), "Memory allocation failed");
        return result;
    }

    /* Step 2: Patch extensions */
    char* tmp = shader_patch_extensions(working);
    if (tmp) { free(working); working = tmp; }

    /* Step 3: Add precision qualifiers */
    tmp = shader_patch_precision(working, shader_type);
    if (tmp) { free(working); working = tmp; }

    /* Step 4: Patch sampler types */
    tmp = shader_patch_samplers(working);
    if (tmp) { free(working); working = tmp; }

    /* Step 5: Patch builtin functions */
    tmp = shader_patch_builtins(working);
    if (tmp) { free(working); working = tmp; }

    /* Step 6: Replace unsupported double-precision types */
    static const char* type_replacements[][2] = {
        { "dvec2", "vec2" },
        { "dvec3", "vec3" },
        { "dvec4", "vec4" },
        { "dmat2", "mat2" },
        { "dmat3", "mat3" },
        { "dmat4", "mat4" },
        { "dmat2x2", "mat2" },
        { "dmat3x3", "mat3" },
        { "dmat4x4", "mat4" },
        { "dmat2x3", "mat2x3" },
        { "dmat2x4", "mat2x4" },
        { "dmat3x2", "mat3x2" },
        { "dmat3x4", "mat3x4" },
        { "dmat4x2", "mat4x2" },
        { "dmat4x3", "mat4x3" },
        { NULL, NULL }
    };

    for (int i = 0; type_replacements[i][0] != NULL; i++) {
        tmp = str_replace_all(working, type_replacements[i][0], type_replacements[i][1]);
        if (tmp) { free(working); working = tmp; }
    }

    /* Step 7: Handle gl_FragColor/gl_FragData for very old shaders */
    if (result.original_version <= 120 && shader_type == GL_FRAGMENT_SHADER) {
        /* Old GLSL used gl_FragColor, modern uses out variable */
        if (strstr(working, "gl_FragColor") && !strstr(working, "out vec4")) {
            /* Add output variable declaration */
            tmp = str_replace_all(working, "gl_FragColor", "prismgl_FragColor");
            if (tmp) { free(working); working = tmp; }

            /* Insert output declaration after precision */
            const char* out_decl = "out vec4 prismgl_FragColor;\n";
            size_t decl_len = strlen(out_decl);
            size_t work_len = strlen(working);
            char* new_src = (char*)malloc(work_len + decl_len + 1);
            if (new_src) {
                /* Find insertion point after precision block */
                const char* insert_after = strstr(working, "precision highp");
                if (insert_after) {
                    /* Find end of last precision line */
                    const char* p = insert_after;
                    while (*p) {
                        if (*p == '\n') {
                            const char* next = p + 1;
                            if (strncmp(next, "precision", 9) != 0) {
                                break;
                            }
                        }
                        p++;
                    }
                    if (*p == '\n') p++;
                    size_t prefix = (size_t)(p - working);
                    memcpy(new_src, working, prefix);
                    memcpy(new_src + prefix, out_decl, decl_len);
                    memcpy(new_src + prefix + decl_len, p, work_len - prefix);
                    new_src[prefix + decl_len + work_len - prefix] = '\0';
                    free(working);
                    working = new_src;
                } else {
                    free(new_src);
                }
            }
        }

        /* varying -> in for fragment shaders */
        tmp = str_replace_all(working, "\nvarying ", "\nin ");
        if (tmp) { free(working); working = tmp; }
    }

    if (result.original_version <= 120 && shader_type == GL_VERTEX_SHADER) {
        /* attribute -> in, varying -> out for vertex shaders */
        tmp = str_replace_all(working, "\nattribute ", "\nin ");
        if (tmp) { free(working); working = tmp; }
        tmp = str_replace_all(working, "\nvarying ", "\nout ");
        if (tmp) { free(working); working = tmp; }
    }

    result.translated_source = working;
    result.success = true;

    LOGI("Shader translation successful (%zu bytes)", strlen(working));
    return result;
}

void shader_translation_free(ShaderTranslation* result) {
    if (result && result->translated_source) {
        free(result->translated_source);
        result->translated_source = NULL;
    }
}
