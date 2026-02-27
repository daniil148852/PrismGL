/*
 * PrismGL Shader Translator
 * Translates GLSL 1.50-4.60 shaders to GLSL ES 3.20
 */

#ifndef SHADER_TRANSLATOR_H
#define SHADER_TRANSLATOR_H

#include <GLES3/gl32.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* translated_source;
    bool success;
    char error_msg[512];
    int original_version;
    int target_version;  /* 320 for GLES 3.20 */
} ShaderTranslation;

/* Initialize the shader translator */
bool shader_translator_init(void);
void shader_translator_shutdown(void);

/* Translate a desktop GLSL shader to GLSL ES */
ShaderTranslation shader_translate(const char* source, GLenum shader_type);

/* Free translated shader source */
void shader_translation_free(ShaderTranslation* result);

/* Detect GLSL version from source */
int shader_detect_version(const char* source);

/* Patch specific shader constructs */
char* shader_patch_extensions(const char* source);
char* shader_patch_precision(const char* source, GLenum shader_type);
char* shader_patch_samplers(const char* source);
char* shader_patch_builtins(const char* source);

#ifdef __cplusplus
}
#endif

#endif /* SHADER_TRANSLATOR_H */
