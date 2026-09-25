
#include "shared.c"
#include "platform.c"
#include "intrinsics.c"
#include "random.c"
#include "smooth.c"
#include "math.c"
#include "input.c"
#include "render.c"
#include "game.c"

// NOTE(vak): Linux syscall implementation

#define __NR_write          (1)
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

// NOTE(vak): Definitions for syscalls

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

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

// NOTE(vak): Macros for syscalls

#define write(fd, buf, count)                       (ssize) LinuxSyscall(__NR_write, fd, (usize)(buf), count, 0, 0, 0)
#define close(fd)                                   (int)   LinuxSyscall(__NR_close, fd, 0, 0, 0, 0, 0)
#define mmap(addr, length, prot, flags, fd, offset) (void*) LinuxSyscall(__NR_mmap, (usize)(addr), length, prot, flags, fd, offset)
#define munmap(addr, length)                        (int)   LinuxSyscall(__NR_munmap, (usize)(addr), length, 0, 0, 0, 0)
#define nanosleep(duration, rem)                    (int)   LinuxSyscall(__NR_nanosleep, (usize)(duration), (usize)(rem), 0, 0, 0, 0)
#define clock_gettime(clockid, res)                 (int)   LinuxSyscall(__NR_clock_gettime, clockid, (usize)(res), 0, 0, 0, 0)

// NOTE(vak): Platform code

#include "linux_platform.c"

// NOTE(vak): Main code

#include "wayland.c"

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#include "vulkan.c"

__attribute__((naked))
void start(void)
{
    // NOTE(vak): Translation of the assembly below to C:
    //      int     ArgCount    = *(int*)(StackPointer + 0);
    //      char*   Args        = (char*)StackPointer + 8;
    //      char*   Envp        = (char*)StackPointer + 8 + 8*ArgCount;
    //      int     ReturnCode  = main(ArgCount, Args, Envp);
    //      exit_group(ReturnCode);

    __asm__ volatile (
        "mov 0(%rsp),           %edi\n"
        "lea 8(%rsp),           %rsi\n"
        "lea 8(%rsp, %rdi, 8),  %rdx\n"
        "call main\n"

        "mov %eax, %edi\n"
        "mov $231, %eax\n" // NOTE(vak): __NR_exit_group
        "syscall\n"
    );
}

static string LinuxGetEnv(char* Envp[], string VariableName)
{
    string VariableAndValue = NilString;

    for (usize Index = 0; Envp[Index] != 0; Index++)
    {
        string Candidate = CString(Envp[Index]);

        if (StringStartsWith(Candidate, VariableName))
        {
            VariableAndValue = Candidate;
            break;
        }
    }

    if (IsNilString(VariableAndValue))
        return (NilString);

    usize From = Minimum(VariableName.Size + 1, VariableAndValue.Size);
    usize Size = VariableAndValue.Size - From;

    string Value = StringView(VariableAndValue, From, Size);
    return (Value);
}

static void LinuxGetSoundSamples(s16* SampleBuffer, usize SampleCount, usize SampleRate, usize ChannelCount)
{
    usize BytesPerSample = sizeof(s16) * ChannelCount;
    ZeroMemory(SampleBuffer, BytesPerSample * SampleCount);
}

s32 main(s32 ArgCount, char* Args[], char* Envp[])
{
    string XdgSessionType = LinuxGetEnv(Envp, Str("XDG_SESSION_TYPE"));

    if (!StringEqual(XdgSessionType, Str("wayland")))
    {
        PrintErr(Str("error: Sorry, only wayland is supported right now!"));
        return (1);
    }

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

// NOTE(vak): CRT stuff

void* memset(void* DestInit, s32 Byte, usize Size)
{
    u8* Dest = (u8*)DestInit;

    while (Size--)
        *Dest++ = (u8)Byte;

    return (DestInit);
}

void* memcpy(void* DestInit, void* SourceInit, usize Size)
{
    u8* Dest = (u8*)DestInit;
    u8* Source = (u8*)SourceInit;

    while (Size--)
        *Dest++ = *Source++;

    return (DestInit);
}

