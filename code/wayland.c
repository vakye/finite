
#pragma once

// NOTE(vak): Cheatsheet

static b32  WaylandSetup            (void);
static void WaylandShutdown         (void);
static void WaylandToggleFullscreen (void);
static b32  WaylandIsClosed         (void);
static b32  WaylandShouldResize     (void);
static void WaylandNotifyResized    (void);
static u32  WaylandGetWidth         (void);
static u32  WaylandGetHeight        (void);
static void WaylandPollEvents       (void);
static void WaylandPresent          (void);

// NOTE(vak): Implementation

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client.h"
#include "xdg-shell.c"

typedef struct
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

    b32                     IsFullscreen;
    b32                     IsClosed;
    b32                     IsResizing;
    b32                     ReadyToResize;
    u32                     Width, Height;
} wayland_state;

static wayland_state Wayland = {0};

static b32 WaylandConnectDisplay    (void);
static b32 WaylandGetRegistry       (void);
static b32 WaylandCreateSurface     (void);
static b32 WaylandGetXdgSurface     (void);
static b32 WaylandGetXdgTopLevel    (void);
static b32 WaylandNotifyServerDone  (void);

static b32 WaylandSetup(void)
{
    if (!WaylandConnectDisplay())       return (false);
    if (!WaylandGetRegistry())          return (false);
    if (!WaylandCreateSurface())        return (false);
    if (!WaylandGetXdgSurface())        return (false);
    if (!WaylandGetXdgTopLevel())       return (false);
    if (!WaylandNotifyServerDone())     return (false);

    return (true);
}

static void WaylandShutdown(void)
{
    if (Wayland.XdgTopLevel)    xdg_toplevel_destroy(Wayland.XdgTopLevel);
    if (Wayland.XdgSurface)     xdg_surface_destroy(Wayland.XdgSurface);
    if (Wayland.Surface)        wl_surface_destroy(Wayland.Surface);

    if (Wayland.Keyboard)       wl_keyboard_release(Wayland.Keyboard);
    if (Wayland.Pointer)        wl_pointer_release(Wayland.Pointer);

    if (Wayland.Output)         wl_output_release(Wayland.Output);
    if (Wayland.Seat)           wl_seat_destroy(Wayland.Seat);
    if (Wayland.XdgWmBase)      xdg_wm_base_destroy(Wayland.XdgWmBase);
    if (Wayland.Compositor)     wl_compositor_destroy(Wayland.Compositor);

    if (Wayland.Registry)       wl_registry_destroy(Wayland.Registry);
    if (Wayland.Display)        wl_display_disconnect(Wayland.Display);
}

static void WaylandToggleFullscreen(void)
{
    if (!Wayland.IsFullscreen)
        xdg_toplevel_set_fullscreen(Wayland.XdgTopLevel, Wayland.Output);
    else
        xdg_toplevel_unset_fullscreen(Wayland.XdgTopLevel);

    Wayland.IsFullscreen = !Wayland.IsFullscreen;
}

static b32 WaylandIsClosed(void)
{
    return (Wayland.IsClosed);
}

static b32 WaylandShouldResize(void)
{
    return (Wayland.ReadyToResize);
}

static void WaylandNotifyResized(void)
{
    Wayland.ReadyToResize   = false;
    Wayland.IsResizing      = false;
}

static u32 WaylandGetWidth(void)
{
    return (Wayland.Width);
}

static u32 WaylandGetHeight(void)
{
    return (Wayland.Height);
}

static void WaylandPollEvents(void)
{
    wl_display_roundtrip(Wayland.Display);
}

static void WaylandPresent(void)
{
    wl_surface_commit(Wayland.Surface);
}

static void WaylandError(char* Message)
{
    // TODO(vak): Implement this using write()
    //fprintf(stderr, "[wayland]: %s\n", Message);
}

// NOTE(vak): Pointer & Keyboard

static void WaylandPointerEnter(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Serial,
    struct wl_surface*  Surface,
    wl_fixed_t          X,
    wl_fixed_t          Y
)
{
}

static void WaylandPointerLeave(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Serial,
    struct wl_surface*  Surface
)
{
}

static void WaylandPointerMotion(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Time,
    wl_fixed_t          X,
    wl_fixed_t          Y
)
{
}

static void WaylandPointerButton(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Serial,
    u32                 Time,
    u32                 Button,
    u32                 State
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
    u32                 Format,
    s32                 FileDescriptor,
    u32                 Size
)
{
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

    b32 XkbOkay = true;

    Wayland.XkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!Wayland.XkbContext)
    {
        WaylandError("failed to create xkb context");
        return;
    }

    Wayland.XkbKeymap = xkb_keymap_new_from_string(
        Wayland.XkbContext,
        KeymapString,
        XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS
    );

    if (!Wayland.XkbKeymap)
    {
        WaylandError("failed to create xkb keymap");
        return;
    }

    Wayland.XkbState = xkb_state_new(Wayland.XkbKeymap);

    if (!Wayland.XkbState)
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
    u32                 Serial,
    struct wl_surface*  Surface,
    struct wl_array*    Keys
)
{
}

static void WaylandKeyboardLeave(
    void*               Data,
    struct wl_keyboard* Keyboard,
    u32                 Serial,
    struct wl_surface*  Surface
)
{
}

static void WaylandKeyboardKey(
    void*               Data,
    struct wl_keyboard* Keyboard,
    u32                 Serial,
    u32                 Time,
    u32                 EvdevScancode,
    u32                 State
)
{
    u32             XkbScancode    = EvdevScancode + 8;
    xkb_keysym_t    KeySym         = xkb_state_key_get_one_sym(Wayland.XkbState, XkbScancode);
    b32             IsDown         = (State == WL_KEYBOARD_KEY_STATE_PRESSED);

    switch (KeySym)
    {
        case XKB_KEY_1:                     InputReportButton(InputButton_Weapon1,      IsDown);    break;
        case XKB_KEY_2:                     InputReportButton(InputButton_Weapon2,      IsDown);    break;
        case XKB_KEY_3:                     InputReportButton(InputButton_Weapon3,      IsDown);    break;

        case XKB_KEY_q:                     InputReportButton(InputButton_PrevWeapon,   IsDown);    break;
        case XKB_KEY_e:                     InputReportButton(InputButton_NextWeapon,   IsDown);    break;

        case XKB_KEY_w: case XKB_KEY_Up:    InputReportButton(InputButton_MoveUp,       IsDown);    break;
        case XKB_KEY_a: case XKB_KEY_Left:  InputReportButton(InputButton_MoveLeft,     IsDown);    break;
        case XKB_KEY_s: case XKB_KEY_Down:  InputReportButton(InputButton_MoveDown,     IsDown);    break;
        case XKB_KEY_d: case XKB_KEY_Right: InputReportButton(InputButton_MoveRight,    IsDown);    break;
        case XKB_KEY_space:                 InputReportButton(InputButton_Shoot,        IsDown);    break;

        case XKB_KEY_F11:
        {
            if (IsDown)
            {
                WaylandToggleFullscreen();
            }
        } break;
    }
}

static void WaylandKeyboardModifiers(
    void*               Data,
    struct wl_keyboard* Keyboard,
    u32                 Serial,
    u32                 ModifiersDepressed,
    u32                 ModifiersLatched,
    u32                 ModifiersLocked,
    u32                 Group
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
    u32                 Serial
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
    u32                 Capabilities
)
{
    if (Capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        Wayland.Pointer = wl_seat_get_pointer(Seat);

        if (Wayland.Pointer)
            wl_pointer_add_listener(Wayland.Pointer, &WaylandPointerListener, 0);
        else
            WaylandError("failed to get wl_pointer from wl_seat");
    }

    if (Capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        Wayland.Keyboard = wl_seat_get_keyboard(Seat);

        if (Wayland.Keyboard)
            wl_keyboard_add_listener(Wayland.Keyboard, &WaylandKeyboardListener, 0);
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
    u32                     Name, 
    const char*             InterfaceInit, 
    u32                     Version 
)
{
    string Interface = CString(InterfaceInit);

    if (StringEqual(Interface, CString(wl_compositor_interface.name)))
    {
        Wayland.Compositor = wl_registry_bind(Registry, Name, &wl_compositor_interface, 1);
    }
    else if (StringEqual(Interface, CString(xdg_wm_base_interface.name)))
    {
        Wayland.XdgWmBase = wl_registry_bind(Registry, Name, &xdg_wm_base_interface, 1);

        if (Wayland.XdgWmBase)
            xdg_wm_base_add_listener(Wayland.XdgWmBase, &WaylandXdgWmBaseListener, 0);
    }
    else if (StringEqual(Interface, CString(wl_seat_interface.name)))
    {
        Wayland.Seat = wl_registry_bind(Registry, Name, &wl_seat_interface, 1);

        if (Wayland.Seat)
            wl_seat_add_listener(Wayland.Seat, &WaylandSeatListener, 0);
    }
    else if (StringEqual(Interface, CString(wl_output_interface.name)))
    {
        Wayland.Output = wl_registry_bind(Registry, Name, &wl_output_interface, 1);
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
    u32                 Serial
)
{
    xdg_surface_ack_configure(XdgSurface, Serial);

    Wayland.ReadyToResize = Wayland.IsResizing;
}

static struct xdg_surface_listener WaylandXdgSurfaceListener =
{
    .configure = WaylandXdgSurfaceConfigure,
};

// NOTE(vak): XDG top level listener

static void WaylandXdgTopLevelConfigure(
    void*                   Data,
    struct xdg_toplevel*    XdgTopLevel,
    s32                     Width,
    s32                     Height,
    struct wl_array*        States
)
{
    u32 ClampedWidth  = (u32)Maximum(0, Width);
    u32 ClampedHeight = (u32)Maximum(0, Height);

    if ((ClampedWidth != Wayland.Width) && (ClampedHeight != Wayland.Height))
    {
        Wayland.Width      = ClampedWidth;
        Wayland.Height     = ClampedHeight;
        Wayland.IsResizing = true;
    }
}

static void WaylandXdgTopLevelClose(
    void*                   Data,
    struct xdg_toplevel*    XdgTopLevel
)
{
    Wayland.IsClosed = true;
}

static struct xdg_toplevel_listener WaylandXdgTopLevelListener =
{
    .configure  = WaylandXdgTopLevelConfigure,
    .close      = WaylandXdgTopLevelClose,
};

// NOTE(vak): Main code

static b32 WaylandConnectDisplay(void)
{
    Wayland.Display = wl_display_connect(0);
    if (!Wayland.Display)
    {
        WaylandError("failed to connect to display");
        return (false);
    }

    return (true);
}

static b32 WaylandGetRegistry(void)
{
    Wayland.Registry = wl_display_get_registry(Wayland.Display);
    if (!Wayland.Registry)
    {
        WaylandError("failed to get wl_registry");
        return (false);
    }

    wl_registry_add_listener(Wayland.Registry, &WaylandRegistryListener, 0);
    wl_display_roundtrip(Wayland.Display);

    b32 NotOkay =
        (Wayland.Compositor    == 0) ||
        (Wayland.XdgWmBase     == 0) ||
        (Wayland.Seat          == 0) ||
        (Wayland.Output        == 0);

    if (!Wayland.Compositor)   WaylandError("failed to register wl_compositor");
    if (!Wayland.XdgWmBase)    WaylandError("failed to register xdg_wm_base");
    if (!Wayland.Seat)         WaylandError("failed to register wl_seat");
    if (!Wayland.Output)       WaylandError("failed to register wl_output");

    b32 Okay = !NotOkay;

    return (Okay);
}

static b32 WaylandCreateSurface(void)
{
    Wayland.Surface = wl_compositor_create_surface(Wayland.Compositor);
    if (!Wayland.Surface)
    {
        WaylandError("failed to create wl_surface");
        return (false);
    }

    return (true);
}

static b32 WaylandGetXdgSurface(void)
{
    Wayland.XdgSurface = xdg_wm_base_get_xdg_surface(Wayland.XdgWmBase, Wayland.Surface);
    if (!Wayland.XdgSurface)
    {
        WaylandError("failed to get xdg_surface from xdg_wm_base");
        return (false);
    }

    xdg_surface_add_listener(Wayland.XdgSurface, &WaylandXdgSurfaceListener, 0);

    return (true);
}

static b32 WaylandGetXdgTopLevel(void)
{
    Wayland.XdgTopLevel = xdg_surface_get_toplevel(Wayland.XdgSurface);
    if (!Wayland.XdgTopLevel)
    {
        WaylandError("failed to get xdg_toplevel from xdg_surface");
        return (false);
    }

    xdg_toplevel_add_listener(Wayland.XdgTopLevel, &WaylandXdgTopLevelListener, 0);
    xdg_toplevel_set_title(Wayland.XdgTopLevel, "finite");
    xdg_toplevel_set_app_id(Wayland.XdgTopLevel, "finite");

    return (true);
}

static b32 WaylandNotifyServerDone(void)
{
    wl_surface_commit(Wayland.Surface);
    wl_display_roundtrip(Wayland.Display);
    wl_surface_commit(Wayland.Surface);

    return (true);
}

