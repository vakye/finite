
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <linux/input-event-codes.h>
#include <wayland-client.h>
#include "xdg-shell-client.h"
#include "xdg-shell.c"

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include "volk.h"
#include "volk.c"

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

    int                     IsResizing;
    int                     ReadyToResize;
    int                     HasClosed;
    int                     Width, Height;

    unsigned int            LastMotionTime;
    double                  CursorX, CursorY;

    unsigned int            LastButtonTime[3];
    unsigned int            ButtonsPressed[3];          // NOTE(vak): Mouse buttons = {Left, Right, Middle}
} wayland_state;

static wayland_state Wayland = {0};

static void HandleXdgWmBasePing(void* Data, struct xdg_wm_base* XdgWmBase, unsigned int Serial)
{
    xdg_wm_base_pong(XdgWmBase, Serial);
}

static struct xdg_wm_base_listener XdgWmBaseListener =
{
    .ping = HandleXdgWmBasePing,
};

static void HandlePointerEnter(
    void* Data,
    struct wl_pointer* Pointer,
    unsigned int Serial,
    struct wl_surface* Surface,
    wl_fixed_t X,
    wl_fixed_t Y
)
{
}

static void HandlePointerLeave(
    void* Data,
    struct wl_pointer* Pointer,
    unsigned int Serial,
    struct wl_surface* Surface
)
{
}

static void HandlePointerMotion(
    void* Data,
    struct wl_pointer* Pointer,
    unsigned int Time,
    wl_fixed_t X,
    wl_fixed_t Y
)
{
    if (Time > Wayland.LastMotionTime)
    {
        Wayland.CursorX = wl_fixed_to_double(X);
        Wayland.CursorY = wl_fixed_to_double(Y);
        Wayland.LastMotionTime = Time;
    }
}

static void HandlePointerButton(
    void* Data,
    struct wl_pointer* Pointer,
    unsigned int Serial,
    unsigned int Time,
    unsigned int Button,
    unsigned int State
)
{
    unsigned int Pressed = (State == WL_POINTER_BUTTON_STATE_PRESSED);
    unsigned int Index = 0;

    if (Button == BTN_LEFT) Index = 0;
    else if (Button == BTN_RIGHT) Index = 1;
    else if (Button == BTN_MIDDLE) Index = 2;
    else Index = 0xFFFFFFFF;

    if (Index != 0xFFFFFFFF)
    {
        if (Time > Wayland.LastButtonTime[Index])
        {
            Wayland.LastButtonTime[Index] = Time;
            Wayland.ButtonsPressed[Index] = Pressed;
        }
    }
}

static struct wl_pointer_listener PointerListener =
{
    .enter = HandlePointerEnter,
    .leave = HandlePointerLeave,
    .motion = HandlePointerMotion,
    .button = HandlePointerButton,
};

static void HandleKeyboardKeymap(
    void* Data,
    struct wl_keyboard* Keyboard,
    unsigned int Format,
    int FileDescriptor,
    unsigned int Size
)
{
    assert(Format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

    // TODO(vak): Setup keymap
}

static void HandleKeyboardEnter(
    void* Data,
    struct wl_keyboard* Keyboard,
    unsigned int Serial,
    struct wl_surface* Surface,
    struct wl_array* Keys
)
{
}

static void HandleKeyboardLeave(
    void* Data,
    struct wl_keyboard* Keyboard,
    unsigned int Serial,
    struct wl_surface* Surface
)
{
}

static void HandleKeyboardKey(
    void* Data,
    struct wl_keyboard* Keyboard,
    unsigned int Serial,
    unsigned int Time,
    unsigned int Key,
    unsigned int State
)
{
    if (State == WL_KEYBOARD_KEY_STATE_PRESSED)
        printf("pressed: ");
    else if (State == WL_KEYBOARD_KEY_STATE_RELEASED)
        printf("released: ");

    printf("key = %u\n", Key);

    // TODO(vak): Map key code to key with keymap
}

static void HandleKeyboardModifiers(
    void* Data,
    struct wl_keyboard* Keyboard,
    unsigned int Serial,
    unsigned int ModifiersDepressed,
    unsigned int ModifiersLatched,
    unsigned int ModifiersLocked,
    unsigned int Group
)
{
    // TODO(vak): Handle modifiers
}

static struct wl_keyboard_listener KeyboardListener =
{
    .keymap = HandleKeyboardKeymap,
    .enter = HandleKeyboardEnter,
    .leave = HandleKeyboardLeave,
    .key = HandleKeyboardKey,
    .modifiers = HandleKeyboardModifiers,
};

static void HandleSeatCapabilities(void* Data, struct wl_seat* Seat, unsigned int Capabilities)
{
    printf("seat capabilities: ");

    if (Capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        Wayland.Pointer = wl_seat_get_pointer(Seat);
        assert(Wayland.Pointer);

        wl_pointer_add_listener(Wayland.Pointer, &PointerListener, 0);

        printf("pointer ");
    }

    if (Capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        Wayland.Keyboard = wl_seat_get_keyboard(Seat);
        assert(Wayland.Keyboard);

        wl_keyboard_add_listener(Wayland.Keyboard, &KeyboardListener, 0);

        printf("keyboard ");
    }

    if (Capabilities & WL_SEAT_CAPABILITY_TOUCH)
        printf("touch ");

    printf("\n");
}

static struct wl_seat_listener SeatListener =
{
    .capabilities = HandleSeatCapabilities,
};

static void HandleRegistryGlobal(
    void*                   Data,
    struct wl_registry*     Registry,
    unsigned int            Name,
    const char*             Interface,
    unsigned int            Version
)
{
    if (strcmp(Interface, wl_compositor_interface.name) == 0)
    {
        Wayland.Compositor = wl_registry_bind(Registry, Name, &wl_compositor_interface, 1);
        assert(Wayland.Compositor);
    }
    else if (strcmp(Interface, xdg_wm_base_interface.name) == 0)
    {
        Wayland.XdgWmBase = wl_registry_bind(Registry, Name, &xdg_wm_base_interface, 1);
        assert(Wayland.XdgWmBase);

        xdg_wm_base_add_listener(Wayland.XdgWmBase, &XdgWmBaseListener, 0);
    }
    else if (strcmp(Interface, wl_seat_interface.name) == 0)
    {
        Wayland.Seat = wl_registry_bind(Registry, Name, &wl_seat_interface, 1);
        assert(Wayland.Seat);

        wl_seat_add_listener(Wayland.Seat, &SeatListener, 0);
    }
}

static struct wl_registry_listener RegistryListener =
{
    .global = HandleRegistryGlobal,
};

static void HandleXdgSurfaceConfigure(void* Data, struct xdg_surface* XdgSurface, unsigned int Serial)
{
    xdg_surface_ack_configure(XdgSurface, Serial);

    if (Wayland.IsResizing)
    {
        Wayland.ReadyToResize = 1;
    }
}

static struct xdg_surface_listener XdgSurfaceListener =
{
    .configure = HandleXdgSurfaceConfigure,
};

static void HandleXdgTopLevelConfigure(
    void* Data,
    struct xdg_toplevel* TopLevel,
    int Width,
    int Height,
    struct wl_array* States
)
{
    if ((Width != Wayland.Width) || (Height != Wayland.Height))
    {
        Wayland.Width = Width;
        Wayland.Height = Height;
        Wayland.IsResizing = 1;
    }
}

static void HandleXdgTopLevelClose(
    void* Data,
    struct xdg_toplevel* TopLevel
)
{
    Wayland.HasClosed = 1;
}

static struct xdg_toplevel_listener XdgTopLevelListener =
{
    .configure  = HandleXdgTopLevelConfigure,
    .close      = HandleXdgTopLevelClose,
};

#define VK_CHECK(VulkanCall) \
    do \
    { \
        VkResult __VulkanResult__ = (VulkanCall); \
        if (__VulkanResult__ != VK_SUCCESS) \
        { \
            fprintf(stderr, "Vulkan call '" #VulkanCall "' failed with error code %i\n", __VulkanResult__); \
            assert(0); \
        } \
    } \
    while (0)

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef struct
{
    VkSurfaceFormatKHR Format;
    VkPresentModeKHR PresentMode;

    VkSwapchainKHR Swapchain;
    unsigned int Width, Height;
    unsigned int ImageCount;
    VkImage Images[16];
    VkImageView ImageViews[16];
} vulkan_swapchain;

static void VulkanDestroySwapchain(
    VkDevice            Device,
    vulkan_swapchain*   Swapchain
)
{
    if (Swapchain->Swapchain)
    {
        for (unsigned int Index = 0; Index < Swapchain->ImageCount; Index++)
            vkDestroyImageView(Device, Swapchain->ImageViews[Index], 0);

        vkDestroySwapchainKHR(Device, Swapchain->Swapchain, 0);
    }
}

static void VulkanResizeSwapchain(
    VkDevice            Device,
    VkPhysicalDevice    PhysicalDevice,
    VkSurfaceKHR        Surface,
    vulkan_swapchain*   Swapchain
)
{
    VK_CHECK(vkDeviceWaitIdle(Device));

    VulkanDestroySwapchain(Device, Swapchain);

    VkSurfaceCapabilitiesKHR SurfaceCaps = {0};

    // TODO(vak): Investigate why the fuck does SurfaceCaps.currentExtent
    // get set to {U32Max, U32Max} ??

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        PhysicalDevice,
        Surface,
        &SurfaceCaps
    ));

    int DesiredImageCount = (SurfaceCaps.minImageCount <= 3) ? (3) : (SurfaceCaps.minImageCount);

    Swapchain->Width = Wayland.Width;
    Swapchain->Height = Wayland.Height;

    VkSwapchainCreateInfoKHR SwapchainInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = Surface,
        .minImageCount = DesiredImageCount,
        .imageFormat = Swapchain->Format.format,
        .imageColorSpace = Swapchain->Format.colorSpace,
        .imageExtent = {Wayland.Width, Wayland.Height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = SurfaceCaps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = Swapchain->PresentMode,
        .clipped = 1,
    };

    VK_CHECK(vkCreateSwapchainKHR(Device, &SwapchainInfo, 0, &Swapchain->Swapchain));

    Swapchain->ImageCount = ARRAY_COUNT(Swapchain->Images);

    VK_CHECK(vkGetSwapchainImagesKHR(Device, Swapchain->Swapchain, &Swapchain->ImageCount, Swapchain->Images));

    for (unsigned int Index = 0; Index < Swapchain->ImageCount; Index++)
    {
        VkImageViewCreateInfo ImageViewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = Swapchain->Images[Index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = Swapchain->Format.format,
            .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        VK_CHECK(vkCreateImageView(Device, &ImageViewInfo, 0, &Swapchain->ImageViews[Index]));
    }

    VK_CHECK(vkDeviceWaitIdle(Device));
}

int main(int ArgCount, char* Args[])
{
    setvbuf(stdout, 0, _IONBF, 0);

    Wayland.Display = wl_display_connect(0);
    assert(Wayland.Display);

    Wayland.Registry = wl_display_get_registry(Wayland.Display);
    assert(Wayland.Display);

    wl_registry_add_listener(Wayland.Registry, &RegistryListener, 0);
    wl_display_roundtrip(Wayland.Display);

    Wayland.Surface = wl_compositor_create_surface(Wayland.Compositor);
    assert(Wayland.Surface);

    Wayland.XdgSurface = xdg_wm_base_get_xdg_surface(Wayland.XdgWmBase, Wayland.Surface);
    assert(Wayland.XdgSurface);

    xdg_surface_add_listener(Wayland.XdgSurface, &XdgSurfaceListener, 0);

    Wayland.XdgTopLevel = xdg_surface_get_toplevel(Wayland.XdgSurface);
    assert(Wayland.XdgTopLevel);

    xdg_toplevel_add_listener(Wayland.XdgTopLevel, &XdgTopLevelListener, 0);

    xdg_toplevel_set_title(Wayland.XdgTopLevel, "finite");
    xdg_toplevel_set_app_id(Wayland.XdgTopLevel, "finite");

    wl_surface_commit(Wayland.Surface);
    wl_display_roundtrip(Wayland.Display);
    wl_surface_commit(Wayland.Surface);

    printf("Wayland setup good\n");

    VK_CHECK(volkInitialize());

    const char* InstanceExtensions[] =
    {
        "VK_KHR_surface",
        "VK_KHR_wayland_surface",
    };

    VkInstanceCreateInfo InstanceInfo =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo)
        {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Finite",
            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
            .pEngineName = "Finite",
            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
            .apiVersion = VK_API_VERSION_1_3,
        },
        .ppEnabledExtensionNames = InstanceExtensions,
        .enabledExtensionCount = ARRAY_COUNT(InstanceExtensions),
    };

    VkInstance Instance = {0};
    VK_CHECK(vkCreateInstance(&InstanceInfo, 0, &Instance));

    volkLoadInstance(Instance);

    VkWaylandSurfaceCreateInfoKHR WaylandSurfaceInfo =
    {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = Wayland.Display,
        .surface = Wayland.Surface,
    };

    VkSurfaceKHR Surface = {0};
    VK_CHECK(vkCreateWaylandSurfaceKHR(Instance, &WaylandSurfaceInfo, 0, &Surface));

    VkPhysicalDevice PhysicalDevices[16] = {0};
    unsigned int PhysicalDeviceCount = ARRAY_COUNT(PhysicalDevices);

    VK_CHECK(vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices));

    VkPhysicalDevice PhysicalDevice = {0};

    for (unsigned int Index = 0; Index < PhysicalDeviceCount; Index++)
    {
        VkPhysicalDeviceProperties Properties = {0};
        vkGetPhysicalDeviceProperties(PhysicalDevices[Index], &Properties);

        if (Properties.apiVersion < VK_API_VERSION_1_3)
            continue;

        if (Properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            continue;

        printf("Using GPU: %s\n", Properties.deviceName);

        PhysicalDevice = PhysicalDevices[Index];
        break;
    }

    assert(PhysicalDevice);

    VkQueueFamilyProperties QueueFamilies[32] = {0};
    unsigned int QueueFamilyCount = ARRAY_COUNT(QueueFamilies);

    vkGetPhysicalDeviceQueueFamilyProperties(
        PhysicalDevice,
        &QueueFamilyCount,
        QueueFamilies
    );

    unsigned int QueueFamilyIndex = 0xFFFFFFFF;

    for (unsigned int Index = 0; Index < QueueFamilyCount; Index++)
    {
        VkQueueFamilyProperties* Properties = QueueFamilies + Index;

        VkQueueFlags RequiredFlags =
            VK_QUEUE_GRAPHICS_BIT |
            VK_QUEUE_TRANSFER_BIT |
            VK_QUEUE_COMPUTE_BIT;

        if ((Properties->queueFlags & RequiredFlags) == RequiredFlags)
        {
            QueueFamilyIndex = Index;
            break;
        }
    }

    assert(QueueFamilyIndex != 0xFFFFFFFF);

    const char* DeviceExtensions[] =
    {
        "VK_KHR_swapchain",
    };

    VkPhysicalDeviceVulkan13Features Vulkan13Features =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = 1,
    };

    VkDeviceCreateInfo DeviceInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &Vulkan13Features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &(VkDeviceQueueCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = QueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = (float[]){1.0f},
        },
        .ppEnabledExtensionNames = DeviceExtensions,
        .enabledExtensionCount = ARRAY_COUNT(DeviceExtensions),
    };

    VkDevice Device = {0};
    VK_CHECK(vkCreateDevice(PhysicalDevice, &DeviceInfo, 0, &Device));

    VkQueue Queue = {0};
    vkGetDeviceQueue(Device, QueueFamilyIndex, 0, &Queue);

    vulkan_swapchain Swapchain = {0};
    // NOTE(vak): Created later in main loop

    VkSurfaceFormatKHR SurfaceFormats[64] = {0};
    unsigned int SurfaceFormatCount = ARRAY_COUNT(SurfaceFormats);

    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        PhysicalDevice,
        Surface,
        &SurfaceFormatCount,
        SurfaceFormats
    ));

    for (unsigned int Index = 0; Index < SurfaceFormatCount; Index++)
    {
        VkSurfaceFormatKHR SurfaceFormat = SurfaceFormats[Index];

        if (SurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
        {
            printf("Swapchain format: R8G8B8A8_UNORM\n");
            Swapchain.Format = SurfaceFormat;
            break;
        }

        if (SurfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
        {
            printf("Swapchain format: B8G8R8A8_UNORM\n");
            Swapchain.Format = SurfaceFormat;
            break;
        }
    }

    assert(Swapchain.Format.format != VK_FORMAT_UNDEFINED);

    VkPresentModeKHR PresentModes[16] = {0};
    unsigned int PresentModeCount = ARRAY_COUNT(PresentModes);

    Swapchain.PresentMode = VK_PRESENT_MODE_FIFO_KHR;

    for (unsigned int Index = 0; Index < PresentModeCount; Index++)
    {
        VkPresentModeKHR PresentMode = PresentModes[Index];

        if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            Swapchain.PresentMode = PresentMode;
            break;
        }
    }

    if (Swapchain.PresentMode == VK_PRESENT_MODE_FIFO_KHR)
        printf("Swapchain present mode: FIFO\n");
    else if (Swapchain.PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        printf("Swapchain present mode: Mailbox\n");
    else
        printf("Swapchain present mode: Unknown\n");

    VkCommandPoolCreateInfo CommandPoolInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = QueueFamilyIndex,
    };

    VkCommandPool CommandPool = {0};
    VK_CHECK(vkCreateCommandPool(Device, &CommandPoolInfo, 0, &CommandPool));

    VkCommandBufferAllocateInfo CommandBufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer CommandBuffer = {0};
    VK_CHECK(vkAllocateCommandBuffers(Device, &CommandBufferInfo, &CommandBuffer));

    VkSemaphoreCreateInfo SemaphoreInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore AcquireSemaphore = {0};
    VK_CHECK(vkCreateSemaphore(Device, &SemaphoreInfo, 0, &AcquireSemaphore));

    VkSemaphore PresentSemaphore = {0};
    VK_CHECK(vkCreateSemaphore(Device, &SemaphoreInfo, 0, &PresentSemaphore));

    printf("Vulkan setup good\n");

    unsigned int ImageIndex = 0;

    while (!Wayland.HasClosed)
    {
        wl_display_roundtrip(Wayland.Display);

        if (Wayland.ReadyToResize)
            VulkanResizeSwapchain(Device, PhysicalDevice, Surface, &Swapchain);

        VK_CHECK(vkAcquireNextImageKHR(
            Device,
            Swapchain.Swapchain,
            ~0ull,
            AcquireSemaphore,
            0,
            &ImageIndex
        ));

        VkCommandBufferBeginInfo BeginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        VK_CHECK(vkResetCommandBuffer(CommandBuffer, 0));
        VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

        VkImageMemoryBarrier RenderBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Swapchain.Images[ImageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            CommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, 0, 0, 0,
            1, &RenderBarrier
        );

        VkRenderingInfo RenderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {.offset = {0, 0}, .extent = {Swapchain.Width, Swapchain.Height}},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkRenderingAttachmentInfo)
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = Swapchain.ImageViews[ImageIndex],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {.float32 = {0.07f, 0.08f, 0.1f, 1.0f}}},
            },
        };

        vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
        vkCmdEndRendering(CommandBuffer);

        VkImageMemoryBarrier PresentBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Swapchain.Images[ImageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            CommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, 0, 0, 0,
            1, &PresentBarrier
        );

        VK_CHECK(vkEndCommandBuffer(CommandBuffer));

        VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo SubmitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &AcquireSemaphore,
            .pWaitDstStageMask = &WaitStage,
            .commandBufferCount = 1,
            .pCommandBuffers = &CommandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &PresentSemaphore,
        };

        VK_CHECK(vkQueueSubmit(Queue, 1, &SubmitInfo, 0));

        VkPresentInfoKHR PresentInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &PresentSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &Swapchain.Swapchain,
            .pImageIndices = &ImageIndex,
        };

        VK_CHECK(vkQueuePresentKHR(Queue, &PresentInfo));

        wl_surface_commit(Wayland.Surface);

        Wayland.IsResizing = 0;
        Wayland.ReadyToResize = 0;

        VK_CHECK(vkDeviceWaitIdle(Device));
    }

    vkDestroySemaphore(Device, PresentSemaphore, 0);
    vkDestroySemaphore(Device, AcquireSemaphore, 0);
    vkDestroyCommandPool(Device, CommandPool, 0);
    VulkanDestroySwapchain(Device, &Swapchain);
    vkDestroyDevice(Device, 0);
    vkDestroySurfaceKHR(Instance, Surface, 0);
    vkDestroyInstance(Instance, 0);

    xdg_toplevel_destroy(Wayland.XdgTopLevel);
    xdg_surface_destroy(Wayland.XdgSurface);
    wl_surface_destroy(Wayland.Surface);
    xdg_wm_base_destroy(Wayland.XdgWmBase);
    wl_compositor_destroy(Wayland.Compositor);
    wl_registry_destroy(Wayland.Registry);
    wl_display_disconnect(Wayland.Display);
    return (0);
}

