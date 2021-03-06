#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <glad/gl.h>
// #include <GL/gl.h>
#include <GL/glx.h>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "interface.hpp"
#include "math.hpp"


#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

static bool ctxErrorOccurred = false;
static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    ctxErrorOccurred = true;
    return 0;
}

static int windowWidth = 1080;
static int windowHeight = 720;

static bool globalRunning = true;
static bool centeredPointer = false;

float cubeVertices[]
{
     1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
     1.0f,-1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,  0.0f, 0.0f, 1.0f,

    -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
    -1.0f,-1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
    -1.0f, 1.0f,-1.0f, -1.0f, 0.0f, 0.0f,
    -1.0f,-1.0f,-1.0f, -1.0f, 0.0f, 0.0f,

    -1.0f, 1.0f,-1.0f,  0.0f, 0.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,  0.0f, 0.0f,-1.0f,
     1.0f, 1.0f,-1.0f,  0.0f, 0.0f,-1.0f,
     1.0f,-1.0f,-1.0f,  0.0f, 0.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  1.0f, 0.0f, 0.0f,
     1.0f,-1.0f,-1.0f,  1.0f, 0.0f, 0.0f,
     1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
     1.0f,-1.0f, 1.0f,  1.0f, 0.0f, 0.0f,

     1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
     1.0f, 1.0f,-1.0f,  0.0f, 1.0f, 0.0f,
    -1.0f, 1.0f,-1.0f,  0.0f, 1.0f, 0.0f,

     1.0f,-1.0f,-1.0f,  0.0f,-1.0f, 0.0f,
    -1.0f,-1.0f,-1.0f,  0.0f,-1.0f, 0.0f,
     1.0f,-1.0f, 1.0f,  0.0f,-1.0f, 0.0f,
    -1.0f,-1.0f, 1.0f,  0.0f,-1.0f, 0.0f
};

uint32_t cubeIndices[]
{
    0, 2, 3,  0, 3, 1,
    4, 6, 7,  4, 7, 5,
    8, 10, 11,  8, 11, 9,
    12, 14, 15,  12, 15, 13,
    16, 18, 19,  16, 19, 17,
    20, 22, 23,  20, 23, 21
};


static bool isExtensionSupported(const char *extList, const char *extension)
{
    const char *start;
    const char *where, *terminator;
  
    where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;

    for (start=extList;;) {
        where = strstr(start, extension);

        if (!where)
            break;

        terminator = where + strlen(extension);

        if ( where == start || *(where - 1) == ' ' )
            if ( *terminator == ' ' || *terminator == '\0' )
                return true;

        start = terminator;
    }

    return false;
}

void GLAPIENTRY openglCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
) {
    bool error = type == GL_DEBUG_TYPE_ERROR;
    std::cerr << "GL: error: " << error << " type: " << type << " severity: " << severity
              << ".\nmessage: " << message << '\n';
}

char *readFileToString(const char *path)
{
    FILE *file = fopen(path, "r");
    
    if (!file)
    {
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
    
    char *data = new char[size];
    fread(data, size, 1, file);
    fclose(file);

    data[size - 1] = '\0';

    return data;
}

int main(int argc, char **argv)
{
    Display *display = XOpenDisplay(NULL);

    if (!display)
    {
        printf("Failed to open display!");
    }

    int visualAttribs[] =
    {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        GLX_RED_SIZE        , 8,
        GLX_GREEN_SIZE      , 8,
        GLX_BLUE_SIZE       , 8,
        GLX_ALPHA_SIZE      , 8,
        GLX_DEPTH_SIZE      , 24,
        GLX_STENCIL_SIZE    , 8,
        GLX_DOUBLEBUFFER    , True,
        //GLX_SAMPLE_BUFFERS  , 1,
        //GLX_SAMPLES         , 4,
        None
    }; 

    int glxMajor, glxMinor;
    if (
        (!glXQueryVersion(display, &glxMajor, &glxMinor))
    ||  (glxMajor == 1 && glxMinor < 3)
    ||  (glxMajor < 1)
    ) {
        printf("Invalid GLX version!");
    }

    int fbCount;
    GLXFBConfig *fbConfigs = glXChooseFBConfig(display, DefaultScreen(display), visualAttribs, &fbCount);
    if (!fbConfigs)
    {
        printf("Failed to retrieve a frame buffer config!");
    }
    printf("Found %d matching configs.\n", fbCount);

    int bestFBConfig = -1, bestSampleCount = -1;
    for (int i = 0; i < fbCount; i++)
    {
        XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, fbConfigs[i]);

        if (visualInfo)
        {
            int sampBuf, samples;
            glXGetFBConfigAttrib(display, fbConfigs[i], GLX_SAMPLE_BUFFERS, &sampBuf);
            glXGetFBConfigAttrib(display, fbConfigs[i], GLX_SAMPLES, &samples);

            printf(
                "Matching fbconfig %d, visual ID 0x%2x: SAMPLE_BUFFERS = %d, SAMPLES = %d\n", 
                i, (uint32_t)(visualInfo->visualid), sampBuf, samples
            );

            if (bestFBConfig < 0 || (sampBuf && samples > bestSampleCount))
            {
                bestFBConfig = i;
                bestSampleCount = samples;
            }
        }

        XFree(visualInfo);
    }

    GLXFBConfig fbConfig = fbConfigs[bestFBConfig];

    XFree(fbConfigs);

    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, fbConfig);
    printf("Chosen visual ID = 0x%x\n", (uint32_t)(visualInfo->visualid)); 

    XSetWindowAttributes setWindowAttrs;
    Colormap colormap;
    setWindowAttrs.colormap = colormap = XCreateColormap(display, RootWindow(display, visualInfo->screen), visualInfo->visual, AllocNone);
    setWindowAttrs.background_pixmap = None;
    setWindowAttrs.border_pixel = 0;
    setWindowAttrs.event_mask = StructureNotifyMask;

    Window window = XCreateWindow(
        display,
        RootWindow(display, visualInfo->screen),
        0, 0, windowWidth, windowHeight, 0, visualInfo->depth,
        InputOutput, 
        visualInfo->visual,
        CWBorderPixel | CWColormap | CWEventMask,
        &setWindowAttrs
    );

    if (!window)
    {
        printf("Failed to create window!\n");
    }

    XFree(visualInfo);

    XSelectInput(display, window, KeyPressMask    | KeyReleaseMask
                                | ButtonPressMask | ButtonReleaseMask
                                | PointerMotionMask
                                | KeymapStateMask
                                | ExposureMask);

    XStoreName(display, window, "OpenGL Context");

    Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

    XMapWindow(display, window);

    int minKeyCodes, maxKeyCodes;
    XDisplayKeycodes(display, &minKeyCodes, &maxKeyCodes);
    int keySymLength;
    KeySym *keySyms = XGetKeyboardMapping(display, minKeyCodes, maxKeyCodes - minKeyCodes, &keySymLength);


    const char *glxExtensions = glXQueryExtensionsString(display, DefaultScreen(display));

    GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (GLXCREATECONTEXTATTRIBSARBPROC)
        glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

    GLXContext context = 0;

    ctxErrorOccurred = false;
    int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

    if (
        (!isExtensionSupported(glxExtensions, "GLX_ARB_create_context"))
    ||  (!glXCreateContextAttribsARB)
    ) {
        printf("\'glXCreateContextARB()\' wasn't found! Using old OpenGL context.\n");
    }

    int contextAttribs[]
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 6,
        //GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        None
    }; 

    context = glXCreateContextAttribsARB(display, fbConfig, 0, True, contextAttribs);

    XSync(display, False);

    if (ctxErrorOccurred || !context)
    {
        printf("Failed to create OpenGL context!\n");
    }

    XSetErrorHandler(oldHandler);

    printf((glXIsDirect(display, context) ? "Direct context.\n" : "Indirect context.\n"));

    glXMakeCurrent(display, window, context);

    if (!gladLoaderLoadGL())
    {
        printf("glad failed to load OpenGL functions!\n");
    }



    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0); 

    const char *vendorString = (const char *)glGetString(GL_VENDOR);
    const char *rendererString = (const char *)glGetString(GL_RENDERER);
    const char *versionString = (const char *)glGetString(GL_VERSION);
    const char *shadingLanguageVersionString = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("Vendor: %s\nRenderer: %s\nVersion: %s\nShading Language version: %s\n",
        vendorString, rendererString, versionString, shadingLanguageVersionString);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
//     glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    uint32_t vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    uint32_t vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    uint32_t ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    int succes;
    char infoLog[512];

    const char *vertexSource = readFileToString("shaders/cringe.vert");
    const char *fragmentSource = readFileToString("shaders/cringe.frag");

    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &succes);

    if (!succes)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Failed to compile vertex shader! Error:\n" << infoLog << '\n';
    }
    
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &succes);

    if (!succes)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Failed to compile vertex shader! Error:\n" << infoLog << '\n';
    }
    
    uint32_t program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &succes);

    if (!succes)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Failed to link shader program! Error:\n" << infoLog << '\n';
    }

    delete[] vertexSource;
    delete[] fragmentSource;

    int uVpMatrix = glGetUniformLocation(program, "u_vpMat");
    int uModelMatrix = glGetUniformLocation(program, "u_modMat");
    int uCameraPosition = glGetUniformLocation(program, "u_cameraPos");
    int uObjectColor = glGetUniformLocation(program, "u_objColor");


    eng::Vec3f cameraPosition(0.0f, 0.0f, 0.0f);
    eng::Vec2f cameraRotation(0.0f, 0.0f);

    eng::Vec3f objectPosition(0.0f, 0.0f, -10.0f);
    eng::Vec3f objectAxis = eng::Vec3f(1.0f, 1.0f, 0.0f).normalize();

    float timePassed = 0.0f;

    int lastPointerX = windowWidth / 2, lastPointerY = windowHeight / 2;
    bool resetPointer = false;

    while (globalRunning)
    {
        int eventCount = QLength(display);
        XEvent event;
        for (int i = 0; i < eventCount; i++)
        {
            XNextEvent(display, &event);

            if (event.type == Expose)
            {
                XWindowAttributes attributes;
                XGetWindowAttributes(display, window, &attributes);
                glViewport(0, 0, attributes.width, attributes.height);
                windowWidth = attributes.width;
                windowHeight = attributes.height;
            }
            else if (event.type == MappingNotify)
            {
                XMappingEvent *mappingEvent = (XMappingEvent*)(&event);
                XRefreshKeyboardMapping(mappingEvent);
                XFree(keySyms);
                XDisplayKeycodes(display, &minKeyCodes, &maxKeyCodes);
                keySyms = XGetKeyboardMapping(display, minKeyCodes, maxKeyCodes - minKeyCodes, &keySymLength);
                printf("Mapping changed\n");
            }
            else if (event.type == KeyPress)
            {
//                 char buffer[128] = { 0 };
                KeySym keySym;
//                 XLookupString(&event.xkey, buffer, sizeof(buffer), &keySym, NULL);
                XKeyPressedEvent *keyPressedEvent = (XKeyPressedEvent*)(&event);
                keySym = keySyms[(keyPressedEvent->keycode - minKeyCodes) * keySymLength];

                if (keySym == XK_Escape)
                {
                    globalRunning = false;
                }
                else if (keySym == XK_Alt_L)
                {
                    centeredPointer = !centeredPointer;

                    if (centeredPointer)
                    {
                        XWarpPointer(display, None, window, 0, 0, 0, 0, windowWidth / 2, windowHeight / 2);
                        lastPointerX = windowWidth / 2;
                        lastPointerY = windowHeight / 2;

                        Cursor invisibleCursor;
                        Pixmap bitmapNoData;
                        XColor black;
                        static char noData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
                        black.red = black.green = black.blue = 0;

                        bitmapNoData = XCreateBitmapFromData(display, window, noData, 8, 8);
                        invisibleCursor = XCreatePixmapCursor(display, bitmapNoData, bitmapNoData, &black, &black, 0, 0);
                        XDefineCursor(display,window, invisibleCursor);
                        XFreeCursor(display, invisibleCursor);
                        XFreePixmap(display, bitmapNoData);
                    }
                    else
                    {
                        XUndefineCursor(display, window);
                    }
                }
                else {

                }

                printf("%d, state: %d, string: %s\n", keyPressedEvent->keycode, 1, XKeysymToString(keySym));
            }
            else if (event.type == KeyRelease)
            {
                XKeyReleasedEvent *keyReleasedEvent = (XKeyReleasedEvent*)(&event);
                printf("%d, state: %d\n", keyReleasedEvent->keycode, 0);
            }
            else if (event.type == ButtonPress)
            {
                XButtonPressedEvent *buttonPressedEvent = (XButtonPressedEvent*)(&event);
                printf("%d, state: %d\n", buttonPressedEvent->button, 1);
            }
            else if (event.type == ButtonRelease)
            {
                XButtonReleasedEvent *buttonReleasedEvent = (XButtonReleasedEvent*)(&event);
                printf("%d, state: %d\n", buttonReleasedEvent->button, 0);
            }
            else if (event.type == MotionNotify)
            {
                XMotionEvent *motionEvent = (XMotionEvent*)(&event);

                if (resetPointer && motionEvent->x == windowWidth / 2 && motionEvent->y == windowHeight / 2)
                {
                    resetPointer = false;
                    lastPointerX = motionEvent->x;
                    lastPointerY = motionEvent->y;
                    break;
                }

                if (centeredPointer)
                {
                    cameraRotation[0] += (float)(motionEvent->y - lastPointerY) / 256.f;
                    cameraRotation[1] += (float)(motionEvent->x - lastPointerX) / 256.f;
                }

                lastPointerX = motionEvent->x;
                lastPointerY = motionEvent->y;

            }
            else if (event.type == ClientMessage
            &&  (unsigned int)(event.xclient.data.l[0]) == WM_DELETE_WINDOW)
            {
                globalRunning = false;
            }
        }

        if (centeredPointer)
        {
            if (abs(lastPointerX - (windowWidth / 2)) > (windowWidth / 4)
            ||  abs(lastPointerY - (windowHeight / 2)) > (windowHeight / 4))
            {
                XWarpPointer(display, None, window, 0, 0, 0, 0, windowWidth / 2, windowHeight / 2);
                resetPointer = true;
//                 XSync(display, False);
            }
        }

        XFlush(display);


        eng::Quaternionf objectRotation(cos(timePassed), (objectAxis * sin(timePassed)).data);
        timePassed += 0.009f;
        eng::Mat4f modelMatrix = eng::Mat4f::translation(objectPosition.data)
                               * eng::Mat4f::rotation(objectRotation.normalize());
        eng::Mat4f viewMatrix = eng::Mat4f::xRotation(cameraRotation[0])
                              * eng::Mat4f::yRotation(cameraRotation[1])
                              * eng::Mat4f::translation((-cameraPosition).data);
        eng::Mat4f projectionMatrix = eng::Mat4f::GL_Projection(90.f, windowWidth, windowHeight, 0.1f, 100.f);
        eng::Mat4f vpMatrix = projectionMatrix * viewMatrix;

        glClearColor(5.0f, 7.0f, 9.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(vao);
        glUseProgram(program);

        glUniformMatrix4fv(uVpMatrix, 1, false, vpMatrix.data);
        glUniformMatrix4fv(uModelMatrix, 1, false, modelMatrix.data);
        glUniform3fv(uCameraPosition, 1, cameraPosition.data);
        glUniform3f(uObjectColor, 0.8f, 0.7f, 0.45f);

        glDrawElements(GL_TRIANGLES, sizeof(cubeIndices) / sizeof(uint32_t), GL_UNSIGNED_INT, (void*)(0));

        glBindVertexArray(0);
        glUseProgram(0);


        glXSwapBuffers(display, window); 
    }

    glXMakeCurrent(display, 0, 0);
    glXDestroyContext(display, context);
    XDestroyWindow(display, window);
    XFreeColormap(display, colormap);
    XCloseDisplay(display);

    return 0;
}
