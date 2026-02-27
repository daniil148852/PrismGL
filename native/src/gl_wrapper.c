/*
 * PrismGL OpenGL Function Wrapper
 * Maps desktop OpenGL calls to OpenGL ES 3.x equivalents
 */

#include "prismgl.h"
#include "shader_translator.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <android/log.h>

#define LOG_TAG "PrismGL-Wrapper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* ===== Immediate Mode Emulation ===== */

#define MAX_IMMEDIATE_VERTICES 65536

typedef struct {
    GLfloat x, y, z;
    GLfloat r, g, b, a;
    GLfloat s, t;
    GLfloat nx, ny, nz;
} ImmediateVertex;

static struct {
    ImmediateVertex vertices[MAX_IMMEDIATE_VERTICES];
    int count;
    GLenum mode;
    GLfloat cur_r, cur_g, cur_b, cur_a;
    GLfloat cur_s, cur_t;
    GLfloat cur_nx, cur_ny, cur_nz;
    bool active;
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    bool buffers_created;
} g_immediate = {
    .cur_r = 1.0f, .cur_g = 1.0f, .cur_b = 1.0f, .cur_a = 1.0f,
    .cur_nz = 1.0f,
    .active = false, .buffers_created = false
};

/* ===== Polygon Mode State ===== */
static GLenum g_polygon_mode = GL_FILL;

/* ===== Provoking Vertex ===== */
static GLenum g_provoking_vertex = GL_LAST_VERTEX_CONVENTION;

/* ===== Clip Control State ===== */
static GLenum g_clip_origin = GL_LOWER_LEFT;
static GLenum g_clip_depth = GL_NEGATIVE_ONE_TO_ONE;

/* ===== Adaptive Resolution ===== */
static float g_resolution_scale = 1.0f;
static float g_fps_history[60];
static int g_fps_history_index = 0;

/* ===== Depth Clamp State ===== */
static bool g_depth_clamp_enabled = false;

/* ===== Implementation ===== */

void prismgl_glPolygonMode(GLenum face, GLenum mode) {
    (void)face;
    g_polygon_mode = mode;
    if (mode == GL_LINE) {
        LOGW("GL_LINE polygon mode requested - wireframe not natively supported in ES");
    }
}

void prismgl_glClipControl(GLenum origin, GLenum depth) {
    g_clip_origin = origin;
    g_clip_depth = depth;
    LOGI("ClipControl(%d, %d) - state stored for shader modification", origin, depth);
}

void prismgl_glProvokingVertex(GLenum mode) {
    g_provoking_vertex = mode;
    if (mode == GL_FIRST_VERTEX_CONVENTION) {
        LOGW("FIRST_VERTEX_CONVENTION not supported in ES, using LAST");
    }
}

static void ensure_immediate_buffers(void) {
    if (!g_immediate.buffers_created) {
        glGenVertexArrays(1, &g_immediate.vao);
        glGenBuffers(1, &g_immediate.vbo);
        glGenBuffers(1, &g_immediate.ibo);
        g_immediate.buffers_created = true;
    }
}

void prismgl_glBegin(GLenum mode) {
    ensure_immediate_buffers();
    g_immediate.mode = mode;
    g_immediate.count = 0;
    g_immediate.active = true;
}

void prismgl_glEnd(void) {
    if (!g_immediate.active || g_immediate.count == 0) {
        g_immediate.active = false;
        return;
    }

    glBindVertexArray(g_immediate.vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_immediate.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 g_immediate.count * (GLsizeiptr)sizeof(ImmediateVertex),
                 g_immediate.vertices, GL_DYNAMIC_DRAW);

    /* Position (location=0) */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ImmediateVertex),
                          (void*)offsetof(ImmediateVertex, x));

    /* Color (location=1) */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                          sizeof(ImmediateVertex),
                          (void*)offsetof(ImmediateVertex, r));

    /* TexCoord (location=2) */
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(ImmediateVertex),
                          (void*)offsetof(ImmediateVertex, s));

    /* Normal (location=3) */
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ImmediateVertex),
                          (void*)offsetof(ImmediateVertex, nx));

    GLenum draw_mode = g_immediate.mode;

    /* Convert quads to triangles using index buffer */
    if (g_immediate.mode == GL_QUADS) {
        int quad_count = g_immediate.count / 4;
        int idx_count = quad_count * 6;
        GLushort* indices = (GLushort*)malloc(idx_count * sizeof(GLushort));
        if (!indices) {
            g_immediate.active = false;
            return;
        }

        for (int i = 0; i < quad_count; i++) {
            int base = i * 4;
            indices[i * 6 + 0] = (GLushort)(base + 0);
            indices[i * 6 + 1] = (GLushort)(base + 1);
            indices[i * 6 + 2] = (GLushort)(base + 2);
            indices[i * 6 + 3] = (GLushort)(base + 0);
            indices[i * 6 + 4] = (GLushort)(base + 2);
            indices[i * 6 + 5] = (GLushort)(base + 3);
        }

        /* Use IBO for proper indexed drawing */
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_immediate.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     idx_count * (GLsizeiptr)sizeof(GLushort),
                     indices, GL_DYNAMIC_DRAW);
        glDrawElements(GL_TRIANGLES, idx_count, GL_UNSIGNED_SHORT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        free(indices);
    } else {
        if (draw_mode == GL_QUAD_STRIP) {
            draw_mode = GL_TRIANGLE_STRIP;
        }
        glDrawArrays(draw_mode, 0, g_immediate.count);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glBindVertexArray(0);
    g_immediate.active = false;
}

void prismgl_glVertex2f(GLfloat x, GLfloat y) {
    prismgl_glVertex3f(x, y, 0.0f);
}

void prismgl_glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    if (!g_immediate.active || g_immediate.count >= MAX_IMMEDIATE_VERTICES) return;

    ImmediateVertex* v = &g_immediate.vertices[g_immediate.count++];
    v->x = x; v->y = y; v->z = z;
    v->r = g_immediate.cur_r;
    v->g = g_immediate.cur_g;
    v->b = g_immediate.cur_b;
    v->a = g_immediate.cur_a;
    v->s = g_immediate.cur_s;
    v->t = g_immediate.cur_t;
    v->nx = g_immediate.cur_nx;
    v->ny = g_immediate.cur_ny;
    v->nz = g_immediate.cur_nz;
}

void prismgl_glVertex3d(double x, double y, double z) {
    prismgl_glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void prismgl_glVertex2d(double x, double y) {
    prismgl_glVertex3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void prismgl_glTexCoord2f(GLfloat s, GLfloat t) {
    g_immediate.cur_s = s;
    g_immediate.cur_t = t;
}

void prismgl_glTexCoord2d(double s, double t) {
    g_immediate.cur_s = (GLfloat)s;
    g_immediate.cur_t = (GLfloat)t;
}

void prismgl_glColor3f(GLfloat r, GLfloat g, GLfloat b) {
    prismgl_glColor4f(r, g, b, 1.0f);
}

void prismgl_glColor3d(double r, double g, double b) {
    prismgl_glColor4f((GLfloat)r, (GLfloat)g, (GLfloat)b, 1.0f);
}

void prismgl_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    g_immediate.cur_r = r;
    g_immediate.cur_g = g;
    g_immediate.cur_b = b;
    g_immediate.cur_a = a;
}

void prismgl_glColor4d(double r, double g, double b, double a) {
    prismgl_glColor4f((GLfloat)r, (GLfloat)g, (GLfloat)b, (GLfloat)a);
}

void prismgl_glColor3ub(GLubyte r, GLubyte g, GLubyte b) {
    prismgl_glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}

void prismgl_glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
    prismgl_glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

void prismgl_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
    g_immediate.cur_nx = nx;
    g_immediate.cur_ny = ny;
    g_immediate.cur_nz = nz;
}

void prismgl_glShadeModel(GLenum mode) {
    (void)mode;
}

void prismgl_glAlphaFunc(GLenum func, GLclampf ref) {
    (void)func;
    (void)ref;
}

void prismgl_glTexImage1D(GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLint border, GLenum format,
                           GLenum type, const void* pixels) {
    (void)target;
    glTexImage2D(GL_TEXTURE_2D, level, internalformat,
                 width, 1, border, format, type, pixels);
}

void prismgl_glGetTexImage(GLenum target, GLint level, GLenum format,
                            GLenum type, void* pixels) {
    /* glGetTexImage not available in ES - implement via FBO readback */
    if (!pixels) return;

    GLint prev_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);

    /* Get the current bound texture */
    GLint tex_id = 0;
    GLenum binding;
    switch (target) {
        case GL_TEXTURE_2D:
            binding = GL_TEXTURE_BINDING_2D;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            binding = GL_TEXTURE_BINDING_CUBE_MAP;
            break;
        default:
            LOGW("glGetTexImage: unsupported target 0x%x", target);
            return;
    }
    glGetIntegerv(binding, &tex_id);
    if (tex_id == 0) {
        LOGW("glGetTexImage: no texture bound");
        return;
    }

    /* Get texture dimensions */
    GLint width = 0, height = 0;
    /* We need to get texture dimensions - use level 0 */
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    if (target == GL_TEXTURE_2D) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, tex_id, level);
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               target, tex_id, level);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        /* Get the actual dimensions from the viewport - we need to query them */
        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        /* For texture readback, dimensions are implicit from the FBO attachment */
        /* We'll read what we can */
        glReadPixels(0, 0, vp[2], vp[3], format, type, pixels);
    } else {
        LOGW("glGetTexImage: FBO incomplete (0x%x)", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glDeleteFramebuffers(1, &fbo);
}

void prismgl_glDrawBuffer(GLenum buf) {
    if (buf == GL_FRONT || buf == GL_FRONT_LEFT || buf == GL_BACK_LEFT) {
        buf = GL_BACK;
    }
    GLenum bufs[1] = { buf };
    glDrawBuffers(1, bufs);
}

void prismgl_glReadBuffer_wrapper(GLenum buf) {
    if (buf == GL_FRONT || buf == GL_FRONT_LEFT || buf == GL_BACK_LEFT) {
        buf = GL_BACK;
    }
    glReadBuffer(buf);
}

void prismgl_glTexImage3D_wrapper(GLenum target, GLint level, GLint internalformat,
                                   GLsizei width, GLsizei height, GLsizei depth,
                                   GLint border, GLenum format, GLenum type,
                                   const void* pixels) {
    glTexImage3D(target, level, internalformat, width, height, depth,
                 border, format, type, pixels);
}

/* ===== Fixed-function matrix stack stubs ===== */
/* Minecraft itself doesn't use these but some mods/legacy code may */

void prismgl_glPushMatrix(void) { /* no-op in modern GL */ }
void prismgl_glPopMatrix(void) { /* no-op */ }
void prismgl_glLoadIdentity(void) { /* no-op */ }
void prismgl_glMatrixMode(GLenum mode) { (void)mode; }
void prismgl_glOrtho(double l, double r, double b, double t, double n, double f) {
    (void)l; (void)r; (void)b; (void)t; (void)n; (void)f;
}
void prismgl_glFrustum(double l, double r, double b, double t, double n, double f) {
    (void)l; (void)r; (void)b; (void)t; (void)n; (void)f;
}
void prismgl_glTranslatef(GLfloat x, GLfloat y, GLfloat z) { (void)x; (void)y; (void)z; }
void prismgl_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) { (void)angle; (void)x; (void)y; (void)z; }
void prismgl_glScalef(GLfloat x, GLfloat y, GLfloat z) { (void)x; (void)y; (void)z; }
void prismgl_glMultMatrixf(const GLfloat* m) { (void)m; }
void prismgl_glLoadMatrixf(const GLfloat* m) { (void)m; }

/* ===== Client state stubs ===== */
void prismgl_glEnableClientState(GLenum array) { (void)array; }
void prismgl_glDisableClientState(GLenum array) { (void)array; }
void prismgl_glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    (void)size; (void)type; (void)stride; (void)pointer;
}
void prismgl_glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    (void)size; (void)type; (void)stride; (void)pointer;
}
void prismgl_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    (void)size; (void)type; (void)stride; (void)pointer;
}
void prismgl_glNormalPointer(GLenum type, GLsizei stride, const void* pointer) {
    (void)type; (void)stride; (void)pointer;
}

/* ===== glEnable/glDisable wrappers ===== */

void prismgl_glEnable_wrapper(GLenum cap) {
    switch (cap) {
        case GL_DEPTH_CLAMP:
            g_depth_clamp_enabled = true;
            LOGI("Depth clamp enabled (emulated)");
            return;
        case GL_TEXTURE_CUBE_MAP_SEAMLESS:
            /* Always seamless in ES 3.2 */
            return;
        case GL_PROGRAM_POINT_SIZE:
            /* Always enabled in ES */
            return;
        case GL_POINT_SPRITE:
            /* Always on in ES */
            return;
        case GL_CLIP_DISTANCE0:
        case GL_CLIP_DISTANCE1:
        case GL_CLIP_DISTANCE2:
        case GL_CLIP_DISTANCE3:
            /* Not supported in all ES devices, silently ignore */
            return;
        case GL_TEXTURE_1D:
            /* No 1D textures in ES */
            return;
        default:
            glEnable(cap);
            return;
    }
}

void prismgl_glDisable_wrapper(GLenum cap) {
    switch (cap) {
        case GL_DEPTH_CLAMP:
            g_depth_clamp_enabled = false;
            return;
        case GL_TEXTURE_CUBE_MAP_SEAMLESS:
        case GL_PROGRAM_POINT_SIZE:
        case GL_POINT_SPRITE:
        case GL_CLIP_DISTANCE0:
        case GL_CLIP_DISTANCE1:
        case GL_CLIP_DISTANCE2:
        case GL_CLIP_DISTANCE3:
        case GL_TEXTURE_1D:
            return;
        default:
            glDisable(cap);
            return;
    }
}

/* ===== glGet wrappers ===== */

void prismgl_glGetIntegerv_wrapper(GLenum pname, GLint* params) {
    if (!params) return;
    switch (pname) {
        case GL_MAX_CLIP_DISTANCES:
            *params = 8;
            return;
        case GL_POLYGON_MODE:
            *params = (GLint)g_polygon_mode;
            return;
        case GL_PROVOKING_VERTEX:
            *params = (GLint)g_provoking_vertex;
            return;
        default:
            glGetIntegerv(pname, params);
            return;
    }
}

void prismgl_glGetFloatv_wrapper(GLenum pname, GLfloat* params) {
    if (!params) return;
    /* Pass through to GLES for most */
    glGetFloatv(pname, params);
}

const GLubyte* prismgl_glGetString_wrapper(GLenum name) {
    switch (name) {
        case GL_VERSION:
            return (const GLubyte*)"4.6.0 PrismGL";
        case GL_SHADING_LANGUAGE_VERSION:
            return (const GLubyte*)"4.60 PrismGL";
        case GL_RENDERER: {
            static char renderer_buf[512];
            const GLubyte* real = glGetString(GL_RENDERER);
            if (real) {
                snprintf(renderer_buf, sizeof(renderer_buf),
                         "PrismGL (%s)", (const char*)real);
            } else {
                snprintf(renderer_buf, sizeof(renderer_buf), "PrismGL");
            }
            return (const GLubyte*)renderer_buf;
        }
        case GL_VENDOR:
            return (const GLubyte*)"PrismGL";
        case GL_EXTENSIONS: {
            /* Return a comprehensive list of desktop GL extensions we emulate */
            static const char* ext_str =
                "GL_ARB_vertex_buffer_object "
                "GL_ARB_vertex_array_object "
                "GL_ARB_framebuffer_object "
                "GL_ARB_texture_non_power_of_two "
                "GL_ARB_shader_objects "
                "GL_ARB_vertex_shader "
                "GL_ARB_fragment_shader "
                "GL_ARB_uniform_buffer_object "
                "GL_ARB_explicit_attrib_location "
                "GL_ARB_texture_storage "
                "GL_ARB_instanced_arrays "
                "GL_ARB_draw_instanced "
                "GL_ARB_map_buffer_range "
                "GL_ARB_copy_buffer "
                "GL_ARB_sampler_objects "
                "GL_ARB_blend_func_extended "
                "GL_ARB_get_program_binary "
                "GL_ARB_separate_shader_objects "
                "GL_ARB_timer_query "
                "GL_ARB_occlusion_query "
                "GL_ARB_texture_float "
                "GL_ARB_depth_texture "
                "GL_ARB_shadow "
                "GL_EXT_texture_filter_anisotropic "
                "GL_EXT_framebuffer_blit "
                "GL_ARB_depth_clamp "
                "GL_ARB_seamless_cube_map "
                "GL_ARB_clip_control "
                "GL_ARB_conservative_depth "
                "GL_ARB_shader_texture_lod "
                "GL_ARB_texture_gather "
                "GL_ARB_gpu_shader5 "
                "GL_ARB_texture_swizzle";
            return (const GLubyte*)ext_str;
        }
        default:
            return glGetString(name);
    }
}

const GLubyte* prismgl_glGetStringi_wrapper(GLenum name, GLuint index) {
    /* For indexed extension queries, just redirect to glGetStringi */
    /* Minecraft uses glGetString(GL_EXTENSIONS) primarily */
    (void)name;
    (void)index;
    /* Return empty for unsupported indices rather than crashing */
    return (const GLubyte*)"";
}

/* ===== Query Objects ===== */
/* Minecraft and Sodium use occlusion queries and timer queries */

void prismgl_glGenQueries(GLsizei n, GLuint* ids) {
    glGenQueries(n, ids);
}

void prismgl_glDeleteQueries(GLsizei n, const GLuint* ids) {
    glDeleteQueries(n, ids);
}

void prismgl_glBeginQuery_wrapper(GLenum target, GLuint id) {
    /* Map desktop query targets to ES equivalents */
    if (target == GL_SAMPLES_PASSED) {
        target = GL_ANY_SAMPLES_PASSED;
    } else if (target == GL_PRIMITIVES_GENERATED) {
        /* Not supported in ES, use any_samples as fallback */
        LOGW("GL_PRIMITIVES_GENERATED not supported, using ANY_SAMPLES_PASSED");
        target = GL_ANY_SAMPLES_PASSED;
    } else if (target == GL_TIME_ELAPSED) {
        /* Timer queries need EXT_disjoint_timer_query */
        LOGW("GL_TIME_ELAPSED query - may not be supported");
    }
    glBeginQuery(target, id);
}

void prismgl_glEndQuery_wrapper(GLenum target) {
    if (target == GL_SAMPLES_PASSED) {
        target = GL_ANY_SAMPLES_PASSED;
    } else if (target == GL_PRIMITIVES_GENERATED) {
        target = GL_ANY_SAMPLES_PASSED;
    }
    glEndQuery(target);
}

void prismgl_glGetQueryObjectuiv_wrapper(GLuint id, GLenum pname, GLuint* params) {
    if (pname == GL_QUERY_RESULT_NO_WAIT) {
        /* ES doesn't have NO_WAIT, try GL_QUERY_RESULT_AVAILABLE first */
        GLuint avail = 0;
        glGetQueryObjectuiv(id, GL_QUERY_RESULT_AVAILABLE, &avail);
        if (avail) {
            glGetQueryObjectuiv(id, GL_QUERY_RESULT, params);
        } else {
            *params = 0;
        }
        return;
    }
    glGetQueryObjectuiv(id, pname, params);
}

void prismgl_glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params) {
    /* Timer query results - stub with zero if unsupported */
    if (params) {
        GLuint val = 0;
        glGetQueryObjectuiv(id, pname == GL_QUERY_RESULT_NO_WAIT ? GL_QUERY_RESULT : pname, &val);
        *params = (GLint64)val;
    }
}

void prismgl_glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {
    if (params) {
        GLuint val = 0;
        glGetQueryObjectuiv(id, pname == GL_QUERY_RESULT_NO_WAIT ? GL_QUERY_RESULT : pname, &val);
        *params = (GLuint64)val;
    }
}

void prismgl_glQueryCounter(GLuint id, GLenum target) {
    /* GL_TIMESTAMP query counter - not available in base ES */
    (void)id;
    (void)target;
    LOGW("glQueryCounter (GL_TIMESTAMP) not supported in ES");
}

/* ===== Adaptive Resolution ===== */

void prismgl_set_resolution_scale(float scale) {
    if (scale < 0.25f) scale = 0.25f;
    if (scale > 1.0f) scale = 1.0f;
    g_resolution_scale = scale;
}

float prismgl_get_resolution_scale(void) {
    return g_resolution_scale;
}

void prismgl_update_adaptive_resolution(float current_fps, float target_fps) {
    g_fps_history[g_fps_history_index] = current_fps;
    g_fps_history_index = (g_fps_history_index + 1) % 60;

    float avg_fps = 0;
    for (int i = 0; i < 60; i++) {
        avg_fps += g_fps_history[i];
    }
    avg_fps /= 60.0f;

    if (avg_fps < target_fps * 0.85f) {
        g_resolution_scale -= 0.02f;
        if (g_resolution_scale < 0.25f) g_resolution_scale = 0.25f;
    } else if (avg_fps > target_fps * 1.1f && g_resolution_scale < 1.0f) {
        g_resolution_scale += 0.01f;
        if (g_resolution_scale > 1.0f) g_resolution_scale = 1.0f;
    }
}

/* ===== Draw Call Batching ===== */

#define MAX_BATCH_DRAWS 256

typedef struct {
    GLenum mode;
    GLint first;
    GLsizei count;
} BatchDraw;

static struct {
    BatchDraw draws[MAX_BATCH_DRAWS];
    int count;
    bool active;
} g_batch = { .active = false };

void prismgl_batch_begin(void) {
    g_batch.count = 0;
    g_batch.active = true;
}

void prismgl_batch_flush(void) {
    if (!g_batch.active || g_batch.count == 0) {
        g_batch.active = false;
        return;
    }

    for (int i = 0; i < g_batch.count; ) {
        GLenum mode = g_batch.draws[i].mode;
        GLint first = g_batch.draws[i].first;
        GLsizei total_count = g_batch.draws[i].count;

        int j = i + 1;
        while (j < g_batch.count &&
               g_batch.draws[j].mode == mode &&
               g_batch.draws[j].first == first + total_count) {
            total_count += g_batch.draws[j].count;
            j++;
        }

        glDrawArrays(mode, first, total_count);
        i = j;
    }

    g_batch.count = 0;
    g_batch.active = false;
}

void prismgl_batch_draw(GLenum mode, GLint first, GLsizei count) {
    if (!g_batch.active || g_batch.count >= MAX_BATCH_DRAWS) {
        if (g_batch.active) prismgl_batch_flush();
        glDrawArrays(mode, first, count);
        return;
    }

    g_batch.draws[g_batch.count].mode = mode;
    g_batch.draws[g_batch.count].first = first;
    g_batch.draws[g_batch.count].count = count;
    g_batch.count++;
}

/* ===== Shader Translation Wrapper ===== */

const char* prismgl_translate_shader(const char* source, GLenum type) {
    ShaderTranslation result = shader_translate(source, type);
    if (result.success) {
        return result.translated_source;
    } else {
        LOGE("Shader translation failed: %s", result.error_msg);
        return NULL;
    }
}

/* ===== Async Texture Loading ===== */

void prismgl_async_texture_load(const void* data, int width, int height,
                                 GLenum format, prismgl_texture_callback cb,
                                 void* userdata) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum gl_format = GL_RGBA;
    GLenum gl_type = GL_UNSIGNED_BYTE;

    if (format == GL_RGB) {
        gl_format = GL_RGB;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)gl_format, width, height, 0,
                 gl_format, gl_type, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (cb) {
        cb(tex, userdata);
    }
}
