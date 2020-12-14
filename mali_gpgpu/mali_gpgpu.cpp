#include <time.h>
#include "mali_gpgpu.h"

constexpr GLuint texSize = 660;
constexpr unsigned int uiWidth = texSize;
constexpr unsigned int uiHeight = texSize;
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
    uniform sampler2D textureC;
    uniform sampler2D textureD;
    void main(void){
        vec4 A = texture2D(textureA, v_texCoord);
        vec4 B = texture2D(textureB, v_texCoord);
        vec4 C = texture2D(textureC, v_texCoord);
        vec4 D = texture2D(textureD, v_texCoord);
        gl_FragColor = (A * B) + (C * D);
    }
    )";


int main(int argc, char** argv)
{
    EGLDisplay	sEGLDisplay;
    EGLContext	sEGLContext;
    EGLSurface	sEGLSurface;
    EGLConfig	aEGLConfigs[1];
    EGLint		cEGLConfigs;
    
    // Environment Dependent Region Start
    hDisplay = EGL_DEFAULT_DISPLAY;

    sEGLDisplay = EGL_CHECK(eglGetDisplay(hDisplay));

    EGL_CHECK(eglInitialize(sEGLDisplay, NULL, NULL));
    EGL_CHECK(eglChooseConfig(sEGLDisplay, aEGLAttributes, aEGLConfigs, 1, &cEGLConfigs));

    if (cEGLConfigs == 0)
    {
        printf("No EGL configurations were returned.\n");
        exit(-1);
    }

    hWindow = create_window(4, 4);

    sEGLSurface = EGL_CHECK(eglCreateWindowSurface(sEGLDisplay,
        aEGLConfigs[0], (EGLNativeWindowType)hWindow, NULL));

    // end Environment Dependent Region End
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

    TEXTURE_TYPE_TOKEN* dataA = new TEXTURE_TYPE_TOKEN[arraySize];
    for (int i = 0; i < arraySize; ++i) {
        float hoge = i;
        dataA[i] = i;
    }
    auto locA = glGetUniformLocation(glslProgram, "textureA");
    auto texA = new textureManager(texSize, texSize, dataA, GL_TEXTURE0, locA);

    TEXTURE_TYPE_TOKEN* dataB = new TEXTURE_TYPE_TOKEN[arraySize];
    for (int i = 0; i < arraySize; ++i) {
        dataB[i] = i;
    }
    auto locB = glGetUniformLocation(glslProgram, "textureB");
    auto texB = new textureManager(texSize, texSize, dataB, GL_TEXTURE1, locB);

    TEXTURE_TYPE_TOKEN* dataC = new TEXTURE_TYPE_TOKEN[arraySize];
    for (int i = 0; i < arraySize; ++i) {
        dataC[i] = i;
    }
    auto locC = glGetUniformLocation(glslProgram, "textureC");
    auto texC = new textureManager(texSize, texSize, dataC, GL_TEXTURE2, locC);

    TEXTURE_TYPE_TOKEN* dataD = new TEXTURE_TYPE_TOKEN[arraySize];
    for (int i = 0; i < arraySize; ++i) {
        dataD[i] = i;
    }
    auto locD = glGetUniformLocation(glslProgram, "textureD");
    auto texD = new textureManager(texSize, texSize, dataD, GL_TEXTURE3, locD);

    // create vertex
    float vertex_position[] = {
        -1.0f,  -1.0f,
        1.0f,  -1.0f,
        1.0f,   1.0f,
        -1.0f,  1.0f
    };

    GLubyte indices[] = {
        0,1,2,2,3,0
    };

    auto positionLocation = glGetAttribLocation(glslProgram, "v_position");
    glEnableVertexAttribArray(positionLocation);
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, false, 0, vertex_position);

    texA->bind();
    texB->bind();

    glViewport(0, 0, uiWidth, uiHeight);
    
    TEXTURE_TYPE_TOKEN* pixels = new TEXTURE_TYPE_TOKEN[texElementSize];
    double start = clock();
    glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(GLubyte),
        GL_UNSIGNED_BYTE, indices);
    FBOMng->readPixels(0, 0, uiWidth, uiHeight, TEXTURE_FORMAT, TEXTURE_TYPE, pixels);

    double end = clock();

    /*
    std::cout << "dataA" << std::endl;
    for (int i = 0; i < texElementSize; i += 4)
        printf("%p\t%u\t%u\t%u\t%u\n", &dataA[i], dataA[i],
            dataA[i + 1], dataA[i + 2], dataA[i + 3]);

    std::cout << "dataB" << std::endl;
    for (int i = 0; i < texElementSize; i += 4)
        printf("%p\t%u\t%u\t%u\t%u\n", &dataB[i], dataB[i],
            dataB[i + 1], dataB[i + 2], dataB[i + 3]);

    std::cout << "result" << std::endl;
    for (int i = 0; i < texElementSize; i += 4)
        printf("%p\t%u\t%u\t%u\t%u\n", &pixels[i], pixels[i],
            pixels[i + 1], pixels[i + 2], pixels[i + 3]);
    */
    
    std::cout << (end-start)/CLOCKS_PER_SEC << std::endl;

    start = clock();
    TEXTURE_TYPE_TOKEN* comp = new TEXTURE_TYPE_TOKEN[texElementSize];
    for (int i = 0; i < texElementSize; ++i) {
        comp[i] = dataA[i] * dataB[i] + dataC[i] * dataD[i];
    }
    end = clock();
    std::cout << (end - start) / CLOCKS_PER_SEC << std::endl;

    delete texA, texB, texC, texD;
    delete[] dataA, dataB, dataC, dataD;
    delete[] pixels, comp;
    delete FBOMng;
    delete shaderMng;
    EGL_CHECK(eglMakeCurrent(sEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    EGL_CHECK(eglDestroySurface(sEGLDisplay, sEGLSurface));
    EGL_CHECK(eglDestroyContext(sEGLDisplay, sEGLContext));
    EGL_CHECK(eglTerminate(sEGLDisplay));
    return 0;
}