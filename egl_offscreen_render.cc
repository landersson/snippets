#include <iostream>
#include <cstdlib>
#include <fstream>
#include <malloc.h>
#include <EGL/egl.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

static const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE
};

static const int pbufferWidth = 512;
static const int pbufferHeight = 512;

static const EGLint pbufferAttribs[] = {
    EGL_WIDTH, pbufferWidth,
    EGL_HEIGHT, pbufferHeight,
    EGL_NONE,
};

std::string eglErrorString(EGLint error)
{
    switch(error)
    {
    case EGL_SUCCESS: return "No error";
    case EGL_NOT_INITIALIZED: return "EGL not initialized or failed to initialize";
    case EGL_BAD_ACCESS: return "Resource inaccessible";
    case EGL_BAD_ALLOC: return "Cannot allocate resources";
    case EGL_BAD_ATTRIBUTE: return "Unrecognized attribute or attribute value";
    case EGL_BAD_CONTEXT: return "Invalid EGL context";
    case EGL_BAD_CONFIG: return "Invalid EGL frame buffer configuration";
    case EGL_BAD_CURRENT_SURFACE: return "Current surface is no longer valid";
    case EGL_BAD_DISPLAY: return "Invalid EGL display";
    case EGL_BAD_SURFACE: return "Invalid surface";
    case EGL_BAD_MATCH: return "Inconsistent arguments";
    case EGL_BAD_PARAMETER: return "Invalid argument";
    case EGL_BAD_NATIVE_PIXMAP: return "Invalid native pixmap";
    case EGL_BAD_NATIVE_WINDOW: return "Invalid native window";
    case EGL_CONTEXT_LOST: return "Context lost";
    }
    return "Unknown error ";
}

void egl_check_error(const char *msg)
{
    int e = eglGetError();
    if (e != EGL_SUCCESS)
    {
        std::cerr << "EGL Error (" << msg << "): " << eglErrorString(e) << std::endl;
        exit(1);
    }
}

static const float points[] = {
   0.0f,  0.5f,  0.0f,
   0.5f, -0.5f,  0.0f,
  -0.5f, -0.5f,  0.0f
};

static const char* vertex_shader =
    "#version 400\n"
    "in vec3 vp;"
    "void main() {"
    "  gl_Position = vec4(vp, 1.0);"
    "}";

static const char* fragment_shader =
    "#version 400\n"
    "out vec4 frag_colour;"
    "void main() {"
    "  frag_colour = vec4(0.0, 0.7, 0.8, 1.0);"
    "}";


void render()
{
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), points, GL_STATIC_DRAW);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, nullptr);
    glCompileShader(fs);

    GLuint shader_programme = glCreateProgram();
    glAttachShader(shader_programme, fs);
    glAttachShader(shader_programme, vs);
    glLinkProgram(shader_programme);

    glClearColor(0.3, 0.3, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_programme);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFlush();
}

int main(int argc, char *argv[])
{
    // 1. Initialize EGL
    EGLDisplay eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    egl_check_error("eglGetDisplay");

    EGLint major, minor;

    bool b = eglInitialize(eglDpy, &major, &minor);
    egl_check_error("eglInitialize");

    printf("EGL Vendor: %s\n", eglQueryString(eglDpy, EGL_VENDOR));
    printf("EGL Version: %s\n", eglQueryString(eglDpy, EGL_VERSION));

    // 2. Select an appropriate configuration
    EGLint numConfigs;
    EGLConfig eglCfg;

    eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
    egl_check_error("eglChooseConfig");

    // 3. Create a surface
    EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg, pbufferAttribs);
    egl_check_error("eglCreatePbufferSurface");

    // 4. Bind the API
    eglBindAPI(EGL_OPENGL_API);
    egl_check_error("eglBindAPI");

    // 5. Create a context and make it current
    EGLContext eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, nullptr);

    eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);
    egl_check_error("eglMakeCurrent");

    for (unsigned i = 0; i < 10; i++)
        render();
    eglWaitGL();

    uint8_t* buffer = (uint8_t*)malloc(pbufferWidth * pbufferHeight * 3);
    glReadPixels(0, 0, pbufferWidth, pbufferHeight, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    egl_check_error("glReadPixels");

    std::ofstream ofs;
    ofs.open("egl.pnm");
    ofs << "P6\n" << pbufferWidth << " " << pbufferHeight << "\n255\n";
    ofs.write((char*)buffer, pbufferWidth * pbufferHeight * 3);
    ofs.close();

    // 6. Terminate EGL when finished
    eglWaitGL();
    eglTerminate(eglDpy);
    return 0;
}

