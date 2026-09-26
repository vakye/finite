
#pragma once

// NOTE(vak): Cheatsheet

typedef enum
{
    GameTexture_White       = 0,
    GameTexture_Font5x9,            // NOTE(vak): Texture is (128 x 64) | Each cell is (7 x 9)
    GameTexture_COUNT,
} game_texture;

typedef struct
{
    rect2           Rect;
    v4              Color;

    rect2           RectUV;
    game_texture    Texture;
} render_rect;

typedef struct
{
    render_rect*    Rects;
    usize           RectCount;
} render_batch;

static void         RenderPrepareForFrame   (void);
static void         RenderOrthographic2D    (rect2 ViewRect);
static void         RenderRect              (rect2 Rect, v4 Color);
static void         RenderRectTextured      (rect2 Rect, v4 Color, rect2 RectUV, game_texture Texture);

// NOTE(vak): Text renderer expects window coordinates, and (0, 0) at top-left.
static v2           RenderText              (string Text, v2 Position, v4 Color); 
static v2           RenderTextCentered      (string Text, v2 Position, v4 Color); 
static v2           RenderGetTextSize       (string Text);
static f32          RenderGetTextSizeX      (string Text);
static f32          RenderGetTextSizeY      (string Text);
static f32          RenderGetLineHeight     (void);

static render_batch RenderGetBatch          (void);

// NOTE(vak): Implementation

typedef struct
{
    m4x4        Projection;
    f32         TextScale;
    f32         LineHeightScale;
    u32         FontCellWidth;
    u32         FontCellHeight;
    usize       RectCount;
    render_rect Rects[16384];
} render_state;

static render_state Render = {0};

static void RenderPrepareForFrame(void)
{
    Render.RectCount        = 0;
    Render.TextScale        = 2.0f;
    Render.LineHeightScale  = 1.4f;
    Render.FontCellWidth    = 7.0f;
    Render.FontCellHeight   = 9.0f;
}

static void RenderOrthographic2D(rect2 ViewRect)
{
    Render.Projection = M4x4Orthographic2D(ViewRect);
}

static v2 RenderTransform2D(m4x4 Projection, v2 A)
{
    v4 Transformed = M4x4MultiplyV4(Projection, V4(A.X, A.Y, 0, 1));
    v2 Result = V2(Transformed.X, Transformed.Y);
    return (Result);
}

static void RenderRect(rect2 Rect, v4 Color)
{
    RenderRectTextured(Rect, Color, R2MinMax(V2(0, 0), V2(1, 1)), GameTexture_White);
}

extern void DebugLog(const char* Message);

static void RenderRectTextured(rect2 Rect, v4 Color, rect2 RectUV, game_texture Texture)
{
    if (Render.RectCount < ArrayCount(Render.Rects))
    {
        render_rect* RenderRect = Render.Rects + Render.RectCount++;

        v2 Min = RenderTransform2D(Render.Projection, Rect.Min);
        v2 Max = RenderTransform2D(Render.Projection, Rect.Max);

        RenderRect->Rect  = R2MinMax(Min, Max);
        RenderRect->Color = Color;

        RenderRect->RectUV = RectUV;
        RenderRect->Texture = Texture;
    }
}

static v2 RenderText(string Text, v2 Position, v4 Color)
{
    game_texture Texture    = GameTexture_Font5x9;
    u32 TextureWidth        = 128;
    u32 TextureHeight       = 64;
    u32 CellCountX          = TextureWidth / Render.FontCellWidth;
    u32 CellCountY          = TextureHeight / Render.FontCellHeight;

    v2 CursorP      = Position;
    v2 GlyphSize    = V2(Render.FontCellWidth * Render.TextScale, Render.FontCellHeight * Render.TextScale);

    for (usize Index = 0; Index < Text.Size; Index++)
    {
        char Character = Text.Data[Index];

        if ((Character > 32) && (Character <= 126))
        {
            u32 CellIndex = Character - 32;
            u32 CellX     = CellIndex % CellCountX;
            u32 CellY     = CellIndex / CellCountX;

            u32 TexelMinX = CellX * Render.FontCellWidth;
            u32 TexelMinY = CellY * Render.FontCellHeight;
            u32 TexelMaxX = TexelMinX + Render.FontCellWidth;
            u32 TexelMaxY = TexelMinY + Render.FontCellHeight;

            v2 Min = CursorP;
            v2 Max = V2Add(Min, GlyphSize);

            v2 MinUV = V2((f32)TexelMinX / (f32)TextureWidth, (f32)TexelMinY / (f32)TextureHeight);
            v2 MaxUV = V2((f32)TexelMaxX / (f32)TextureWidth, (f32)TexelMaxY / (f32)TextureHeight);

            RenderRectTextured(R2MinMax(Min, Max), Color, R2MinMax(MinUV, MaxUV), Texture);

            CursorP.X += GlyphSize.X;
        }
        else if (Character == ' ')
        {
            CursorP.X += GlyphSize.X;
        }
        else if (Character == '\t')
        {
            CursorP.X += 4.0f * GlyphSize.X;
        }
        else if (Character == '\n')
        {
            CursorP.X = Position.X;
            CursorP.Y += Render.LineHeightScale * GlyphSize.Y;
        }
    }

    return (CursorP);
}

static v2 RenderTextCentered(string Text, v2 Position, v4 Color)
{
    Position.X -= 0.5f*RenderGetTextSizeX(Text);
    return RenderText(Text, Position, Color);
}

static v2 RenderGetTextSize(string Text)
{
    v2 Result = V2(RenderGetTextSizeX(Text), RenderGetTextSizeY(Text));
    return (Result);
}

static f32 RenderGetTextSizeX(string Text)
{
    f32 WidestRowSoFar = 0.0f;
    f32 CurrentRowWidth = 0.0f;

    v2 GlyphSize = V2(Render.FontCellWidth * Render.TextScale, Render.FontCellHeight * Render.TextScale);

    for (usize Index = 0; Index < Text.Size; Index++)
    {
        char Character = Text.Data[Index];

        if ((Character >= 32) && (Character <= 126))
        {
            CurrentRowWidth += GlyphSize.X;
        }
        else if (Character == '\t')
        {
            CurrentRowWidth += 4.0f * GlyphSize.X;
        }
        else if (Character == '\n')
        {
            WidestRowSoFar = Maximum(WidestRowSoFar, CurrentRowWidth);
            CurrentRowWidth = 0.0f;
        }
    }

    WidestRowSoFar = Maximum(WidestRowSoFar, CurrentRowWidth);

    return (WidestRowSoFar);
}

static f32 RenderGetTextSizeY(string Text)
{
    u32 LineCount = 1;

    for (usize Index = 0; Index < Text.Size; Index++)
    {
        char Character = Text.Data[Index];

        if (Character == '\n')
            LineCount++;
    }

    v2 GlyphSize = V2(Render.FontCellWidth * Render.TextScale, Render.FontCellHeight * Render.TextScale);
    f32 Result = LineCount * Render.LineHeightScale * GlyphSize.Y;

    return (Result);
}

static f32 RenderGetLineHeight(void)
{
    f32 GlyphSizeY = Render.FontCellHeight * Render.TextScale;
    f32 Result = Render.LineHeightScale * GlyphSizeY;
    return (Result);
}

static render_batch RenderGetBatch(void)
{
    render_batch Batch =
    {
        .RectCount  = Render.RectCount,
        .Rects      = Render.Rects,
    };

    return (Batch);
}

