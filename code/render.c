
#pragma once

// NOTE(vak): Cheatsheet

typedef struct
{
    rect2   Rect;
    v4      Color;
} render_rect;

typedef struct
{
    m4x4            Projection;
    render_rect*    Rects;
    usize           RectCount;
} render_batch;

static void         RenderPrepareForFrame   (void);
static void         RenderOrthographic2D    (rect2 ViewRect);
static void         RenderRect              (rect2 Rect, v4 Color);
static render_batch RenderGetBatch          (void);

// NOTE(vak): Implementation

typedef struct
{
    m4x4        Projection;
    usize       RectCount;
    render_rect Rects[65536];
} render_state;

static render_state Render = {0};

static void RenderPrepareForFrame(void)
{
    Render.RectCount = 0;
}

static void RenderOrthographic2D(rect2 ViewRect)
{
    Render.Projection = M4x4Orthographic2D(ViewRect);
}

static void RenderRect(rect2 Rect, v4 Color)
{
    Assert(Render.RectCount < ArrayCount(Render.Rects));

    render_rect* RenderRect = Render.Rects + Render.RectCount++;

    RenderRect->Rect  = Rect;
    RenderRect->Color = Color;
}

static render_batch RenderGetBatch(void)
{
    render_batch Batch =
    {
        .Projection = Render.Projection,
        .RectCount  = Render.RectCount,
        .Rects      = Render.Rects,
    };

    return (Batch);
}

