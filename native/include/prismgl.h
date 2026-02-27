/*
 * PrismGL - OpenGL 4.x to OpenGL ES 3.x Translation Layer
 * High-performance renderer for Minecraft Java Edition on Android
 *
 * Copyright (c) 2026 PrismGL Contributors
 * Licensed under Apache License 2.0
 */

#ifndef PRISMGL_H
#define PRISMGL_H

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== OpenGL constants not in GLES ===== */

/* Polygon modes */
#ifndef GL_FILL
#define GL_FILL                     0x1B02
#endif
#ifndef GL_LINE
#define GL_LINE                     0x1B01
#endif
#ifndef GL_POINT
#define GL_POINT                    0x1B00
#endif
#define GL_POLYGON_MODE             0x0B40

/* Quads */
#define GL_QUADS                    0x0007
#define GL_QUAD_STRIP               0x0008

/* Texture targets */
#define GL_TEXTURE_1D               0x0DE0
#define GL_TEXTURE_1D_ARRAY         0x8C18
#ifndef GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER           0x8C2A
#endif
#define GL_TEXTURE_RECTANGLE        0x84F5
#define GL_PROXY_TEXTURE_2D         0x8064
#define GL_TEXTURE_BINDING_1D       0x8068

/* Texture parameters */
#define GL_TEXTURE_LOD_BIAS         0x8501
#define GL_TEXTURE_BORDER_COLOR     0x1004
#define GL_CLAMP_TO_BORDER          0x812D
#define GL_MIRROR_CLAMP_TO_EDGE     0x8743

/* Framebuffer */
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER         0x8CA9
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER         0x8CA8
#endif

/* Clip distance */
#define GL_CLIP_DISTANCE0           0x3000
#define GL_CLIP_DISTANCE1           0x3001
#define GL_CLIP_DISTANCE2           0x3002
#define GL_CLIP_DISTANCE3           0x3003
#define GL_CLIP_DISTANCE4           0x3004
#define GL_CLIP_DISTANCE5           0x3005
#define GL_CLIP_DISTANCE6           0x3006
#define GL_CLIP_DISTANCE7           0x3007
#define GL_MAX_CLIP_DISTANCES       0x0D32

/* Double precision */
#define GL_DOUBLE                   0x140A

/* Provoking vertex */
#define GL_FIRST_VERTEX_CONVENTION  0x8E4D
#define GL_LAST_VERTEX_CONVENTION   0x8E4E
#define GL_PROVOKING_VERTEX         0x8E4F

/* Query */
#define GL_SAMPLES_PASSED           0x8914
#define GL_PRIMITIVES_GENERATED     0x8C87
#define GL_TIME_ELAPSED             0x88BF
#define GL_TIMESTAMP                0x8E28
#define GL_QUERY_RESULT_NO_WAIT     0x9194

/* Program interface */
#ifndef GL_ACTIVE_RESOURCES
#define GL_ACTIVE_RESOURCES              0x92F5
#endif
#ifndef GL_MAX_NAME_LENGTH
#define GL_MAX_NAME_LENGTH               0x92F6
#endif
#ifndef GL_BUFFER_BINDING
#define GL_BUFFER_BINDING                0x9302
#endif
#ifndef GL_BUFFER_DATA_SIZE
#define GL_BUFFER_DATA_SIZE              0x9303
#endif

/* Texture swizzle */
#ifndef GL_TEXTURE_SWIZZLE_R
#define GL_TEXTURE_SWIZZLE_R        0x8E42
#endif
#ifndef GL_TEXTURE_SWIZZLE_G
#define GL_TEXTURE_SWIZZLE_G        0x8E43
#endif
#ifndef GL_TEXTURE_SWIZZLE_B
#define GL_TEXTURE_SWIZZLE_B        0x8E44
#endif
#ifndef GL_TEXTURE_SWIZZLE_A
#define GL_TEXTURE_SWIZZLE_A        0x8E45
#endif
#define GL_TEXTURE_SWIZZLE_RGBA     0x8E46

/* Program binary */
#ifndef GL_PROGRAM_BINARY_LENGTH
#define GL_PROGRAM_BINARY_LENGTH    0x8741
#endif

/* Buffer mapping */
#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT             0x0001
#endif
#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT            0x0002
#endif
#ifndef GL_MAP_PERSISTENT_BIT
#define GL_MAP_PERSISTENT_BIT       0x0040
#endif
#ifndef GL_MAP_COHERENT_BIT
#define GL_MAP_COHERENT_BIT         0x0080
#endif
#ifndef GL_MAP_INVALIDATE_RANGE_BIT
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#endif
#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif
#ifndef GL_MAP_FLUSH_EXPLICIT_BIT
#define GL_MAP_FLUSH_EXPLICIT_BIT   0x0010
#endif
#ifndef GL_MAP_UNSYNCHRONIZED_BIT
#define GL_MAP_UNSYNCHRONIZED_BIT   0x0020
#endif

/* Clip control */
#define GL_LOWER_LEFT               0x8CA1
#define GL_UPPER_LEFT               0x8CA2
#define GL_NEGATIVE_ONE_TO_ONE      0x935E
#define GL_ZERO_TO_ONE              0x935F

/* Multi-draw */
/* These are desktop GL only */

/* Depth clamp */
#define GL_DEPTH_CLAMP              0x864F

/* Seamless cube map */
#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F

/* Point sprite */
#define GL_POINT_SPRITE             0x8861
#define GL_VERTEX_PROGRAM_POINT_SIZE 0x8642
#define GL_PROGRAM_POINT_SIZE       0x8642

/* Stencil */
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX            0x1901
#endif

/* Draw indirect */
#ifndef GL_DRAW_INDIRECT_BUFFER
#define GL_DRAW_INDIRECT_BUFFER     0x8F3F
#endif

/* glGetString queries for desktop GL */
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C

/* Compatibility mode vertex attribs */
#define GL_VERTEX_ARRAY             0x8074
#define GL_NORMAL_ARRAY             0x8075
#define GL_COLOR_ARRAY              0x8076
#define GL_TEXTURE_COORD_ARRAY      0x8078

/* ===== PrismGL Configuration ===== */

typedef struct {
    bool shader_cache_enabled;
    bool draw_call_batching;
    bool adaptive_resolution;
    bool async_texture_loading;
    bool vulkan_backend;          /* Use Vulkan via ANGLE/Zink if available */
    float resolution_scale;       /* 0.25 - 1.0 */
    int max_cached_shaders;
    int gpu_vendor;               /* 0=unknown, 1=Adreno, 2=Mali, 3=PowerVR */
    char cache_dir[512];
} PrismGLConfig;

/* ===== Initialization ===== */
bool prismgl_init(const char* cache_dir);
void prismgl_shutdown(void);
void prismgl_set_config(const PrismGLConfig* config);
PrismGLConfig* prismgl_get_config(void);

/* ===== GPU Detection ===== */
int prismgl_detect_gpu(void);
const char* prismgl_get_gpu_name(void);
void prismgl_apply_gpu_tweaks(int gpu_vendor);

/* ===== Shader Cache ===== */
bool prismgl_shader_cache_init(const char* cache_dir);
void prismgl_shader_cache_shutdown(void);
GLuint prismgl_shader_cache_get(uint64_t hash);
void prismgl_shader_cache_put(uint64_t hash, GLuint program);
uint64_t prismgl_hash_shader_source(const char* vertex_src, const char* fragment_src);

/* ===== Draw Call Batching ===== */
void prismgl_batch_begin(void);
void prismgl_batch_flush(void);
void prismgl_batch_draw(GLenum mode, GLint first, GLsizei count);

/* ===== Adaptive Resolution ===== */
void prismgl_set_resolution_scale(float scale);
float prismgl_get_resolution_scale(void);
void prismgl_update_adaptive_resolution(float current_fps, float target_fps);

/* ===== Async Texture Loading ===== */
typedef void (*prismgl_texture_callback)(GLuint texture_id, void* userdata);
void prismgl_async_texture_load(const void* data, int width, int height,
                                 GLenum format, prismgl_texture_callback cb,
                                 void* userdata);

/* ===== OpenGL Function Wrappers ===== */
/* These are the main entry points that translate GL 4.x calls to GLES 3.x */
void prismgl_glPolygonMode(GLenum face, GLenum mode);
void prismgl_glClipControl(GLenum origin, GLenum depth);
void prismgl_glProvokingVertex(GLenum mode);
void prismgl_glBegin(GLenum mode);
void prismgl_glEnd(void);
void prismgl_glVertex2f(GLfloat x, GLfloat y);
void prismgl_glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void prismgl_glVertex3d(double x, double y, double z);
void prismgl_glVertex2d(double x, double y);
void prismgl_glTexCoord2f(GLfloat s, GLfloat t);
void prismgl_glTexCoord2d(double s, double t);
void prismgl_glColor3f(GLfloat r, GLfloat g, GLfloat b);
void prismgl_glColor3d(double r, double g, double b);
void prismgl_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void prismgl_glColor4d(double r, double g, double b, double a);
void prismgl_glColor3ub(GLubyte r, GLubyte g, GLubyte b);
void prismgl_glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
void prismgl_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void prismgl_glShadeModel(GLenum mode);
void prismgl_glAlphaFunc(GLenum func, GLclampf ref);
void prismgl_glTexImage1D(GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLint border, GLenum format,
                           GLenum type, const void* pixels);
void prismgl_glGetTexImage(GLenum target, GLint level, GLenum format,
                            GLenum type, void* pixels);
void prismgl_glDrawBuffer(GLenum buf);
void prismgl_glReadBuffer_wrapper(GLenum buf);

/* Additional GL wrappers for Minecraft compatibility */
void prismgl_glTexImage3D_wrapper(GLenum target, GLint level, GLint internalformat,
                                   GLsizei width, GLsizei height, GLsizei depth,
                                   GLint border, GLenum format, GLenum type,
                                   const void* pixels);
void prismgl_glPushMatrix(void);
void prismgl_glPopMatrix(void);
void prismgl_glLoadIdentity(void);
void prismgl_glMatrixMode(GLenum mode);
void prismgl_glOrtho(double left, double right, double bottom, double top,
                      double near_val, double far_val);
void prismgl_glFrustum(double left, double right, double bottom, double top,
                        double near_val, double far_val);
void prismgl_glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void prismgl_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void prismgl_glScalef(GLfloat x, GLfloat y, GLfloat z);
void prismgl_glMultMatrixf(const GLfloat* m);
void prismgl_glLoadMatrixf(const GLfloat* m);
void prismgl_glEnableClientState(GLenum array);
void prismgl_glDisableClientState(GLenum array);
void prismgl_glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void prismgl_glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void prismgl_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void prismgl_glNormalPointer(GLenum type, GLsizei stride, const void* pointer);

/* Depth clamp and point size */
void prismgl_glEnable_wrapper(GLenum cap);
void prismgl_glDisable_wrapper(GLenum cap);
void prismgl_glGetIntegerv_wrapper(GLenum pname, GLint* params);
void prismgl_glGetFloatv_wrapper(GLenum pname, GLfloat* params);
const GLubyte* prismgl_glGetString_wrapper(GLenum name);
const GLubyte* prismgl_glGetStringi_wrapper(GLenum name, GLuint index);

/* Query objects - stub implementation for Minecraft */
void prismgl_glGenQueries(GLsizei n, GLuint* ids);
void prismgl_glDeleteQueries(GLsizei n, const GLuint* ids);
void prismgl_glBeginQuery_wrapper(GLenum target, GLuint id);
void prismgl_glEndQuery_wrapper(GLenum target);
void prismgl_glGetQueryObjectuiv_wrapper(GLuint id, GLenum pname, GLuint* params);
void prismgl_glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params);
void prismgl_glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params);
void prismgl_glQueryCounter(GLuint id, GLenum target);

/* Shader translation */
const char* prismgl_translate_shader(const char* source, GLenum type);

/* Proc address loader */
void* prismgl_get_proc_address(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* PRISMGL_H */
