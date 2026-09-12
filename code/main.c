
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "shared.c"
#include "update.c"
#include "render.c"

#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "wayland.c"

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include "volk.h"
#include "volk.c"

static input Input = {0};

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
    vulkan_swapchain*   Swapchain,
    int                 Width,
    int                 Height
)
{
    VK_CHECK(vkDeviceWaitIdle(Device));

    VulkanDestroySwapchain(Device, Swapchain);

    VkSurfaceCapabilitiesKHR SurfaceCaps = {0};

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        PhysicalDevice,
        Surface,
        &SurfaceCaps
    ));

    int DesiredImageCount = (SurfaceCaps.minImageCount <= 3) ? (3) : (SurfaceCaps.minImageCount);

    Swapchain->Width = Width;
    Swapchain->Height = Height;

    VkSwapchainCreateInfoKHR SwapchainInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = Surface,
        .minImageCount = DesiredImageCount,
        .imageFormat = Swapchain->Format.format,
        .imageColorSpace = Swapchain->Format.colorSpace,
        .imageExtent = {Swapchain->Width, Swapchain->Height},
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

static unsigned int VulkanSelectMemoryType(
    VkPhysicalDevice PhysicalDevice,
    VkMemoryPropertyFlags DesiredPropertyFlags,
    unsigned int MemoryTypeBits
)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

    unsigned int Result = ~0u;

    for (unsigned int Index = 0; Index < MemoryProperties.memoryTypeCount; Index++)
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

typedef struct
{
    VkBuffer        Buffer;
    VkDeviceMemory  Memory;
    size_t          Size;
    void*           Mapping;
} vulkan_buffer;

static void VulkanMakeBuffer(
    VkDevice Device,
    VkPhysicalDevice PhysicalDevice,
    size_t Size,
    VkBufferUsageFlags UsageFlags,
    VkMemoryPropertyFlags MemoryPropertyFlags,
    int Mapped,
    vulkan_buffer* Buffer
)
{
    Buffer->Size = Size;

    VkBufferCreateInfo BufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = Size,
        .usage = UsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateBuffer(Device, &BufferInfo, 0, &Buffer->Buffer));

    VkMemoryRequirements MemoryRequirements = {0};
    vkGetBufferMemoryRequirements(Device, Buffer->Buffer, &MemoryRequirements);

    unsigned int MemoryTypeIndex = VulkanSelectMemoryType(
        PhysicalDevice,
        MemoryPropertyFlags,
        MemoryRequirements.memoryTypeBits
    );

    assert(MemoryTypeIndex != ~0u);

    VkMemoryAllocateInfo AllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MemoryRequirements.size,
        .memoryTypeIndex = MemoryTypeIndex,
    };

    VK_CHECK(vkAllocateMemory(Device, &AllocateInfo, 0, &Buffer->Memory));
    VK_CHECK(vkBindBufferMemory(Device, Buffer->Buffer, Buffer->Memory, 0));

    if (Mapped)
    {
        VK_CHECK(vkMapMemory(Device, Buffer->Memory, 0, Buffer->Size, 0, &Buffer->Mapping));
    }
}

static void VulkanDestroyBuffer(VkDevice Device, vulkan_buffer* Buffer)
{
    if (Buffer->Mapping)
        vkUnmapMemory(Device, Buffer->Memory);

    vkFreeMemory(Device, Buffer->Memory, 0);
    vkDestroyBuffer(Device, Buffer->Buffer, 0);
}

typedef struct
{
    float X, Y;
    float U, V;
    float R, G, B, A;
} vulkan_vertex;

typedef struct
{
    float Projection[16];
} vulkan_push_constants;

int main(int ArgCount, char* Args[])
{
    setvbuf(stdout, 0, _IONBF, 0);

    wayland_state Wayland = {0};
    if (!WaylandSetup(&Wayland))
    {
        WaylandShutdown(&Wayland);
        return (1);
    }

    VK_CHECK(volkInitialize());

    const char* InstanceExtensions[] =
    {
        "VK_KHR_surface",
        "VK_KHR_wayland_surface",
    };

    const char* InstanceLayers[] =
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
            .apiVersion = VK_API_VERSION_1_4,
        },
        .ppEnabledExtensionNames = InstanceExtensions,
        .enabledExtensionCount = ARRAY_COUNT(InstanceExtensions),
        .ppEnabledLayerNames = InstanceLayers,
        .enabledLayerCount = ARRAY_COUNT(InstanceLayers),
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

    // TOOD(vak): Allocate this
    static VkPhysicalDevice PhysicalDevices[16] = {0};
    unsigned int PhysicalDeviceCount = ARRAY_COUNT(PhysicalDevices);

    VK_CHECK(vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices));

    VkPhysicalDevice Preferred = {0};
    VkPhysicalDevice Fallback = {0};

    VkPhysicalDevice PhysicalDevice = {0};

    for (unsigned int Index = 0; Index < PhysicalDeviceCount; Index++)
    {
        VkPhysicalDeviceProperties Properties = {0};
        vkGetPhysicalDeviceProperties(PhysicalDevices[Index], &Properties);

        if (Properties.apiVersion < VK_API_VERSION_1_4)
            continue;

        if (Properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            if (!Fallback) Fallback = PhysicalDevices[Index];
        }
        else
        {
            if (!Preferred) Preferred = PhysicalDevices[Index];
        }

        printf("GPU %u: %s", Index, Properties.deviceName);

        if (Preferred == PhysicalDevices[Index])
            printf(" (preferred)");
        else if (Fallback == PhysicalDevices[Index])
            printf(" (fallback)");

        printf(
            " (Vulkan %u.%u.%u)",
            (Properties.apiVersion >> 22) & 0x7F,
            (Properties.apiVersion >> 12) & 0x3FF,
            (Properties.apiVersion >>  0) & 0xFFF
        );

        printf("\n");
    }

    PhysicalDevice = (Preferred) ? (Preferred) : (Fallback);

    if (PhysicalDevice == Preferred)
        printf("Using preferred GPU\n");
    else if (PhysicalDevice == Fallback)
        printf("Using fallback GPU\n");

    assert(PhysicalDevice);

    // TODO(vak): Allocate this
    static VkQueueFamilyProperties QueueFamilies[32] = {0};
    unsigned int QueueFamilyCount = ARRAY_COUNT(QueueFamilies);

    vkGetPhysicalDeviceQueueFamilyProperties(
        PhysicalDevice,
        &QueueFamilyCount,
        QueueFamilies
    );

    unsigned int QueueFamilyIndex = ~0u;

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

    assert(QueueFamilyIndex != ~0u);

    const char* DeviceExtensions[] =
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

    // TODO(vak): Allocate this
    static VkSurfaceFormatKHR SurfaceFormats[256] = {0};
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

    // NOTE(vak): Allocate this
    static VkPresentModeKHR PresentModes[16] = {0};
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

    static unsigned int VertexCode[] =
    {
        #include "shaders/basic.vert.h"
    };

    static unsigned int FragmentCode[] =
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
    VK_CHECK(vkCreateShaderModule(Device, &VertexModuleInfo, 0, &VertexModule));

    VkShaderModuleCreateInfo FragmentModuleInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(FragmentCode),
        .pCode = FragmentCode,
    };

    VkShaderModule FragmentModule = {0};
    VK_CHECK(vkCreateShaderModule(Device, &FragmentModuleInfo, 0, &FragmentModule));

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
        .bindingCount = ARRAY_COUNT(SetBindings),
        .pBindings = SetBindings,
    };

    VkDescriptorSetLayout SetLayout = {0};
    VK_CHECK(vkCreateDescriptorSetLayout(Device, &SetLayoutInfo, 0, &SetLayout));

    VkPipelineLayoutCreateInfo PipelineLayoutInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &SetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &(VkPushConstantRange)
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(vulkan_push_constants),
        },
    };

    VkPipelineLayout PipelineLayout = {0};
    VK_CHECK(vkCreatePipelineLayout(Device, &PipelineLayoutInfo, 0, &PipelineLayout));

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
        .dynamicStateCount = ARRAY_COUNT(SpecifiedDynamicStates),
        .pDynamicStates = SpecifiedDynamicStates,
    };

    VkPipelineRenderingCreateInfo PipelineRenderingInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &Swapchain.Format.format,
    };

    VkGraphicsPipelineCreateInfo PipelineInfo =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &PipelineRenderingInfo,
        .stageCount = ARRAY_COUNT(StageInfos),
        .pStages = StageInfos,
        .layout = PipelineLayout,
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

    VkPipeline Pipeline = {0};
    VK_CHECK(vkCreateGraphicsPipelines(Device, 0, 1, &PipelineInfo, 0, &Pipeline));

    vkDestroyShaderModule(Device, FragmentModule, 0);
    vkDestroyShaderModule(Device, VertexModule, 0);

    vulkan_buffer VertexBuffer = {0};

    VulkanMakeBuffer(
        Device,
        PhysicalDevice,
        1 * 1024 * 1024,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        1,
        &VertexBuffer
    );

    VkSemaphoreCreateInfo SemaphoreInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore AcquireSemaphore = {0};
    VK_CHECK(vkCreateSemaphore(Device, &SemaphoreInfo, 0, &AcquireSemaphore));

    VkSemaphore PresentSemaphore = {0};
    VK_CHECK(vkCreateSemaphore(Device, &SemaphoreInfo, 0, &PresentSemaphore));

    printf("Vulkan setup good\n");

    render_spec RenderSpec = {0};
    RenderSpec.MaxRectCount = (unsigned int)(VertexBuffer.Size / (6*sizeof(vulkan_vertex)));
    RenderSpec.Rects = mmap(0, RenderSpec.MaxRectCount * sizeof(render_rect), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    assert(RenderSpec.Rects);

    world World = {0};
    SetupWorld(&World);

    Input.DeltaTime = 1.0f / 60.0f;
    unsigned int ImageIndex = 0;

    struct timespec FrameBegin = {0};
    clock_gettime(CLOCK_MONOTONIC, &FrameBegin);

    while (!Wayland.IsClosed)
    {
        wl_display_roundtrip(Wayland.Display);

        if (Wayland.ReadyToResize)
            VulkanResizeSwapchain(Device, PhysicalDevice, Surface, &Swapchain, Wayland.Width, Wayland.Height);

        Input.WindowSizeX = Swapchain.Width;
        Input.WindowSizeY = Swapchain.Height;

        render_batch RenderBatch = {0};

        UpdateWorld(&Input, &World);
        RenderWorld(&World, &RenderSpec, &RenderBatch);

        unsigned int VertexCount = 0;
        for (unsigned int RectIndex = 0; RectIndex < RenderBatch.RectCount; RectIndex++)
        {
            render_rect* Rect = RenderSpec.Rects + RectIndex;

            vulkan_vertex* V = (vulkan_vertex*)VertexBuffer.Mapping + VertexCount;

            V[0] = (vulkan_vertex){Rect->MinX, Rect->MinY, 0.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};
            V[1] = (vulkan_vertex){Rect->MaxX, Rect->MinY, 1.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};
            V[2] = (vulkan_vertex){Rect->MaxX, Rect->MaxY, 1.0f, 1.0f, Rect->R, Rect->G, Rect->B, Rect->A};

            V[3] = (vulkan_vertex){Rect->MaxX, Rect->MaxY, 1.0f, 1.0f, Rect->R, Rect->G, Rect->B, Rect->A};
            V[4] = (vulkan_vertex){Rect->MinX, Rect->MaxY, 0.0f, 1.0f, Rect->R, Rect->G, Rect->B, Rect->A};
            V[5] = (vulkan_vertex){Rect->MinX, Rect->MinY, 0.0f, 0.0f, Rect->R, Rect->G, Rect->B, Rect->A};

            VertexCount += 6;
        }

        vulkan_push_constants PushConstants = {0};
        memcpy(PushConstants.Projection, RenderBatch.Projection, sizeof(RenderBatch.Projection));

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

        VkViewport Viewport =
        {
            .x = 0.0f,
            .y = (float)Swapchain.Height,
            .width = (float)Swapchain.Width,
            .height = -(float)Swapchain.Height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D Scissor = RenderingInfo.renderArea;

        vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
        vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

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
                    .buffer = VertexBuffer.Buffer,
                    .offset = 0,
                    .range = VertexBuffer.Size,
                },
            },
        };

        vkCmdPushDescriptorSet(
            CommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            PipelineLayout,
            0,
            ARRAY_COUNT(DescriptorWrites),
            DescriptorWrites
        );

        vkCmdPushConstants(
            CommandBuffer,
            PipelineLayout,
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

        struct timespec Now = {0};
        clock_gettime(CLOCK_MONOTONIC, &Now);

        Input.DeltaTime =
            (double)(Now.tv_sec - FrameBegin.tv_sec) +
            (double)(Now.tv_nsec - FrameBegin.tv_nsec) * 1e-9;

        clock_gettime(CLOCK_MONOTONIC, &FrameBegin);
    }

    vkDestroySemaphore(Device, PresentSemaphore, 0);
    vkDestroySemaphore(Device, AcquireSemaphore, 0);
    VulkanDestroyBuffer(Device, &VertexBuffer);
    vkDestroyPipeline(Device, Pipeline, 0);
    vkDestroyPipelineLayout(Device, PipelineLayout, 0);
    vkDestroyDescriptorSetLayout(Device, SetLayout, 0);
    vkDestroyCommandPool(Device, CommandPool, 0);
    VulkanDestroySwapchain(Device, &Swapchain);
    vkDestroyDevice(Device, 0);
    vkDestroySurfaceKHR(Instance, Surface, 0);
    vkDestroyInstance(Instance, 0);

    WaylandShutdown(&Wayland);

    return (0);
}
