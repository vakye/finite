
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "shared.c"
#include "intrinsics.c"
#include "random.c"
#include "math.c"
#include "input.c"
#include "render.c"
#include "game.c"

#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "wayland.c"

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#include "vulkan.c"

s32 main(s32 ArgCount, char* Args[])
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

    struct timespec SeedTime = {0};
    clock_gettime(CLOCK_MONOTONIC, &SeedTime);
    usize RandomSeed = SeedTime.tv_nsec;

    GameSetup(RandomSeed);

    float DeltaTime = 1.0f/60.0f;

    struct timespec FrameBegin = {0};
    clock_gettime(CLOCK_MONOTONIC, &FrameBegin);

    while (!WaylandIsClosed())
    {
        InputPrepareForFrame();
        RenderPrepareForFrame();

        WaylandPollEvents();

        if (WaylandShouldResize())
            if (!VulkanResize(WaylandGetWidth(), WaylandGetHeight()))
                break;

        GameUpdateAndRender(DeltaTime, WaylandGetWidth(), WaylandGetHeight());

        if (!VulkanRender())
            break;

        WaylandPresent();

        struct timespec Now = {0};
        clock_gettime(CLOCK_MONOTONIC, &Now);

        DeltaTime =
            (f64)(Now.tv_sec - FrameBegin.tv_sec) +
            (f64)(Now.tv_nsec - FrameBegin.tv_nsec) * 1e-9;

        clock_gettime(CLOCK_MONOTONIC, &FrameBegin);
    }

    VulkanShutdown();
    WaylandShutdown();

    return (0);
}
