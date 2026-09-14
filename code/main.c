
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "shared.c"
#include "intrinsics.c"
#include "random.c"
#include "math.c"
#include "update.c"
#include "render.c"

#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "wayland.c"

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#include "vulkan.c"

int main(int ArgCount, char* Args[])
{
    setvbuf(stdout, 0, _IONBF, 0);

    if (!WaylandSetup())
    {
        WaylandShutdown();
        return (1);
    }

    if (!VulkanSetup())
    {
        VulkanShutdown();
        return (1);
    }

    render_spec RenderSpec = {0};
    {
        RenderSpec.MaxRectCount = 65536;
        RenderSpec.Rects = mmap(0, RenderSpec.MaxRectCount * sizeof(render_rect), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

        if (!RenderSpec.Rects)
        {
            fprintf(stderr, "failed to allocate RenderSpec.Rects\n");
            return (1);
        }
    }

    world World = {0};
    {
        struct timespec Now = {0};
        clock_gettime(CLOCK_MONOTONIC, &Now);

        size_t RandomSeed = Now.tv_nsec;

        SetupWorld(&World, RandomSeed);
    }

    platform Platform = {0};
    platform_input* Input = &Platform.Input;

    {
        Platform.DeltaTime = 1.0f / 60.0f;
    }

    struct timespec FrameBegin = {0};
    clock_gettime(CLOCK_MONOTONIC, &FrameBegin);

    while (!WaylandIsClosed())
    {
        WaylandPollEvents();

        if (WaylandShouldResize())
        {
            if (!VulkanResize(WaylandGetWidth(), WaylandGetHeight()))
                break;
        }

        Platform.WindowSizeX = WaylandGetWidth();
        Platform.WindowSizeY = WaylandGetHeight();

        render_batch RenderBatch = {0};

        UpdateWorld(&Platform, &World);
        RenderWorld(&World, &RenderSpec, &RenderBatch);

        if (!VulkanRender(&RenderSpec, &RenderBatch))
            break;

        WaylandPresent();

        struct timespec Now = {0};
        clock_gettime(CLOCK_MONOTONIC, &Now);

        Platform.DeltaTime =
            (double)(Now.tv_sec - FrameBegin.tv_sec) +
            (double)(Now.tv_nsec - FrameBegin.tv_nsec) * 1e-9;

        clock_gettime(CLOCK_MONOTONIC, &FrameBegin);
    }

    VulkanShutdown();
    WaylandShutdown();

    return (0);
}
