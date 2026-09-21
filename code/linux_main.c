
#include "shared.c"
#include "intrinsics.c"
#include "random.c"
#include "math.c"
#include "input.c"
#include "render.c"
#include "game.c"

// NOTE(vak): This syscall implementation only works for x86_64 right now

#define __NR_close          (3)
#define __NR_mmap           (9)
#define __NR_munmap         (11)
#define __NR_nanosleep      (35)
#define __NR_clock_gettime  (228)

static ssize LinuxSyscall(usize NR, usize A, usize B, usize C, usize D, usize E, usize F)
{
    ssize Result = 0;

    register usize R10 __asm__("r10") = D;
    register usize R8  __asm__("r8")  = E;
    register usize R9  __asm__("r9")  = F;

    __asm__ volatile (
        "syscall" :
        "=a"(Result) :
        "a"(NR),
        "D"(A),
        "S"(B),
        "d"(C),
        "r"(R10),
        "r"(R8),
        "r"(R9) :
        "memory", "rcx", "r11"
    );

    return (Result);
}

#define PROT_NONE   (0x00)
#define PROT_READ   (0x01)
#define PROT_WRITE  (0x02)

#define MAP_PRIVATE (0x02)

#define CLOCK_MONOTONIC (1)

struct timespec
{
    s64 tv_sec;
    s64 tv_nsec;
};

#define close(fd) (int)LinuxSyscall(__NR_close, fd, 0, 0, 0, 0, 0)
#define mmap(addr, length, prot, flags, fd, offset) (void*)LinuxSyscall(__NR_mmap, (usize)(addr), length, prot, flags, fd, offset)
#define munmap(addr, length) (int)LinuxSyscall(__NR_munmap, (usize)(addr), length, 0, 0, 0, 0)
#define nanosleep(duration, rem) (int)LinuxSyscall(__NR_nanosleep, (usize)(duration), (usize)(rem), 0, 0, 0, 0)
#define clock_gettime(clockid, res) (int)LinuxSyscall(__NR_clock_gettime, clockid, (usize)(res), 0, 0, 0, 0)

#include "wayland.c"

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#include "vulkan.c"

__attribute__((naked))
void start(void)
{
    __asm__ volatile (
        "mov 0(%rsp), %edi\n"
        "lea 8(%rsp), %rsi\n"
        "call main\n"

        "mov %eax, %edi\n"
        "mov $231, %eax\n" // NOTE(vak): __NR_exit_group
        "syscall\n"
    );
}

s32 main(s32 ArgCount, char* Args[])
{
    if (!WaylandSetup())
    {
        WaylandShutdown();
        return (1);
    }

    WaylandToggleFullscreen();

    static u32 WhiteImageRGBA[2 * 2] =
    {
        0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF,
    };

    static u8 FontImageAlpha[128 * 64 * 1] =
    {
        #include "bitmap_font5x9.h"
    };

    vulkan_texture_load_set TextureLoadSet =
    {
        .LoadInfos =
        {
            [GameTexture_White]     = {VulkanPixelKind_RGBA,    WhiteImageRGBA, 2,   2 },
            [GameTexture_Font5x9]   = {VulkanPixelKind_Alpha,   FontImageAlpha, 128, 64},
        },
    };

    if (!VulkanSetup(&TextureLoadSet))
    {
        VulkanShutdown();
        return (1);
    }

    struct timespec SeedTime = {0};
    clock_gettime(CLOCK_MONOTONIC, &SeedTime);
    usize RandomSeed = SeedTime.tv_nsec;

    GameSetup(RandomSeed);

    float TargetDeltaTime = 1.0f/165.0f;
    float DeltaTime = TargetDeltaTime;

    struct timespec FrameBegin = {0};
    clock_gettime(CLOCK_MONOTONIC, &FrameBegin);

    while (!WaylandIsClosed())
    {
        InputPrepareForFrame();
        RenderPrepareForFrame();

        WaylandPollEvents();

        if (WaylandShouldResize())
        {
            if (!VulkanResize(WaylandGetWidth(), WaylandGetHeight()))
                break;

            WaylandNotifyResized();
        }

        GameUpdateAndRender(DeltaTime, WaylandGetWidth(), WaylandGetHeight());

        if (!VulkanRender())
            break;

        WaylandPresent();

        struct timespec Now = {0};
        clock_gettime(CLOCK_MONOTONIC, &Now);

        DeltaTime =
            (f64)(Now.tv_sec - FrameBegin.tv_sec) +
            (f64)(Now.tv_nsec - FrameBegin.tv_nsec) * 1e-9;

        if (DeltaTime < TargetDeltaTime)
        {
            f64 Seconds = TargetDeltaTime - DeltaTime;

            struct timespec Duration = (struct timespec)
            {
                .tv_sec = (usize)(Seconds),
                .tv_nsec = (usize)(Seconds * 1e9) % 1000000000,
            };

            struct timespec Remaining = {0};

            do
            {
                nanosleep(&Duration, &Remaining);
                Duration = Remaining;
            } while (Remaining.tv_nsec);
        }

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

