
#pragma once

// NOTE(vak): VK_USE_PLATFORM_* is defined by the user
// before including this file.

// NOTE(vak): Cheatsheet

static b32  VulkanSetup     (void);
static void VulkanShutdown  (void);
static b32  VulkanResize    (u32 Width, u32 Height);
static b32  VulkanRender    (void);

// NOTE(vak): Implementation

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include "volk.h"
#include "volk.c"

typedef struct
{
    VkBuffer        Buffer;
    VkDeviceMemory  Memory;
    usize           Size;
    void*           Mapping;
} vulkan_buffer;

typedef struct
{
    u32                         VersionOfAPI;
    VkInstance                  Instance;
    VkSurfaceKHR                Surface;
    VkPhysicalDevice            PhysicalDevice;
    u32                         QueueFamilyIndex;
    VkDevice                    Device;
    VkQueue                     Queue;
    VkCommandPool               CommandPool;
    VkCommandBuffer             CommandBuffer;
    VkSemaphore                 AcquireSemaphore;
    VkSemaphore                 SubmitSemaphore;

    VkSurfaceFormatKHR          SwapchainFormat;
    VkPresentModeKHR            PresentMode;

    VkDescriptorSetLayout       SetLayout;
    VkPipelineLayout            PipelineLayout;
    VkPipeline                  Pipeline;

    vulkan_buffer               VertexBuffer;

    VkSwapchainKHR              Swapchain; 
    VkExtent2D                  SwapchainExtent;
    u32                         SwapchainImageCount;
    VkImage                     SwapchainImages[16];
    VkImageView                 SwapchainImageViews[16];
} vulkan_state;

typedef struct
{
    v2 Position;
    v2 TexCoord;
    v4 Color;
} vulkan_vertex;

typedef struct
{
    m4x4 Projection;
} vulkan_push_constants;

static vulkan_state Vulkan = {0};

static void VulkanError(char* Message);

static b32 VulkanCreateInstance         (void);
static b32 VulkanCreateSurface          (void);
static b32 VulkanPickPhysicalDevice     (void);
static b32 VulkanSelectQueueFamily      (void);
static b32 VulkanCreateDevice           (void);
static b32 VulkanGetQueue               (void);
static b32 VulkanCreateCommandPool      (void);
static b32 VulkanAllocateCommandBuffer  (void);
static b32 VulkanCreateSemaphores       (void);
static b32 VulkanPickSwapchainFormat    (void);
static b32 VulkanPickPresentMode        (void);
static b32 VulkanCreateSetLayout        (void);
static b32 VulkanCreatePipelineLayout   (void);
static b32 VulkanCreatePipeline         (void);

static b32 VulkanCreateBuffer(
    vulkan_buffer*          Buffer,
    usize                   Size,
    VkBufferUsageFlags      UsageFlags,
    VkMemoryPropertyFlags   MemoryPropertyFlags,
    b32                     Mapped
);

static void VulkanDestroyBuffer(vulkan_buffer* Buffer);

static b32 VulkanSetup(void)
{
    if (!VulkanCreateInstance())            return (false);
    if (!VulkanCreateSurface())             return (false);
    if (!VulkanPickPhysicalDevice())        return (false);
    if (!VulkanSelectQueueFamily())         return (false);
    if (!VulkanCreateDevice())              return (false);
    if (!VulkanGetQueue())                  return (false);
    if (!VulkanCreateCommandPool())         return (false);
    if (!VulkanAllocateCommandBuffer())     return (false);
    if (!VulkanCreateSemaphores())          return (false);
    if (!VulkanPickSwapchainFormat())       return (false);
    if (!VulkanPickPresentMode())           return (false);
    if (!VulkanCreateSetLayout())           return (false);
    if (!VulkanCreatePipelineLayout())      return (false);
    if (!VulkanCreatePipeline())            return (false);

    if (!VulkanCreateBuffer(
        &Vulkan.VertexBuffer,
        2 * 1024 * 1024,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        true
    ))
    {
        return (false);
    }

    // NOTE(vak): Swapchain will be created with VulkanResize()

    return (true);
}

static void VulkanShutdown(void)
{
    if (Vulkan.Swapchain)
    {
        for (usize Index = 0; Index < Vulkan.SwapchainImageCount; Index++)
            vkDestroyImageView(Vulkan.Device, Vulkan.SwapchainImageViews[Index], 0);

        vkDestroySwapchainKHR(Vulkan.Device, Vulkan.Swapchain, 0);
    }

    VulkanDestroyBuffer(&Vulkan.VertexBuffer);

    if (Vulkan.Pipeline)               vkDestroyPipeline(Vulkan.Device, Vulkan.Pipeline, 0);
    if (Vulkan.PipelineLayout)         vkDestroyPipelineLayout(Vulkan.Device, Vulkan.PipelineLayout, 0);
    if (Vulkan.SetLayout)              vkDestroyDescriptorSetLayout(Vulkan.Device, Vulkan.SetLayout, 0);

    if (Vulkan.SubmitSemaphore)        vkDestroySemaphore(Vulkan.Device, Vulkan.SubmitSemaphore, 0);
    if (Vulkan.AcquireSemaphore)       vkDestroySemaphore(Vulkan.Device, Vulkan.AcquireSemaphore, 0);

    if (Vulkan.CommandBuffer)          vkFreeCommandBuffers(Vulkan.Device, Vulkan.CommandPool, 1, &Vulkan.CommandBuffer);
    if (Vulkan.CommandPool)            vkDestroyCommandPool(Vulkan.Device, Vulkan.CommandPool, 0);

    if (Vulkan.Device)                 vkDestroyDevice(Vulkan.Device, 0);
    if (Vulkan.Surface)                vkDestroySurfaceKHR(Vulkan.Instance, Vulkan.Surface, 0);
    if (Vulkan.Instance)               vkDestroyInstance(Vulkan.Instance, 0);
}

static b32 VulkanResize(u32 Width, u32 Height)
{
    if ((Vulkan.SwapchainExtent.width == Width) &&
        (Vulkan.SwapchainExtent.height == Height))
    {
        return (true);
    }

    if (vkDeviceWaitIdle(Vulkan.Device))
    {
        VulkanError("failed to wait until device idle before swapchain resize");
        return (false);
    }

    if (Vulkan.Swapchain)
    {
        for (usize Index = 0; Index < Vulkan.SwapchainImageCount; Index++)
            vkDestroyImageView(Vulkan.Device, Vulkan.SwapchainImageViews[Index], 0);

        vkDestroySwapchainKHR(Vulkan.Device, Vulkan.Swapchain, 0);
    }

    VkSurfaceCapabilitiesKHR SurfaceCaps = {0};

    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        Vulkan.PhysicalDevice,
        Vulkan.Surface,
        &SurfaceCaps
    ))
    {
        VulkanError("failed to get physical device surface capabilities");
        return (false);
    }

    u32 DesiredImageCount = Minimum(SurfaceCaps.minImageCount, 3);

    Vulkan.SwapchainExtent = (VkExtent2D){.width = Width, .height = Height};

    VkSwapchainCreateInfoKHR SwapchainInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = Vulkan.Surface,
        .minImageCount = DesiredImageCount,
        .imageFormat = Vulkan.SwapchainFormat.format,
        .imageColorSpace = Vulkan.SwapchainFormat.colorSpace,
        .imageExtent = Vulkan.SwapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = SurfaceCaps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = Vulkan.PresentMode,
        .clipped = 1,
    };

    if (vkCreateSwapchainKHR(Vulkan.Device, &SwapchainInfo, 0, &Vulkan.Swapchain))
    {
        VulkanError("failed to create swapchain");
        return (false);
    }

    Vulkan.SwapchainImageCount = ArrayCount(Vulkan.SwapchainImages);

    if (vkGetSwapchainImagesKHR(Vulkan.Device, Vulkan.Swapchain, &Vulkan.SwapchainImageCount, Vulkan.SwapchainImages))
    {
        VulkanError("failed to get swapchain images");
        return (false);
    }

    for (usize Index = 0; Index < Vulkan.SwapchainImageCount; Index++)
    {
        VkImageViewCreateInfo ImageViewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = Vulkan.SwapchainImages[Index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = Vulkan.SwapchainFormat.format,
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

        if (vkCreateImageView(Vulkan.Device, &ImageViewInfo, 0, &Vulkan.SwapchainImageViews[Index]))
        {
            VulkanError("failed to create swapchain image view");
            return (false);
        }
    }

    if (vkDeviceWaitIdle(Vulkan.Device))
    {
        VulkanError("failed to wait until device idle after swapchain resize");
        return (false);
    }

    return (true);
}

static b32 VulkanRender(void)
{
    render_batch Batch = RenderGetBatch();

    usize MaxVertexCount = Vulkan.VertexBuffer.Size / sizeof(vulkan_vertex);
    usize VerticesNeeded = Batch.RectCount * 6;

    if (MaxVertexCount < VerticesNeeded)
    {
        VulkanError("vertex buffer isn't large enough for render_batch");
        return (false);
    }

    u32 VertexCount = 0;

    for (u32 RectIndex = 0; RectIndex < Batch.RectCount; RectIndex++)
    {
        render_rect* RenderRect = Batch.Rects + RectIndex;

        v2 Min = RenderRect->Rect.Min;
        v2 Max = RenderRect->Rect.Max;

        vulkan_vertex* V = (vulkan_vertex*)Vulkan.VertexBuffer.Mapping + VertexCount;

        V[0] = (vulkan_vertex){V2(Min.X, Min.Y), V2(0.0f, 0.0f), RenderRect->Color};
        V[1] = (vulkan_vertex){V2(Max.X, Min.Y), V2(1.0f, 0.0f), RenderRect->Color};
        V[2] = (vulkan_vertex){V2(Max.X, Max.Y), V2(1.0f, 1.0f), RenderRect->Color};

        V[3] = (vulkan_vertex){V2(Max.X, Max.Y), V2(1.0f, 1.0f), RenderRect->Color};
        V[4] = (vulkan_vertex){V2(Min.X, Max.Y), V2(0.0f, 1.0f), RenderRect->Color};
        V[5] = (vulkan_vertex){V2(Min.X, Min.Y), V2(0.0f, 0.0f), RenderRect->Color};

        VertexCount += 6;
    }

    vulkan_push_constants PushConstants =
    {
        .Projection = Batch.Projection,
    };

    u32 ImageIndex = 0;

    if (vkAcquireNextImageKHR(
        Vulkan.Device,
        Vulkan.Swapchain,
        U64Max,
        Vulkan.AcquireSemaphore,
        0,
        &ImageIndex
    ))
    {
        VulkanError("failed to acquire image");
        return (false);
    }

    VkCommandBuffer CommandBuffer       = Vulkan.CommandBuffer;
    VkSemaphore     AcquireSemaphore    = Vulkan.AcquireSemaphore;
    VkSemaphore     SubmitSemaphore     = Vulkan.SubmitSemaphore;

    VkCommandBufferBeginInfo BeginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    if (vkResetCommandBuffer(CommandBuffer, 0))
    {
        VulkanError("failed to reset command buffer");
        return (false);
    }

    if (vkBeginCommandBuffer(CommandBuffer, &BeginInfo))
    {
        VulkanError("failed to begin command buffer");
        return (false);
    }

    VkImageMemoryBarrier RenderBarrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstAccessMask = VK_ACCESS_NONE,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = Vulkan.SwapchainImages[ImageIndex],
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
        .renderArea = {.offset = {0, 0}, .extent = Vulkan.SwapchainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(VkRenderingAttachmentInfo)
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = Vulkan.SwapchainImageViews[ImageIndex],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {.float32 = {0.07f, 0.08f, 0.1f, 1.0f}}},
        },
    };

    vkCmdBeginRendering(CommandBuffer, &RenderingInfo);

    VkViewport Viewport =
    {
        .x = 0.0f,
        .y = (f32)Vulkan.SwapchainExtent.height,
        .width = (f32)Vulkan.SwapchainExtent.width,
        .height = -(f32)Vulkan.SwapchainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D Scissor = RenderingInfo.renderArea;

    vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
    vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Vulkan.Pipeline);

    VkWriteDescriptorSet DescriptorWrites[] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = 0,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &(VkDescriptorBufferInfo)
            {
                .buffer = Vulkan.VertexBuffer.Buffer,
                .offset = 0,
                .range = Vulkan.VertexBuffer.Size,
            },
        },
    };

    vkCmdPushDescriptorSet(
        CommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        Vulkan.PipelineLayout,
        0,
        ArrayCount(DescriptorWrites),
        DescriptorWrites
    );

    vkCmdPushConstants(
        CommandBuffer,
        Vulkan.PipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(PushConstants),
        &PushConstants
    );

    vkCmdDraw(CommandBuffer, VertexCount, 1, 0, 0);

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
        .image = Vulkan.SwapchainImages[ImageIndex],
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

    if (vkEndCommandBuffer(CommandBuffer))
    {
        VulkanError("failed to end command buffer");
        return (0);
    }

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
        .pSignalSemaphores = &SubmitSemaphore,
    };

    if (vkQueueSubmit(Vulkan.Queue, 1, &SubmitInfo, 0))
    {
        VulkanError("failed to submit");
        return (false);
    }

    VkPresentInfoKHR PresentInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &SubmitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &Vulkan.Swapchain,
        .pImageIndices = &ImageIndex,
    };

    if (vkQueuePresentKHR(Vulkan.Queue, &PresentInfo))
    {
        VulkanError("failed to present");
        return (false);
    }

    if (vkDeviceWaitIdle(Vulkan.Device))
    {
        VulkanError("failed to wait until device idle after render");
        return (false);
    }

    return (true);
}

static void VulkanError(char* Message)
{
    fprintf(stderr, "[vulkan]: %s\n", Message);
}

static b32 VulkanCreateInstance(void)
{
    if (volkInitialize())
    {
        VulkanError("failed to initialize volk");
        return (false);
    }

    Vulkan.VersionOfAPI = VK_API_VERSION_1_4;

    if (volkGetInstanceVersion() < Vulkan.VersionOfAPI)
    {
        VulkanError("vulkan 1.4 or higher is required");
        return (false);
    }

    const char* Extensions[] =
    {
        "VK_KHR_surface",

    #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        "VK_KHR_wayland_surface",
    #else
        #error Missing Vulkan surface extension for platform
    #endif
    };

    const char* Layers[] =
    {
        "VK_LAYER_KHRONOS_validation",
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
            .apiVersion = Vulkan.VersionOfAPI,
        },
        .ppEnabledExtensionNames = Extensions,
        .enabledExtensionCount = ArrayCount(Extensions),
        .ppEnabledLayerNames = Layers,
        .enabledLayerCount = ArrayCount(Layers),
    };

    if (vkCreateInstance(&InstanceInfo, 0, &Vulkan.Instance))
    {
        VulkanError("failed to create instance");
        return (false);
    }

    volkLoadInstance(Vulkan.Instance);

    return (true);
}

static b32 VulkanCreateSurface(void)
{
    #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VkWaylandSurfaceCreateInfoKHR WaylandSurfaceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .display = Wayland.Display,
            .surface = Wayland.Surface,
        };

        if (vkCreateWaylandSurfaceKHR(Vulkan.Instance, &WaylandSurfaceInfo, 0, &Vulkan.Surface))
        {
            VulkanError("failed to create wayland surface");
            return (false);
        }
    #else
        #error Missing Vulkan surface creation code for platform
    #endif

    return (true);
}

static b32 VulkanPickPhysicalDevice(void)
{
    // TODO(vak): Suballocate this from arena allocator
    VkPhysicalDevice PhysicalDevices[64] = {0};
    u32 PhysicalDeviceCount = ArrayCount(PhysicalDevices);

    if (vkEnumeratePhysicalDevices(Vulkan.Instance, &PhysicalDeviceCount, PhysicalDevices))
    {
        VulkanError("failed to enumerate physical devices");
        return (false);
    }

    if (PhysicalDeviceCount == 0)
    {
        VulkanError("no GPU available");
        return (false);
    }

    VkPhysicalDevice Preferred = {0};
    VkPhysicalDevice Fallback = {0};

    for (u32 Index = 0; Index < PhysicalDeviceCount; Index++)
    {
        VkPhysicalDeviceProperties Properties = {0};
        vkGetPhysicalDeviceProperties(PhysicalDevices[Index], &Properties);

        if (Properties.apiVersion < Vulkan.VersionOfAPI)
            continue;

        if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            if (!Preferred) Preferred = PhysicalDevices[Index];
        }
        else
        {
            if (!Fallback) Fallback = PhysicalDevices[Index];
        }
    }

    Vulkan.PhysicalDevice = (Preferred) ? (Preferred) : (Fallback);

    if (!Vulkan.PhysicalDevice)
    {
        VulkanError("no suitable GPU were found");
        return (false);
    }

    return (true);
}

static b32 VulkanSelectQueueFamily(void)
{
    // TODO(vak): Suballocate this from arena allocator
    VkQueueFamilyProperties QueueFamilies[64] = {0};
    u32 QueueFamilyCount = ArrayCount(QueueFamilies);

    vkGetPhysicalDeviceQueueFamilyProperties(
        Vulkan.PhysicalDevice,
        &QueueFamilyCount,
        QueueFamilies
    );

    if (QueueFamilyCount == 0)
    {
        VulkanError("GPU has no available queue family");
        return (false);
    }

    Vulkan.QueueFamilyIndex = U32Max;

    for (u32 Index = 0; Index < QueueFamilyCount; Index++)
    {
        VkQueueFamilyProperties* Properties = QueueFamilies + Index;

        VkQueueFlags RequiredFlags =
            VK_QUEUE_GRAPHICS_BIT |
            VK_QUEUE_TRANSFER_BIT |
            VK_QUEUE_COMPUTE_BIT;

        if ((Properties->queueFlags & RequiredFlags) == RequiredFlags)
        {
            Vulkan.QueueFamilyIndex = Index;
            break;
        }
    }

    if (Vulkan.QueueFamilyIndex == U32Max)
    {
        VulkanError("unable to find a suitable queue family");
        return (false);
    }

    return (true);
}

static b32 VulkanCreateDevice(void)
{
    const char* Extensions[] =
    {
        "VK_KHR_swapchain",
    };

    VkPhysicalDeviceVulkan14Features Vulkan14Features =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pushDescriptor = 1,
    };

    VkPhysicalDeviceVulkan13Features Vulkan13Features =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &Vulkan14Features,
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
            .queueFamilyIndex = Vulkan.QueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = (f32[]){1.0f},
        },
        .ppEnabledExtensionNames = Extensions,
        .enabledExtensionCount = ArrayCount(Extensions),
    };

    if (vkCreateDevice(Vulkan.PhysicalDevice, &DeviceInfo, 0, &Vulkan.Device))
    {
        VulkanError("failed to create device");
        return (false);
    }

    return (true);
}

static b32 VulkanGetQueue(void)
{
    vkGetDeviceQueue(Vulkan.Device, Vulkan.QueueFamilyIndex, 0, &Vulkan.Queue);
    return (true);
}

static b32 VulkanCreateCommandPool(void)
{
    VkCommandPoolCreateInfo CommandPoolInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = Vulkan.QueueFamilyIndex,
    };

    if (vkCreateCommandPool(Vulkan.Device, &CommandPoolInfo, 0, &Vulkan.CommandPool))
    {
        VulkanError("failed to create command pool");
        return (false);
    }

    return (true);
}

static b32 VulkanAllocateCommandBuffer(void)
{
    VkCommandBufferAllocateInfo CommandBufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = Vulkan.CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(Vulkan.Device, &CommandBufferInfo, &Vulkan.CommandBuffer))
    {
        VulkanError("failed to allocate command buffer");
        return (false);
    }

    return (true);
}

static b32 VulkanCreateSemaphores(void)
{
    VkSemaphoreCreateInfo SemaphoreInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    if (vkCreateSemaphore(Vulkan.Device, &SemaphoreInfo, 0, &Vulkan.AcquireSemaphore))
    {
        VulkanError("failed to create acquire semaphore");
        return (false);
    }

    if (vkCreateSemaphore(Vulkan.Device, &SemaphoreInfo, 0, &Vulkan.SubmitSemaphore))
    {
        VulkanError("failed to create submit semaphore");
        return (false);
    }

    return (true);
}

static b32 VulkanCreateSetLayout(void)
{
    VkDescriptorSetLayoutBinding SetBindings[] =
    {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
    };

    VkDescriptorSetLayoutCreateInfo SetLayoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
        .bindingCount = ArrayCount(SetBindings),
        .pBindings = SetBindings,
    };

    if (vkCreateDescriptorSetLayout(Vulkan.Device, &SetLayoutInfo, 0, &Vulkan.SetLayout))
    {
        VulkanError("failed to create descriptor set layout");
        return (false);
    }

    return (true);
}

static b32 VulkanCreatePipelineLayout(void)
{
    VkPipelineLayoutCreateInfo PipelineLayoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &Vulkan.SetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &(VkPushConstantRange)
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(vulkan_push_constants),
        },
    };

    if (vkCreatePipelineLayout(Vulkan.Device, &PipelineLayoutInfo, 0, &Vulkan.PipelineLayout))
    {
        VulkanError("failed to create pipeline layout");
        return (false);
    }

    return (true);
}

static b32 VulkanCreatePipeline(void)
{
    static u32 VertexCode[] =
    {
        #include "shaders/basic.vert.h"
    };

    static u32 FragmentCode[] =
    {
        #include "shaders/basic.frag.h"
    };

    VkShaderModuleCreateInfo VertexModuleInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(VertexCode),
        .pCode = VertexCode,
    };

    VkShaderModule VertexModule = {0};
    VkResult VertexModuleResult = vkCreateShaderModule(Vulkan.Device, &VertexModuleInfo, 0, &VertexModule);

    VkShaderModuleCreateInfo FragmentModuleInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(FragmentCode),
        .pCode = FragmentCode,
    };

    VkShaderModule FragmentModule = {0};
    VkResult FragmentModuleResult = vkCreateShaderModule(Vulkan.Device, &FragmentModuleInfo, 0, &FragmentModule);

    if (VertexModuleResult)     VulkanError("failed to create vertex shader module");
    if (FragmentModuleResult)   VulkanError("failed to create fragment shader module");

    if (VertexModuleResult || FragmentModuleResult)
    {
        vkDestroyShaderModule(Vulkan.Device, VertexModule, 0);
        vkDestroyShaderModule(Vulkan.Device, FragmentModule, 0);
        return (false);
    }

    VkDescriptorSetLayoutBinding SetBindings[] =
    {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
    };

    VkPipelineShaderStageCreateInfo StageInfos[] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = VertexModule,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = FragmentModule,
            .pName = "main",
        },
    };

    VkPipelineVertexInputStateCreateInfo VertexInputStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineTessellationStateCreateInfo TessellationStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
    };

    VkViewport InitialViewport = {0};
    VkRect2D InitialScissor = {0};

    VkPipelineViewportStateCreateInfo ViewportStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &InitialViewport,
        .scissorCount = 1,
        .pScissors = &InitialScissor,
    };

    VkPipelineRasterizationStateCreateInfo RasterizationStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo MultisampleStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo DepthStencilStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    VkPipelineColorBlendStateCreateInfo ColorBlendStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &(VkPipelineColorBlendAttachmentState)
        {
            .blendEnable = 1,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT,
        },
    };

    VkDynamicState SpecifiedDynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo DynamicStateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = ArrayCount(SpecifiedDynamicStates),
        .pDynamicStates = SpecifiedDynamicStates,
    };

    VkPipelineRenderingCreateInfo PipelineRenderingInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &Vulkan.SwapchainFormat.format,
    };

    VkGraphicsPipelineCreateInfo PipelineInfo =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &PipelineRenderingInfo,
        .stageCount = ArrayCount(StageInfos),
        .pStages = StageInfos,
        .layout = Vulkan.PipelineLayout,
        .pVertexInputState = &VertexInputStateInfo,
        .pInputAssemblyState = &InputAssemblyStateInfo,
        .pTessellationState = &TessellationStateInfo,
        .pViewportState = &ViewportStateInfo,
        .pRasterizationState = &RasterizationStateInfo,
        .pMultisampleState = &MultisampleStateInfo,
        .pDepthStencilState = &DepthStencilStateInfo,
        .pColorBlendState = &ColorBlendStateInfo,
        .pDynamicState = &DynamicStateInfo,
    };

    if (vkCreateGraphicsPipelines(Vulkan.Device, 0, 1, &PipelineInfo, 0, &Vulkan.Pipeline))
    {
        VulkanError("failed to create graphics pipeline");
        return (false);
    }

    vkDestroyShaderModule(Vulkan.Device, FragmentModule, 0);
    vkDestroyShaderModule(Vulkan.Device, VertexModule, 0);

    return (true);
}

static b32 VulkanPickSwapchainFormat(void)
{
    // TODO(vak): Suballocate this from arena allocator
    VkSurfaceFormatKHR SurfaceFormats[512] = {0};
    u32 SurfaceFormatCount = ArrayCount(SurfaceFormats);

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(
        Vulkan.PhysicalDevice,
        Vulkan.Surface,
        &SurfaceFormatCount,
        SurfaceFormats
    ))
    {
        VulkanError("failed to get physical device surface formats");
        return (false);
    }

    for (u32 Index = 0; Index < SurfaceFormatCount; Index++)
    {
        VkSurfaceFormatKHR SurfaceFormat = SurfaceFormats[Index];

        if ((SurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) ||
            (SurfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM))
        {
            Vulkan.SwapchainFormat = SurfaceFormat;
            break;
        }
    }

    if (Vulkan.SwapchainFormat.format == VK_FORMAT_UNDEFINED)
    {
        VulkanError("unable to pick a suitable swapchain format");
        return (false);
    }

    return (true);
}

static b32 VulkanPickPresentMode(void)
{
    Vulkan.PresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // TODO(vak): Suballocate this from arena allocator
    VkPresentModeKHR PresentModes[64] = {0};
    u32 PresentModeCount = ArrayCount(PresentModes);

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
        Vulkan.PhysicalDevice,
        Vulkan.Surface,
        &PresentModeCount,
        PresentModes
    ))
    {
        return (true); // NOTE(vak): Default to FIFO on failure
    }

    for (u32 Index = 0; Index < PresentModeCount; Index++)
    {
        VkPresentModeKHR PresentMode = PresentModes[Index];

        if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            Vulkan.PresentMode = PresentMode;
            break;
        }
    }

    return (true);
}

static u32 VulkanSelectMemoryType(
    VkMemoryPropertyFlags   DesiredPropertyFlags,
    u32                     MemoryTypeBits
)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(Vulkan.PhysicalDevice, &MemoryProperties);

    u32 Result = U32Max;

    for (u32 Index = 0; Index < MemoryProperties.memoryTypeCount; Index++)
    {
        VkMemoryType* MemoryType = MemoryProperties.memoryTypes + Index;

        if ((MemoryTypeBits & (1 << Index)) == 0)
            continue;

        if ((MemoryType->propertyFlags & DesiredPropertyFlags) != DesiredPropertyFlags)
            continue;

        Result = Index;
        break;
    }

    return (Result);
}

static b32 VulkanCreateBuffer(
    vulkan_buffer*          Buffer,
    usize                   Size,
    VkBufferUsageFlags      UsageFlags,
    VkMemoryPropertyFlags   MemoryPropertyFlags,
    b32                     Mapped
)
{
    memset(Buffer, 0, sizeof(vulkan_buffer));

    Buffer->Size = Size;

    VkBufferCreateInfo BufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = Size,
        .usage = UsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(Vulkan.Device, &BufferInfo, 0, &Buffer->Buffer))
    {
        VulkanError("failed to create buffer");
        return (false);
    }

    VkMemoryRequirements MemoryRequirements = {0};
    vkGetBufferMemoryRequirements(Vulkan.Device, Buffer->Buffer, &MemoryRequirements);

    u32 MemoryTypeIndex = VulkanSelectMemoryType(
        MemoryPropertyFlags,
        MemoryRequirements.memoryTypeBits
    );

    if (MemoryTypeIndex == U32Max)
    {
        VulkanError("failed to select suitable memory type for buffer");
        return (false);
    }

    VkMemoryAllocateInfo AllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MemoryRequirements.size,
        .memoryTypeIndex = MemoryTypeIndex,
    };

    if (vkAllocateMemory(Vulkan.Device, &AllocateInfo, 0, &Buffer->Memory))
    {
        VulkanError("failed to allocate memory for buffer");
        return (false);
    }

    if (vkBindBufferMemory(Vulkan.Device, Buffer->Buffer, Buffer->Memory, 0))
    {
        VulkanError("failed to bind memory to buffer");
        return (false);
    }

    if (Mapped)
    {
        if (vkMapMemory(Vulkan.Device, Buffer->Memory, 0, Buffer->Size, 0, &Buffer->Mapping))
        {
            VulkanError("failed to map buffer memory");
            return (false);
        }
    }

    return (true);
}

static void VulkanDestroyBuffer(vulkan_buffer* Buffer)
{
    if (Buffer->Mapping)    vkUnmapMemory(Vulkan.Device, Buffer->Memory);
    if (Buffer->Memory)     vkFreeMemory(Vulkan.Device, Buffer->Memory, 0);
    if (Buffer->Buffer)     vkDestroyBuffer(Vulkan.Device, Buffer->Buffer, 0);
}

