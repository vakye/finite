
#include "shared.c"
#include "platform.c"
#include "intrinsics.c"
#include "random.c"
#include "smooth.c"
#include "math.c"
#include "input.c"
#include "render.c"
#include "game.c"

#define GL_VERTEX_SHADER                (0x8B31)
#define GL_FRAGMENT_SHADER              (0x8B30)
#define GL_COLOR_BUFFER_BIT             (0x4000)

#define GL_COMPILE_STATUS               (0x8B81)
#define GL_LINK_STATUS                  (0x8B82)

#define GL_TEXTURE0                     (0x84C0)
#define GL_TEXTURE_2D                   (0x0DE1)
#define GL_TEXTURE_MIN_FILTER           (0x2801)
#define GL_TEXTURE_MAG_FILTER           (0x2800)
#define GL_NEAREST                      (0x2600)
#define GL_TEXTURE_WRAP_S               (0x2802)
#define GL_TEXTURE_WRAP_T               (0x2803)
#define GL_REPEAT                       (0x2901)
#define GL_UNSIGNED_BYTE                (0x1401)
#define GL_RGBA                         (0x1908)
#define GL_ALPHA                        (0x1906)

#define GL_ARRAY_BUFFER                 (0x8892)
#define GL_DYNAMIC_DRAW                 (0x88E8)
#define GL_FLOAT                        (0x1406)

#define GL_TRIANGLES                    (0x0004)

// NOTE(vak): Imports from Javascript side
        
extern void DebugLog                        (const char* Message);

extern float JS_MillisecondsNow             (void);

extern void WebGL_ClearColor                (float R, float G, float B, float A);
extern void WebGL_Clear                     (unsigned int BufferMask);
extern void WebGL_Viewport                  (int X, int Y, int Width, int Height);

extern int  WebGL_CreateShader              (int Type);
extern void WebGL_ShaderSource              (int ShaderID, const char* SourceCode);
extern void WebGL_CompileShader             (int ShaderID);
extern int  WebGL_GetShaderIV               (int ShaderID, int ParameterName);
extern void WebGL_GetShaderInfoLog          (int ShaderID, char* Output, int OutputMaxLength);

extern int  WebGL_CreateProgram             (void);
extern void WebGL_AttachShader              (int ProgramID, int ShaderID);
extern void WebGL_LinkProgram               (int ProgramID);
extern int  WebGL_GetProgramIV              (int ProgramID, int ParameterName);
extern void WebGL_GetProgramInfoLog         (int ProgramID, char* Output, int OutputMaxLength);
extern void WebGL_UseProgram                (int ProgramID);

extern int  WebGL_GetUniformLocation        (int ProgramID, const char* NamePointer);
extern void WebGL_Uniform1I                 (int UniformLocation, int Value);

extern int  WebGL_CreateTexture             (void);
extern void WebGL_BindTexture               (int Target, int TextureID);
extern void WebGL_ActiveTexture             (int Unit);
extern void WebGL_TexParameterI             (int Target, int Parameter, int Value);
extern void WebGL_TexImage2D                (int Target, int Level, int InternalFormat, int Width, int Height, int Border, int Format, int Type, const void* Pointer);

extern int  WebGL_CreateVertexArray         (void);
extern void WebGL_BindVertexArray           (int VertexArrayID);

extern int  WebGL_GenBuffer                 (void);
extern void WebGL_BindBuffer                (int Target, int BufferID);
extern void WebGL_BufferData                (int Target, const void* Pointer, int Length, int Usage);
extern void WebGL_BufferSubData             (int Target, int Offset, int Length, const void* Data);
extern void WebGL_EnableVertexAttribArray   (int Location);
extern void WebGL_VertexAttribPointer       (int Location, int Size, int Type, int Normalized, int Stride, int Offset);

extern void WebGL_DrawArrays                (int Mode, int First, int Count);

static const char* VertShaderSourceCode =
    "#version 300 es\n"

    "precision mediump float;"

    "layout(location = 0) in vec2 InPosition;"
    "layout(location = 1) in vec2 InTexCoord;"
    "layout(location = 2) in vec4 InColor;"
    "layout(location = 3) in float InTextureIndex;"

    "out vec2 VertexTexCoord;"
    "out vec4 VertexColor;"
    "out float VertexTextureIndex;"

    "void main()"
    "{"
        "gl_Position = vec4(InPosition, 0.0, 1.0);"
        "VertexTexCoord = InTexCoord;"
        "VertexColor = InColor;"
        "VertexTextureIndex = InTextureIndex;"
    "}"
;

static const char* FragShaderSourceCode =
    "#version 300 es\n"

    "precision mediump float;"

    "in vec2 VertexTexCoord;"
    "in vec4 VertexColor;"
    "in float VertexTextureIndex;"

    "uniform sampler2D TextureSampler0;"
    "uniform sampler2D TextureSampler1;"

    "out vec4 FragmentColor;"

    "void main()"
    "{"
        "vec4 TexelColor = vec4(1.0);"

        "switch (int(VertexTextureIndex))"
        "{"
            "default:"
            "case 0: TexelColor     = texture(TextureSampler0, VertexTexCoord);     break;"
            "case 1: TexelColor.a   = texture(TextureSampler1, VertexTexCoord).a;   break;"
        "}"

        "FragmentColor = TexelColor * VertexColor;"
    "}"
;

static int      VertShaderID        = 0;
static int      FragShaderID        = 0;
static int      ProgramID           = 0;
static int      VertexArrayID       = 0;
static int      VertexBufferID      = 0;
static usize    WasmNanoseconds     = 0;

#include "wasm_platform.c"

typedef struct
{
    v2 Position;
    v2 TexCoord;
    v4 Color;
    float TextureIndex;
} webgl_vertex;

#define WEBGL_MAX_RECT_PER_FLUSH (4096)

__attribute__((export_name("Init")))
void Init(usize RandomSeed)
{
    static char LogBuffer[1024];

    {
        VertShaderID = WebGL_CreateShader(GL_VERTEX_SHADER);
        FragShaderID = WebGL_CreateShader(GL_FRAGMENT_SHADER);

        WebGL_ShaderSource(VertShaderID, VertShaderSourceCode);
        WebGL_ShaderSource(FragShaderID, FragShaderSourceCode);

        WebGL_CompileShader(VertShaderID);
        WebGL_CompileShader(FragShaderID);

        int ShaderCompilationFailed = 0;

        if (!WebGL_GetShaderIV(VertShaderID, GL_COMPILE_STATUS))
        {
            WebGL_GetShaderInfoLog(VertShaderID, LogBuffer, sizeof(LogBuffer));
            DebugLog("Vertex shader compilation errors:\n");
            DebugLog(LogBuffer);
            ShaderCompilationFailed |= 1;
        }

        if (!WebGL_GetShaderIV(FragShaderID, GL_COMPILE_STATUS))
        {
            WebGL_GetShaderInfoLog(FragShaderID, LogBuffer, sizeof(LogBuffer));
            DebugLog("Fragment shader compilation errors:\n");
            DebugLog(LogBuffer);
            ShaderCompilationFailed |= 1;
        }

        if (ShaderCompilationFailed)
            return;
    }

    {
        ProgramID = WebGL_CreateProgram();

        WebGL_AttachShader(ProgramID, VertShaderID);
        WebGL_AttachShader(ProgramID, FragShaderID);

        WebGL_LinkProgram(ProgramID);

        if (!WebGL_GetProgramIV(ProgramID, GL_LINK_STATUS))
        {
            WebGL_GetProgramInfoLog(ProgramID, LogBuffer, sizeof(LogBuffer));
            DebugLog("Program link errors:\n");
            DebugLog(LogBuffer);
            return;
        }

        WebGL_UseProgram(ProgramID);
    }

    {
        int UniformLocation = -1;

        UniformLocation = WebGL_GetUniformLocation(ProgramID, "TextureSampler0");
        WebGL_Uniform1I(UniformLocation, 0);

        UniformLocation = WebGL_GetUniformLocation(ProgramID, "TextureSampler1");
        WebGL_Uniform1I(UniformLocation, 1);
    }

    {
        unsigned int WhiteImage[2 * 2] =
        {
            0xFFFFFFFF, 0xFFFFFFFF,
            0xFFFFFFFF, 0xFFFFFFFF,
        };

        int Texture = WebGL_CreateTexture();

        WebGL_ActiveTexture(GL_TEXTURE0 + 0);        
        WebGL_BindTexture(GL_TEXTURE_2D, Texture);

        WebGL_TexParameterI(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        WebGL_TexParameterI(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        WebGL_TexParameterI(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        WebGL_TexParameterI(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        WebGL_TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, WhiteImage);
    }

    {
        static unsigned char FontAtlasImage[128 * 64] =
        {
            #include "bitmap_font5x9.h"
        };

        int Texture = WebGL_CreateTexture();

        WebGL_ActiveTexture(GL_TEXTURE0 + 1);        
        WebGL_BindTexture(GL_TEXTURE_2D, Texture);

        WebGL_TexParameterI(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        WebGL_TexParameterI(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        WebGL_TexParameterI(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        WebGL_TexParameterI(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        WebGL_TexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 128, 64, 0, GL_ALPHA, GL_UNSIGNED_BYTE, FontAtlasImage);
    }

    {
        VertexArrayID = WebGL_CreateVertexArray();
        WebGL_BindVertexArray(VertexArrayID);
    }

    {
        VertexBufferID = WebGL_GenBuffer();

        usize VertexPerRect = 6;
        usize VertexBufferSize = WEBGL_MAX_RECT_PER_FLUSH * VertexPerRect * sizeof(webgl_vertex);

        WebGL_BindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
        WebGL_BufferData(GL_ARRAY_BUFFER, 0, VertexBufferSize, GL_DYNAMIC_DRAW);

        WebGL_EnableVertexAttribArray(0);
        WebGL_EnableVertexAttribArray(1);
        WebGL_EnableVertexAttribArray(2);
        WebGL_EnableVertexAttribArray(3);

        WebGL_VertexAttribPointer(0, 2, GL_FLOAT, 0, sizeof(webgl_vertex), 0 * sizeof(float));
        WebGL_VertexAttribPointer(1, 2, GL_FLOAT, 0, sizeof(webgl_vertex), 2 * sizeof(float));
        WebGL_VertexAttribPointer(2, 4, GL_FLOAT, 0, sizeof(webgl_vertex), 4 * sizeof(float));
        WebGL_VertexAttribPointer(3, 1, GL_FLOAT, 0, sizeof(webgl_vertex), 8 * sizeof(float));
    }

    GameSetup(RandomSeed);
}

static int WasmWindowWidth = 0;
static int WasmWindowHeight = 0;

__attribute__((export_name("Resize")))
void Resize(int Width, int Height)
{
    WebGL_Viewport(0, 0, Width, Height);

    WasmWindowWidth = Width;
    WasmWindowHeight = Height;
}

__attribute__((export_name("PrepareInput")))
void PrepareInput(void)
{
    InputPrepareForFrame();
}

__attribute__((export_name("Frame")))
void Frame(float DeltaTime)
{
    WasmNanoseconds += (usize)(DeltaTime * 1e9f);

    RenderPrepareForFrame();
    GameUpdateAndRender(DeltaTime, WasmWindowWidth, WasmWindowHeight);

    WebGL_ClearColor(0.07f, 0.08f, 0.1f, 1.0f);
    WebGL_Clear(GL_COLOR_BUFFER_BIT);

    u32 RectCount = 0;
    static webgl_vertex Vertices[WEBGL_MAX_RECT_PER_FLUSH * 6] = {0};

    render_batch Batch = RenderGetBatch();

    for (usize RectIndex = 0; RectIndex < Batch.RectCount; RectIndex++)
    {
        if (RectCount == WEBGL_MAX_RECT_PER_FLUSH)
        {
            WebGL_BufferSubData(GL_ARRAY_BUFFER, 0, RectCount * 6 * sizeof(webgl_vertex), Vertices);
            WebGL_DrawArrays(GL_TRIANGLES, 0, RectCount*6);
            RectCount = 0;
        }

        render_rect* RenderRect = Batch.Rects + RectIndex;

        v2 Min = RenderRect->Rect.Min;
        v2 Max = RenderRect->Rect.Max;
        v4 Color = RenderRect->Color;

        v2 MinUV = RenderRect->RectUV.Min;
        v2 MaxUV = RenderRect->RectUV.Max;
        float TextureIndex = (float)RenderRect->Texture;

        webgl_vertex* V = Vertices + RectCount*6;

        V[0] = (webgl_vertex){V2(Min.X, Min.Y), V2(MinUV.U, MinUV.V), Color, TextureIndex};
        V[1] = (webgl_vertex){V2(Max.X, Min.Y), V2(MaxUV.U, MinUV.V), Color, TextureIndex};
        V[2] = (webgl_vertex){V2(Max.X, Max.Y), V2(MaxUV.U, MaxUV.V), Color, TextureIndex};

        V[3] = (webgl_vertex){V2(Max.X, Max.Y), V2(MaxUV.U, MaxUV.V), Color, TextureIndex};
        V[4] = (webgl_vertex){V2(Min.X, Max.Y), V2(MinUV.U, MaxUV.V), Color, TextureIndex};
        V[5] = (webgl_vertex){V2(Min.X, Min.Y), V2(MinUV.U, MinUV.V), Color, TextureIndex};

        RectCount++;
    }

    if (RectCount > 0)
    {
        WebGL_BufferSubData(GL_ARRAY_BUFFER, 0, RectCount * 6 * sizeof(webgl_vertex), Vertices);
        WebGL_DrawArrays(GL_TRIANGLES, 0, RectCount*6);
    }
}

__attribute__((export_name("ReportMoveUp")))
void ReportMoveUp(int IsDown)
{
    InputReportButton(InputButton_MoveUp, IsDown);
}

__attribute__((export_name("ReportMoveDown")))
void ReportMoveDown(int IsDown)
{
    InputReportButton(InputButton_MoveDown, IsDown);
}

__attribute__((export_name("ReportMoveLeft")))
void ReportMoveLeft(int IsDown)
{
    InputReportButton(InputButton_MoveLeft, IsDown);
}

__attribute__((export_name("ReportMoveRight")))
void ReportMoveRight(int IsDown)
{
    InputReportButton(InputButton_MoveRight, IsDown);
}

__attribute__((export_name("ReportShoot")))
void ReportShoot(int IsDown)
{
    InputReportButton(InputButton_Shoot, IsDown);
}

__attribute__((export_name("ReportWeapon1")))
void ReportWeapon1(int IsDown)
{
    InputReportButton(InputButton_Weapon1, IsDown);
}

__attribute__((export_name("ReportWeapon2")))
void ReportWeapon2(int IsDown)
{
    InputReportButton(InputButton_Weapon2, IsDown);
}

__attribute__((export_name("ReportWeapon3")))
void ReportWeapon3(int IsDown)
{
    InputReportButton(InputButton_Weapon3, IsDown);
}

__attribute__((export_name("ReportPrev")))
void ReportPrev(int IsDown)
{
    InputReportButton(InputButton_Prev, IsDown);
}

__attribute__((export_name("ReportNext")))
void ReportNext(int IsDown)
{
    InputReportButton(InputButton_Next, IsDown);
}

__attribute__((export_name("ReportBuy")))
void ReportBuy(int IsDown)
{
    InputReportButton(InputButton_Buy, IsDown);
}

