#include "mali_gpgpu.h"

constexpr unsigned int uiWidth =  4;
constexpr unsigned int uiHeight = 4;
constexpr GLuint texSize = 4;
constexpr GLuint texElementSize = 4 * texSize * texSize;

HWND hWindow;
HDC  hDisplay;

/* EGL Configuration */
EGLint aEGLAttributes[] = 
{
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

EGLint aEGLContextAttributes[] = 
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

const GLchar* vtxsource = R"(
    attribute vec2 v_position;
    varying vec2 v_texCoord;
    void main(void)
    {
        v_texCoord = (v_position + vec2(1.0)) * 0.5;
        gl_Position = vec4(v_position, 0.0, 1.0);
    }
    )";
const GLchar* flgsource = R"(
    precision lowp float;
    varying vec2 v_texCoord;
    uniform sampler2D textureA;
    uniform sampler2D textureB;
    void main(void){
        vec4 A = texture2D(textureA, v_texCoord);
        vec4 B = texture2D(textureB, v_texCoord);
        gl_FragColor = A * B;
    }
    )";

int main(int argc, char** argv) 
{
    EGLDisplay	sEGLDisplay;
    EGLContext	sEGLContext;
    EGLSurface	sEGLSurface;
    EGLConfig	aEGLConfigs[1];
    EGLint		cEGLConfigs;

    hDisplay = EGL_DEFAULT_DISPLAY;

    sEGLDisplay = EGL_CHECK(eglGetDisplay(hDisplay));

    EGL_CHECK(eglInitialize(sEGLDisplay, NULL, NULL));
    EGL_CHECK(eglChooseConfig(sEGLDisplay, aEGLAttributes, aEGLConfigs, 1, &cEGLConfigs));

    if (cEGLConfigs == 0) 
    {
        printf("No EGL configurations were returned.\n");
        exit(-1);
    }

    hWindow = create_window(uiWidth, uiHeight);

    sEGLSurface = EGL_CHECK(eglCreateWindowSurface(sEGLDisplay,
        aEGLConfigs[0], (EGLNativeWindowType)hWindow, NULL));

    if (sEGLSurface == EGL_NO_SURFACE) 
    {
        printf("Failed to create EGL surface.\n");
        exit(-1);
    }

    sEGLContext = EGL_CHECK(eglCreateContext(sEGLDisplay,
        aEGLConfigs[0], EGL_NO_CONTEXT, aEGLContextAttributes));

    if (sEGLContext == EGL_NO_CONTEXT) 
    {
        printf("Failed to create EGL context.\n");
        exit(-1);
    }

    eglMakeCurrent(sEGLDisplay, sEGLSurface, sEGLSurface, sEGLContext);

    // create program obj
    auto shaderMng = new shaderManager(vtxsource, flgsource);
    shaderMng->useProgram();
    auto glslProgram = (shaderMng->glslProgram);

    // create FBO
    auto FBOMng = new fboManager(uiWidth, uiHeight);
    fboManager::checkCurrentFBOStatus(); // check status
    
    // create texture
    constexpr GLuint arraySize = texElementSize;

    GLubyte dataA[arraySize];
    for (int i = 0; i < arraySize; ++i) {
        dataA[i] = 2*i;
    }
    auto locA = glGetUniformLocation(glslProgram, "textureA");
    auto texA = new textureManager(texSize, texSize, dataA, GL_TEXTURE0, locA);

    auto dataB = new GLubyte[arraySize];
    for (int i = 0; i < arraySize; ++i) {
        dataB[i] = 3*i;
    }
    auto locB = glGetUniformLocation(glslProgram, "textureB");
    auto texB = new textureManager(texSize, texSize, dataB, GL_TEXTURE1, locB);

    // create vertex
    float vertex_position[] = {
        -1.0f,  -1.0f,
        1.0f    -1.0f,
        1.0f,   1.0f,
        -1.0f,  1.0f
    };

    const GLfloat vertex_uv[] = {
        0.f,    0.f,
        1.f,    0.f,
        1.f,    1.f,
        0.f,    1.f
    };

    auto positionLocation = glGetAttribLocation(glslProgram, "v_position");
    auto uvLocation       = glGetAttribLocation(glslProgram, "v_uv");

    glEnableVertexAttribArray(positionLocation);
    glEnableVertexAttribArray(uvLocation);

    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, false, 0, vertex_position);
    glVertexAttribPointer(uvLocation, 2, GL_FLOAT, false, 0, vertex_uv);

    texA->bind();
    texB->bind();

    glViewport(0, 0, uiWidth, uiHeight);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    GLubyte pixels[texElementSize];
    FBOMng->readPixels(0, 0, uiWidth, uiHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    std::cout << "dataA" << std::endl;
    for (int i = 0; i < texElementSize; i += 4)
        printf("%p\t%d\t%d\t%d\t%d\n", &dataA[i], dataA[i],
            dataA[i + 1], dataA[i + 2], dataA[i + 3]);

    std::cout << "dataB" << std::endl;
    for (int i = 0; i < texElementSize; i += 4)
        printf("%p\t%d\t%d\t%d\t%d\n", &dataB[i], dataB[i],
            dataB[i + 1], dataB[i + 2], dataB[i + 3]);

    std::cout << "result" << std::endl;
    for (int i = 0; i < texElementSize; i += 4)
        printf("%p\t%d\t%d\t%d\t%d\n", &pixels[i], pixels[i],
            pixels[i + 1], pixels[i + 2], pixels[i + 3]);

    delete texA, texB;
    delete FBOMng;
    delete shaderMng;
    EGL_CHECK(eglMakeCurrent(sEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    EGL_CHECK(eglDestroySurface(sEGLDisplay, sEGLSurface));
    EGL_CHECK(eglDestroyContext(sEGLDisplay, sEGLContext));
    EGL_CHECK(eglTerminate(sEGLDisplay));
    return 0;
}