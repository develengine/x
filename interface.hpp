#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <glad/gl.h>
#include <GL/glx.h>

#include <stdint.h>


#ifdef ENG_LINUX
#define ENG_X
#endif

#define MouseButtonMask uint32_t
#define KeyboardModMask uint32_t

#if defined ENG_WINDOWS
#define Key uint32_t
#elif defined ENG_X
#define Key KeySym
#else
#error "UNKNOWN PLATFORM: Key"
#endif

#if defined ENG_WINDOWS
#define PlatformKey(win, x) ( win )
#elif defined ENG_X
#define PlatformKey(win, x) ( x )
#else
#error "UNKNOWN PLATFORM: PlatformKey"
#endif

extern int EngineMain(int argc, char *argv[]);

namespace engine {


enum MouseButton
{
    BUTTON_LEFT = 1,
    BUTTON_MIDDLE = 2,
    BUTTON_RIGHT = 4,
    BUTTON_SCROLL_UP = 8,
    BUTTON_SCROLL_DOWN = 16
};

enum KeyboardMod
{
    MOD_SHIFT = 1,
    MOD_LOCK = 2,
    MOD_CONTROL = 4,
    MOD_1 = 8,
    MOD_2 = 16,
    MOD_3 = 32,
    MOD_4 = 64,
    MOD_5 = 128
};


struct MouseMotionCallbackInfo
{
    int x, y, relX, relY;
    MouseButtonMask buttons;
    KeyboardModMask mods;
};

struct MouseButtonCallbackInfo
{
    int x, y;
    MouseButton button;
    bool down;
    KeyboardModMask mods;
};

struct KeyboardCallbackInfo
{
    Key key;
    bool down;
    KeyboardModMask mods;
    uint32_t count;
};

struct ScreenCallbackInfo
{
    int width, height;
    bool hidden;
};

struct TextCallbackInfo
{
    char *text;
};


void addMouseMotionCallback(void(*func)(MouseMotionCallbackInfo));
void addMouseButtonCallback(void(*func)(MouseButtonCallbackInfo));
void addKeyboardCallback(void(*func)(KeyboardCallbackInfo));
void addScreenCallback(void(*func)(ScreenCallbackInfo));
void addTextCallback(void(*func)(TextCallbackInfo));

void pollEvents();
void swapBuffers();


}

