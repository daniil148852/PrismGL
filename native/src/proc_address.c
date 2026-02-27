/*
 * PrismGL Proc Address Loader
 * Maps OpenGL function names to PrismGL implementations or native GLES functions
 */

#include "prismgl.h"

#include <string.h>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "PrismGL-ProcAddr"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

typedef struct {
    const char* name;
    void* func;
} GLFuncEntry;

/* No-op stub for functions we intentionally ignore */
static void stub_noop(void) {}
static void stub_noop_1i(int a) { (void)a; }
static void stub_noop_1f(float a) { (void)a; }

/* Function table for overridden GL functions */
static const GLFuncEntry g_overrides[] = {
    /* ===== Immediate mode ===== */
    { "glBegin",              (void*)prismgl_glBegin },
    { "glEnd",                (void*)prismgl_glEnd },
    { "glVertex2f",           (void*)prismgl_glVertex2f },
    { "glVertex3f",           (void*)prismgl_glVertex3f },
    { "glVertex2d",           (void*)prismgl_glVertex2d },
    { "glVertex3d",           (void*)prismgl_glVertex3d },
    { "glTexCoord2f",         (void*)prismgl_glTexCoord2f },
    { "glTexCoord2d",         (void*)prismgl_glTexCoord2d },
    { "glColor3f",            (void*)prismgl_glColor3f },
    { "glColor3d",            (void*)prismgl_glColor3d },
    { "glColor4f",            (void*)prismgl_glColor4f },
    { "glColor4d",            (void*)prismgl_glColor4d },
    { "glColor3ub",           (void*)prismgl_glColor3ub },
    { "glColor4ub",           (void*)prismgl_glColor4ub },
    { "glNormal3f",           (void*)prismgl_glNormal3f },

    /* ===== State management ===== */
    { "glPolygonMode",        (void*)prismgl_glPolygonMode },
    { "glClipControl",        (void*)prismgl_glClipControl },
    { "glProvokingVertex",    (void*)prismgl_glProvokingVertex },
    { "glShadeModel",         (void*)prismgl_glShadeModel },
    { "glAlphaFunc",          (void*)prismgl_glAlphaFunc },
    { "glEnable",             (void*)prismgl_glEnable_wrapper },
    { "glDisable",            (void*)prismgl_glDisable_wrapper },
    { "glGetIntegerv",        (void*)prismgl_glGetIntegerv_wrapper },
    { "glGetFloatv",          (void*)prismgl_glGetFloatv_wrapper },
    { "glGetString",          (void*)prismgl_glGetString_wrapper },
    { "glGetStringi",         (void*)prismgl_glGetStringi_wrapper },

    /* ===== Texture ===== */
    { "glTexImage1D",         (void*)prismgl_glTexImage1D },
    { "glGetTexImage",        (void*)prismgl_glGetTexImage },

    /* ===== Framebuffer ===== */
    { "glDrawBuffer",         (void*)prismgl_glDrawBuffer },
    { "glReadBuffer",         (void*)prismgl_glReadBuffer_wrapper },

    /* ===== Fixed function matrix (legacy) ===== */
    { "glPushMatrix",         (void*)prismgl_glPushMatrix },
    { "glPopMatrix",          (void*)prismgl_glPopMatrix },
    { "glLoadIdentity",       (void*)prismgl_glLoadIdentity },
    { "glMatrixMode",         (void*)prismgl_glMatrixMode },
    { "glOrtho",              (void*)prismgl_glOrtho },
    { "glFrustum",            (void*)prismgl_glFrustum },
    { "glTranslatef",         (void*)prismgl_glTranslatef },
    { "glRotatef",            (void*)prismgl_glRotatef },
    { "glScalef",             (void*)prismgl_glScalef },
    { "glMultMatrixf",        (void*)prismgl_glMultMatrixf },
    { "glLoadMatrixf",        (void*)prismgl_glLoadMatrixf },

    /* ===== Client state (legacy) ===== */
    { "glEnableClientState",  (void*)prismgl_glEnableClientState },
    { "glDisableClientState", (void*)prismgl_glDisableClientState },
    { "glVertexPointer",      (void*)prismgl_glVertexPointer },
    { "glColorPointer",       (void*)prismgl_glColorPointer },
    { "glTexCoordPointer",    (void*)prismgl_glTexCoordPointer },
    { "glNormalPointer",      (void*)prismgl_glNormalPointer },

    /* ===== Query objects ===== */
    { "glGenQueries",         (void*)prismgl_glGenQueries },
    { "glDeleteQueries",      (void*)prismgl_glDeleteQueries },
    { "glBeginQuery",         (void*)prismgl_glBeginQuery_wrapper },
    { "glEndQuery",           (void*)prismgl_glEndQuery_wrapper },
    { "glGetQueryObjectuiv",  (void*)prismgl_glGetQueryObjectuiv_wrapper },
    { "glGetQueryObjecti64v", (void*)prismgl_glGetQueryObjecti64v },
    { "glGetQueryObjectui64v",(void*)prismgl_glGetQueryObjectui64v },
    { "glQueryCounter",       (void*)prismgl_glQueryCounter },

    /* ===== No-op stubs for unsupported desktop GL ===== */
    { "glPushAttrib",         (void*)stub_noop_1i },
    { "glPopAttrib",          (void*)stub_noop },
    { "glPushClientAttrib",   (void*)stub_noop_1i },
    { "glPopClientAttrib",    (void*)stub_noop },
    { "glLineWidth",          (void*)stub_noop_1f },
    { "glPointSize",          (void*)stub_noop_1f },
    { "glLogicOp",            (void*)stub_noop_1i },
    { "glClampColor",         (void*)stub_noop },

    /* Sentinel */
    { NULL, NULL }
};

static void* g_gles_lib = NULL;
static void* g_gles2_lib = NULL;
static void* g_egl_lib = NULL;

static void ensure_libs_loaded(void) {
    if (!g_gles_lib) {
        g_gles_lib = dlopen("libGLESv3.so", RTLD_NOW);
    }
    if (!g_gles2_lib) {
        g_gles2_lib = dlopen("libGLESv2.so", RTLD_NOW);
    }
    if (!g_egl_lib) {
        g_egl_lib = dlopen("libEGL.so", RTLD_NOW);
    }
}

void* prismgl_get_proc_address(const char* name) {
    if (!name) return NULL;

    /* First check our overrides */
    for (int i = 0; g_overrides[i].name != NULL; i++) {
        if (strcmp(name, g_overrides[i].name) == 0) {
            return g_overrides[i].func;
        }
    }

    /* Try EGL proc address */
    ensure_libs_loaded();

    typedef void* (*PFNEGLGETPROCADDRESSPROC)(const char*);
    static PFNEGLGETPROCADDRESSPROC eglGetProcAddr = NULL;
    if (!eglGetProcAddr && g_egl_lib) {
        eglGetProcAddr = (PFNEGLGETPROCADDRESSPROC)dlsym(g_egl_lib, "eglGetProcAddress");
    }

    void* func = NULL;
    if (eglGetProcAddr) {
        func = eglGetProcAddr(name);
    }

    /* Try direct dlsym from GLES3 library */
    if (!func && g_gles_lib) {
        func = dlsym(g_gles_lib, name);
    }

    /* Try GLES2 library */
    if (!func && g_gles2_lib) {
        func = dlsym(g_gles2_lib, name);
    }

    if (!func) {
        /* Try with EXT/ARB/OES suffixes removed */
        char base_name[256];
        strncpy(base_name, name, sizeof(base_name) - 1);
        base_name[sizeof(base_name) - 1] = '\0';

        /* Try adding OES suffix (some functions need it in ES) */
        char oes_name[256];
        snprintf(oes_name, sizeof(oes_name), "%sOES", name);
        if (eglGetProcAddr) func = eglGetProcAddr(oes_name);
        if (!func && g_gles_lib) func = dlsym(g_gles_lib, oes_name);

        /* Try adding EXT suffix */
        if (!func) {
            char ext_name[256];
            snprintf(ext_name, sizeof(ext_name), "%sEXT", name);
            if (eglGetProcAddr) func = eglGetProcAddr(ext_name);
            if (!func && g_gles_lib) func = dlsym(g_gles_lib, ext_name);
        }

        /* Try stripping EXT/ARB/NV/AMD suffix */
        if (!func) {
            char* ext = strstr(base_name, "EXT");
            if (!ext) ext = strstr(base_name, "ARB");
            if (!ext) ext = strstr(base_name, "NV");
            if (!ext) ext = strstr(base_name, "AMD");
            if (!ext) ext = strstr(base_name, "OES");

            if (ext) {
                *ext = '\0';
                if (eglGetProcAddr) func = eglGetProcAddr(base_name);
                if (!func && g_gles_lib) func = dlsym(g_gles_lib, base_name);
                if (!func && g_gles2_lib) func = dlsym(g_gles2_lib, base_name);
            }
        }
    }

    if (!func) {
        LOGW("Function not found: %s (returning NULL)", name);
    }

    return func;
}
