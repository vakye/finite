
#include "shared.c"

#define GL_VERTEX_SHADER                (0x8B31)
#define GL_FRAGMENT_SHADER              (0x8B30)
#define GL_COLOR_BUFFER_BIT             (0x4000)

#define GL_COMPILE_STATUS               (0x8B81)
#define GL_LINK_STATUS                  (0x8B82)

#define GL_ARRAY_BUFFER                 (0x8892)
#define GL_STATIC_DRAW                  (0x88E4)
#define GL_FLOAT                        (0x1406)

#define GL_TRIANGLES                    (0x0004)

// NOTE(vak): Imports from Javascript side
        
extern void DebugLog                        (const char* Message);

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

extern int  WebGL_CreateVertexArray         (void);
extern void WebGL_BindVertexArray           (int VertexArrayID);

extern int  WebGL_GenBuffer                 (void);
extern void WebGL_BindBuffer                (int Target, int BufferID);
extern void WebGL_BufferData                (int Target, const void* Pointer, int Length, int Usage);
extern void WebGL_EnableVertexAttribArray   (int Location);
extern void WebGL_VertexAttribPointer       (int Location, int Size, int Type, int Normalized, int Stride, int Offset);

extern void WebGL_DrawArrays                (int Mode, int First, int Count);

static const char* VertShaderSourceCode =
    "#version 300 es\n"

    "precision mediump float;"

    "layout(location = 0) in vec2 InPosition;"
    "layout(location = 1) in vec2 InTexCoord;"
    "layout(location = 2) in vec4 InColor;"

    "out vec2 VertexTexCoord;"
    "out vec4 VertexColor;"

    "void main()"
    "{"
        "gl_Position    = vec4(InPosition, 0.0, 1.0);"
        "VertexTexCoord = InTexCoord;"
        "VertexColor    = InColor;"
    "}"
;

static const char* FragShaderSourceCode =
    "#version 300 es\n"

    "precision mediump float;"

    "in vec2 VertexTexCoord;"
    "in vec4 VertexColor;"

    "out vec4 FragmentColor;"

    "void main()"
    "{"
        "FragmentColor = VertexColor;"
    "}"
;

static int VertShaderID     = 0;
static int FragShaderID     = 0;
static int ProgramID        = 0;
static int VertexArrayID    = 0;
static int VertexBufferID   = 0;

__attribute__((export_name("Init")))
void Init(void)
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
        VertexArrayID = WebGL_CreateVertexArray();
        WebGL_BindVertexArray(VertexArrayID);
    }

    {
        static float VertexData[] =
        {
            -0.5f, -0.5f,       0.0f, 0.0f,         1.0f, 0.0f, 0.0f, 1.0f,
            +0.5f, -0.5f,       1.0f, 0.0f,         0.0f, 1.0f, 0.0f, 1.0f,
            +0.0f, +0.5f,       0.5f, 1.0f,         0.0f, 0.0f, 1.0f, 1.0f,
        };

        VertexBufferID = WebGL_GenBuffer();

        WebGL_BindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
        WebGL_BufferData(GL_ARRAY_BUFFER, VertexData, sizeof(VertexData), GL_STATIC_DRAW);

        WebGL_EnableVertexAttribArray(0);
        WebGL_EnableVertexAttribArray(1);
        WebGL_EnableVertexAttribArray(2);

        WebGL_VertexAttribPointer(0, 2, GL_FLOAT, 0, 8*sizeof(float), 0 * sizeof(float));
        WebGL_VertexAttribPointer(1, 2, GL_FLOAT, 0, 8*sizeof(float), 2 * sizeof(float));
        WebGL_VertexAttribPointer(2, 4, GL_FLOAT, 0, 8*sizeof(float), 4 * sizeof(float));
    }
}

__attribute__((export_name("Resize")))
void Resize(int Width, int Height)
{
    WebGL_Viewport(0, 0, Width, Height);
}

__attribute__((export_name("Frame")))
void Frame(float TimeNow)
{
    WebGL_ClearColor(0.07f, 0.08f, 0.1f, 1.0f);
    WebGL_Clear(GL_COLOR_BUFFER_BIT);
    WebGL_DrawArrays(GL_TRIANGLES, 0, 3);
}

