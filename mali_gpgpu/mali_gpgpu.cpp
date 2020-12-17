#include <time.h>
#include "mali_gpgpu.h"

constexpr GLuint texSize = 8;
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
    #define EPS 1.0/255.0
    #define SIGMOID_COEF 6.0
    precision lowp float;
    varying vec2 v_texCoord;
    uniform sampler2D textureA;
    uniform sampler2D textureB;
    uniform sampler2D textureC;
    uniform sampler2D textureD;

    vec4 u2s(vec4 uvec){
        // convert normalized unsigned value [0, 1] into signed value [-1.0, 1.0]
        bvec4 isMinus = greaterThan(uvec, vec4(0.5));
        vec4 signed = (uvec - vec4(isMinus));
        return signed * 2.0; // normalized value
    }
    
    vec4 s2u(vec4 svec){
        // convert [-1.0, 1.0] signed value into unsigned value [0, 1]
        bvec4 isMinus = lessThan(svec, vec4(0.0));
        vec4 uns = (svec / 2.0 + vec4(isMinus));
        return uns; // [0.0, 1.0]
    }

    vec4 sigmoid(vec4 v){
        return 1.0 / (1.0 + exp( -SIGMOID_COEF * v));
    }

    void main(void){
        // aggregation procedure
        vec4 segTex0 = u2s(texture2D(textureA, v_texCoord));
        vec4 segTex1 = u2s(texture2D(textureB, v_texCoord));
        vec4 detTex0 = u2s(texture2D(textureC, v_texCoord));
        vec4 detTex1 = u2s(texture2D(textureD, v_texCoord));
        // segment-branch-prepare
        vec4 sigmoid_segTex0 = sigmoid(segTex0); // [-1.0, 1.0] -> [0.0, 1.0]
        vec4 sigmoid_segTex1 = sigmoid(segTex1);
        // aggregationd
        vec4 mul0 = sigmoid_segTex0 * detTex0; // [-1.0, 1.0] * [-1.0, 1.0] = [-1.0, 1.0]
        vec4 mul1 = sigmoid_segTex1 * detTex1;
        vec4 result = mul0 + mul1; // [-1.0, 1.0] + [-1.0, 1.0] = [-2.0, 2.0]
        gl_FragColor = s2u(result / 2.0);
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
    auto shaderMng = std::make_unique<shaderManager>(vtxsource, flgsource);
    shaderMng->useProgram();
    auto glslProgram = (shaderMng->glslProgram);

    // create FBO
    auto FBOMng = std::make_unique<fboManager>(uiWidth, uiHeight);
    fboManager::checkCurrentFBOStatus(); // check status

    // create texture
    constexpr GLuint arraySize = texElementSize;

    std::unique_ptr<TEXTURE_TYPE_TOKEN[]> dataA(new TEXTURE_TYPE_TOKEN[arraySize]);
    for (int i = 0; i < arraySize; ++i) {
        float hoge = i;
        dataA[i] = i;
    }
    auto locA = glGetUniformLocation(glslProgram, "textureA");
    auto texA = std::make_unique<textureManager>(texSize, texSize, dataA.get(), GL_TEXTURE0, locA);

    std::unique_ptr<TEXTURE_TYPE_TOKEN[]> dataB(new TEXTURE_TYPE_TOKEN[arraySize]);
    for (int i = 0; i < arraySize; ++i) {
        dataB[i] = i;
    }
    auto locB = glGetUniformLocation(glslProgram, "textureB");
    auto texB = std::make_unique<textureManager>(texSize, texSize, dataB.get(), GL_TEXTURE1, locB);

    std::unique_ptr<TEXTURE_TYPE_TOKEN[]> dataC(new TEXTURE_TYPE_TOKEN[arraySize]);
    for (int i = 0; i < arraySize; ++i) {
        dataC[i] = i;
    }
    auto locC = glGetUniformLocation(glslProgram, "textureC");
    auto texC = std::make_unique<textureManager>(texSize, texSize, dataC.get(), GL_TEXTURE2, locC);

    std::unique_ptr<TEXTURE_TYPE_TOKEN[]> dataD(new TEXTURE_TYPE_TOKEN[arraySize]);
    for (int i = 0; i < arraySize; ++i) {
        dataD[i] = i;
    }
    auto locD = glGetUniformLocation(glslProgram, "textureD");
    auto texD = std::make_unique<textureManager>(texSize, texSize, dataD.get(), GL_TEXTURE3, locD);

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

    std::unique_ptr<TEXTURE_TYPE_TOKEN[]> pixels(new TEXTURE_TYPE_TOKEN[arraySize]);
    double start = clock();
    glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(GLubyte),
        GL_UNSIGNED_BYTE, indices);
    FBOMng->readPixels(0, 0, uiWidth, uiHeight, TEXTURE_FORMAT, TEXTURE_TYPE, pixels.get());

    double end = clock();

    
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
    
    
    std::cout << (end-start)/CLOCKS_PER_SEC << std::endl;

    start = clock();
    std::unique_ptr<TEXTURE_TYPE_TOKEN[]> comp(new TEXTURE_TYPE_TOKEN[arraySize]);
    for (int i = 0; i < texElementSize; ++i) {
        comp[i] = dataA[i] * dataB[i];
    }
    end = clock();
    std::cout << (end - start) / CLOCKS_PER_SEC << std::endl;

    texA.reset(); texB.reset(); texC.reset(); texD.reset();
    FBOMng.reset();
    shaderMng.reset();
    EGL_CHECK(eglMakeCurrent(sEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    EGL_CHECK(eglDestroySurface(sEGLDisplay, sEGLSurface));
    EGL_CHECK(eglDestroyContext(sEGLDisplay, sEGLContext));
    EGL_CHECK(eglTerminate(sEGLDisplay));
    return 0;
}