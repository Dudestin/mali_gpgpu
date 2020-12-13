#pragma once
typedef unsigned short hfloat;

#define TEXTURE_TYPE GL_UNSIGNED_BYTE
#define TEXTURE_TYPE_TOKEN GLubyte
#define TEXTURE_INTERNAL_FMT GL_RGBA8_OES
#define TEXTURE_FORMAT GL_RGBA

/*
#define TEXTURE_TYPE GL_HALF_FLOAT_OES
#define TEXTURE_TYPE_TOKEN hfloat
#define TEXTURE_INTERNAL_FMT GL_RGBA16_EXT
#define TEXTURE_FORMAT GL_RGBA
*/

#define EGL_CHECK(x) \
    x; \
    { \
        EGLint eglError = eglGetError(); \
        if(eglError != EGL_SUCCESS) { \
            fprintf(stderr, "eglGetError() = %i (0x%.8x) at line %i\n", eglError, eglError, __LINE__); \
            exit(1); \
        } \
    }