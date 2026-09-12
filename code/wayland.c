
#pragma once

// NOTE(vak): Cheatsheet

typedef struct wayland_state wayland_state;

static int          WaylandSetup(wayland_state* Wayland);
static void         WaylandShutdown(wayland_state* Wayland);
static int          WaylandIsClosed(wayland_state* Wayland);
static int          WaylandShouldResize(wayland_state* Wayland);
static unsigned int WaylandGetWidth(wayland_state* Wayland);
static unsigned int WaylandGetHeight(wayland_state* Wayland);
static void         WaylandPollEvents(wayland_state* Wayland, platform_input* Input);
static void         WaylandPresent(wayland_state* Wayland);

// NOTE(vak): Implementation

#include <sys/mman.h>
#include <sys/unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client.h"
#include "xdg-shell.c"

struct wayland_state
{
    struct wl_display*      Display;
    struct wl_registry*     Registry;
    struct wl_compositor*   Compositor;
    struct xdg_wm_base*     XdgWmBase;
    struct wl_surface*      Surface;
    struct xdg_surface*     XdgSurface;
    struct xdg_toplevel*    XdgTopLevel;
    struct wl_seat*         Seat;
    struct wl_pointer*      Pointer;
    struct wl_keyboard*     Keyboard;
    struct wl_output*       Output;

    struct xkb_context*     XkbContext;
    struct xkb_keymap*      XkbKeymap;
    struct xkb_state*       XkbState;

    int                     IsFullscreen;
    int                     IsClosed;

    int                     IsResizing;
    int                     ReadyToResize;
    int                     Width, Height;

    platform_input*         Input;
};

static int WaylandConnectDisplay    (wayland_state* Wayland);
static int WaylandGetRegistry       (wayland_state* Wayland);
static int WaylandCreateSurface     (wayland_state* Wayland);
static int WaylandGetXdgSurface     (wayland_state* Wayland);
static int WaylandGetXdgTopLevel    (wayland_state* Wayland);
static int WaylandNotifyServerDone  (wayland_state* Wayland);

static int WaylandSetup(wayland_state* Wayland)
{
    memset(Wayland, 0, sizeof(wayland_state));

    if (!WaylandConnectDisplay(Wayland))            return (0);
    if (!WaylandGetRegistry(Wayland))               return (0);
    if (!WaylandCreateSurface(Wayland))             return (0);
    if (!WaylandGetXdgSurface(Wayland))             return (0);
    if (!WaylandGetXdgTopLevel(Wayland))            return (0);
    if (!WaylandNotifyServerDone(Wayland))          return (0);

    return (1);
}

static void WaylandShutdown(wayland_state* Wayland)
{
    if (Wayland->XdgTopLevel)   xdg_toplevel_destroy(Wayland->XdgTopLevel);
    if (Wayland->XdgSurface)    xdg_surface_destroy(Wayland->XdgSurface);
    if (Wayland->Surface)       wl_surface_destroy(Wayland->Surface);

    if (Wayland->Keyboard)      wl_keyboard_release(Wayland->Keyboard);
    if (Wayland->Pointer)       wl_pointer_release(Wayland->Pointer);

    if (Wayland->Output)        wl_output_release(Wayland->Output);
    if (Wayland->Seat)          wl_seat_destroy(Wayland->Seat);
    if (Wayland->XdgWmBase)     xdg_wm_base_destroy(Wayland->XdgWmBase);
    if (Wayland->Compositor)    wl_compositor_destroy(Wayland->Compositor);

    if (Wayland->Registry)      wl_registry_destroy(Wayland->Registry);
    if (Wayland->Display)       wl_display_disconnect(Wayland->Display);

    memset(Wayland, 0, sizeof(wayland_state));
}

static int WaylandIsClosed(wayland_state* Wayland)
{
    return (Wayland->IsClosed);
}

static int WaylandShouldResize(wayland_state* Wayland)
{
    return (Wayland->ReadyToResize);
}

static unsigned int WaylandGetWidth(wayland_state* Wayland)
{
    return Maximum(0, Wayland->Width);
}

static unsigned int WaylandGetHeight(wayland_state* Wayland)
{
    return Maximum(0, Wayland->Height);
}

static void WaylandPollEvents(wayland_state* Wayland, platform_input* Input)
{
    Wayland->Input = Input;
    wl_display_roundtrip(Wayland->Display);
    Wayland->Input = 0;
}

static void WaylandPresent(wayland_state* Wayland)
{
    wl_surface_commit(Wayland->Surface);
}

static void WaylandError(char* Message)
{
    fprintf(stderr, "[wayland]: %s\n", Message);
}

// NOTE(vak): Pointer & Keyboard

static void WaylandPointerEnter(
    void*               Data,
    struct wl_pointer*  Pointer,
    unsigned int        Serial,
    struct wl_surface*  Surface,
    wl_fixed_t          X,
    wl_fixed_t          Y
)
{
}

static void WaylandPointerLeave(
    void*               Data,
    struct wl_pointer*  Pointer,
    unsigned int        Serial,
    struct wl_surface*  Surface
)
{
}

static void WaylandPointerMotion(
    void*               Data,
    struct wl_pointer*  Pointer,
    unsigned int        Time,
    wl_fixed_t          X,
    wl_fixed_t          Y
)
{
}

static void WaylandPointerButton(
    void*               Data,
    struct wl_pointer*  Pointer,
    unsigned int        Serial,
    unsigned int        Time,
    unsigned int        Button,
    unsigned int        State
)
{
}

static struct wl_pointer_listener WaylandPointerListener =
{
    .enter  = WaylandPointerEnter,
    .leave  = WaylandPointerLeave,
    .motion = WaylandPointerMotion,
    .button = WaylandPointerButton,
};

static void WaylandKeyboardKeymap(
    void*               Data,
    struct wl_keyboard* Keyboard,
    unsigned int        Format,
    int                 FileDescriptor,
    unsigned int        Size
)
{
    wayland_state* Wayland = (wayland_state*)Data;

    if (Format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        WaylandError("unknown keyboard keymap");
        return;
    }

    char* KeymapString = mmap(0, Size, PROT_READ, MAP_PRIVATE, FileDescriptor, 0);
    if (!KeymapString)
    {
        WaylandError("failed to mmap keymap");
        return;
    }

    int XkbOkay = true;

    Wayland->XkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!Wayland->XkbContext)
    {
        WaylandError("failed to create xkb context");
        return;
    }

    Wayland->XkbKeymap = xkb_keymap_new_from_string(
        Wayland->XkbContext,
        KeymapString,
        XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS
    );

    if (!Wayland->XkbKeymap)
    {
        WaylandError("failed to create xkb keymap");
        return;
    }

    Wayland->XkbState = xkb_state_new(Wayland->XkbKeymap);

    if (!Wayland->XkbState)
    {
        WaylandError("failed to create xkb state");
        return;
    }

    munmap(KeymapString, Size);
    close(FileDescriptor);
}

static void WaylandKeyboardEnter(
    void*               Data,
    struct wl_keyboard* Keyboard,
    unsigned int        Serial,
    struct wl_surface*  Surface,
    struct wl_array*    Keys
)
{
}

static void WaylandKeyboardLeave(
    void*               Data,
    struct wl_keyboard* Keyboard,
    unsigned int        Serial,
    struct wl_surface*  Surface
)
{
}

static void WaylandKeyboardKey(
    void*               Data,
    struct wl_keyboard* Keyboard,
    unsigned int        Serial,
    unsigned int        Time,
    unsigned int        EvdevScancode,
    unsigned int        State
)
{
    wayland_state* Wayland = (wayland_state*)Data;

    if (!Wayland->Input)
        return;

    platform_input* Input = Wayland->Input;

    unsigned int XkbScancode    = EvdevScancode + 8;
    xkb_keysym_t KeySym         = xkb_state_key_get_one_sym(Wayland->XkbState, XkbScancode);
    unsigned int IsDown         = (State == WL_KEYBOARD_KEY_STATE_PRESSED);

    input_button Button = U32Max;

    switch (KeySym)
    {
        case XKB_KEY_w: case XKB_KEY_Up:    Button = InputButton_MoveUp;        break;
        case XKB_KEY_a: case XKB_KEY_Left:  Button = InputButton_MoveLeft;      break;
        case XKB_KEY_s: case XKB_KEY_Down:  Button = InputButton_MoveDown;      break;
        case XKB_KEY_d: case XKB_KEY_Right: Button = InputButton_MoveRight;     break;
        case XKB_KEY_space:                 Button = InputButton_Shoot;         break;
    }

    if (Button < InputButton_COUNT)
        Input->ButtonStates[Button].IsDown = IsDown;
}

static void WaylandKeyboardModifiers(
    void*               Data,
    struct wl_keyboard* Keyboard,
    unsigned int        Serial,
    unsigned int        ModifiersDepressed,
    unsigned int        ModifiersLatched,
    unsigned int        ModifiersLocked,
    unsigned int        Group
)
{
}

static struct wl_keyboard_listener WaylandKeyboardListener =
{
    .keymap     = WaylandKeyboardKeymap,
    .enter      = WaylandKeyboardEnter,
    .leave      = WaylandKeyboardLeave,
    .key        = WaylandKeyboardKey,
    .modifiers  = WaylandKeyboardModifiers,
};

// NOTE(vak): XDG Window Manager Base

static void WaylandXdgWmBasePing(
    void*               Data,
    struct xdg_wm_base* XdgWmBase,
    unsigned int        Serial
)
{
    xdg_wm_base_pong(XdgWmBase, Serial);
}

static struct xdg_wm_base_listener WaylandXdgWmBaseListener =
{
    .ping = WaylandXdgWmBasePing,
};

static void WaylandSeatCapabilities(
    void*               Data,
    struct wl_seat*     Seat,
    unsigned int        Capabilities
)
{
    wayland_state* Wayland = (wayland_state*)Data;

    if (Capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        Wayland->Pointer = wl_seat_get_pointer(Seat);

        if (Wayland->Pointer)
            wl_pointer_add_listener(Wayland->Pointer, &WaylandPointerListener, Wayland);
        else
            WaylandError("failed to get wl_pointer from wl_seat");
    }

    if (Capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        Wayland->Keyboard = wl_seat_get_keyboard(Seat);

        if (Wayland->Keyboard)
            wl_keyboard_add_listener(Wayland->Keyboard, &WaylandKeyboardListener, Wayland);
        else
            WaylandError("failed to get wl_keyboard from wl_seat");
    }
}

static struct wl_seat_listener WaylandSeatListener =
{
    .capabilities = WaylandSeatCapabilities,
};

// NOTE(vak): Registry & interface binding

static void WaylandHandleRegistryGlobal( 
    void*                   Data, 
    struct wl_registry*     Registry, 
    unsigned int            Name, 
    const char*             Interface, 
    unsigned int            Version 
)
{
    wayland_state* Wayland = (wayland_state*)Data;

    if (strcmp(Interface, wl_compositor_interface.name) == 0)
    {
        Wayland->Compositor = wl_registry_bind(Registry, Name, &wl_compositor_interface, 1);
    }
    else if (strcmp(Interface, xdg_wm_base_interface.name) == 0)
    {
        Wayland->XdgWmBase = wl_registry_bind(Registry, Name, &xdg_wm_base_interface, 1);

        if (Wayland->XdgWmBase)
            xdg_wm_base_add_listener(Wayland->XdgWmBase, &WaylandXdgWmBaseListener, Wayland);
    }
    else if (strcmp(Interface, wl_seat_interface.name) == 0)
    {
        Wayland->Seat = wl_registry_bind(Registry, Name, &wl_seat_interface, 1);

        if (Wayland->Seat)
            wl_seat_add_listener(Wayland->Seat, &WaylandSeatListener, Wayland);
    }
    else if (strcmp(Interface, wl_output_interface.name) == 0)
    {
        Wayland->Output = wl_registry_bind(Registry, Name, &wl_output_interface, 1);
    }
}

static struct wl_registry_listener WaylandRegistryListener =
{
    .global = WaylandHandleRegistryGlobal,
};

// NOTE(vak): XDG surface listener

static void WaylandXdgSurfaceConfigure(
    void*               Data,
    struct xdg_surface* XdgSurface,
    unsigned int        Serial
)
{
    wayland_state* Wayland = (wayland_state*)Data;

    xdg_surface_ack_configure(XdgSurface, Serial);

    Wayland->ReadyToResize = Wayland->IsResizing;
}

static struct xdg_surface_listener WaylandXdgSurfaceListener =
{
    .configure = WaylandXdgSurfaceConfigure,
};

// NOTE(vak): XDG top level listener

static void WaylandXdgTopLevelConfigure(
    void*                   Data,
    struct xdg_toplevel*    XdgTopLevel,
    int                     Width,
    int                     Height,
    struct wl_array*        States
)
{
    wayland_state* Wayland = (wayland_state*)Data;

    if ((Width != Wayland->Width) && (Height != Wayland->Height))
    {
        Wayland->Width      = Width;
        Wayland->Height     = Height;
        Wayland->IsResizing = true;
    }
}

static void WaylandXdgTopLevelClose(
    void*                   Data,
    struct xdg_toplevel*    XdgTopLevel
)
{
    wayland_state* Wayland = (wayland_state*)Data;
    Wayland->IsClosed = true;
}

static struct xdg_toplevel_listener WaylandXdgTopLevelListener =
{
    .configure  = WaylandXdgTopLevelConfigure,
    .close      = WaylandXdgTopLevelClose,
};

// NOTE(vak): Main code

static int WaylandConnectDisplay(wayland_state* Wayland)
{
    Wayland->Display = wl_display_connect(0);
    if (!Wayland->Display)
    {
        WaylandError("failed to connect to display");
        return (0);
    }

    return (1);
}

static int WaylandGetRegistry(wayland_state* Wayland)
{
    Wayland->Registry = wl_display_get_registry(Wayland->Display);
    if (!Wayland->Registry)
    {
        WaylandError("failed to get wl_registry");
        return (0);
    }

    wl_registry_add_listener(Wayland->Registry, &WaylandRegistryListener, Wayland);
    wl_display_roundtrip(Wayland->Display);

    int NotOkay =
        (Wayland->Compositor    == 0) ||
        (Wayland->XdgWmBase     == 0) ||
        (Wayland->Seat          == 0) ||
        (Wayland->Output        == 0);

    if (!Wayland->Compositor)   WaylandError("failed to register wl_compositor");
    if (!Wayland->XdgWmBase)    WaylandError("failed to register xdg_wm_base");
    if (!Wayland->Seat)         WaylandError("failed to register wl_seat");
    if (!Wayland->Output)       WaylandError("failed to register wl_output");

    int Okay = !NotOkay;

    return (Okay);
}

static int WaylandCreateSurface(wayland_state* Wayland)
{
    Wayland->Surface = wl_compositor_create_surface(Wayland->Compositor);
    if (!Wayland->Surface)
    {
        WaylandError("failed to create wl_surface");
        return (0);
    }

    return (1);
}

static int WaylandGetXdgSurface(wayland_state* Wayland)
{
    Wayland->XdgSurface = xdg_wm_base_get_xdg_surface(Wayland->XdgWmBase, Wayland->Surface);
    if (!Wayland->XdgSurface)
    {
        WaylandError("failed to get xdg_surface from xdg_wm_base");
        return (0);
    }

    xdg_surface_add_listener(Wayland->XdgSurface, &WaylandXdgSurfaceListener, Wayland);

    return (1);
}

static int WaylandGetXdgTopLevel(wayland_state* Wayland)
{
    Wayland->XdgTopLevel = xdg_surface_get_toplevel(Wayland->XdgSurface);
    if (!Wayland->XdgTopLevel)
    {
        WaylandError("failed to get xdg_toplevel from xdg_surface");
        return (0);
    }

    xdg_toplevel_add_listener(Wayland->XdgTopLevel, &WaylandXdgTopLevelListener, Wayland);
    xdg_toplevel_set_title(Wayland->XdgTopLevel, "finite");
    xdg_toplevel_set_app_id(Wayland->XdgTopLevel, "finite");

    return (1);
}

static int WaylandNotifyServerDone(wayland_state* Wayland)
{
    wl_surface_commit(Wayland->Surface);
    wl_display_roundtrip(Wayland->Display);
    wl_surface_commit(Wayland->Surface);

    return (1);
}

