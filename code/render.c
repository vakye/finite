
typedef struct
{
    rect2   Rect;
    v4      Color;
} render_rect;

typedef struct
{
    unsigned int    MaxRectCount;
    render_rect*    Rects;
} render_spec;

typedef struct
{
    m4x4            Projection;
    unsigned int    RectCount;
} render_batch;

static void PushOrthographic2D(render_spec* Spec, render_batch* Batch, rect2 ViewRect)
{
    Batch->Projection = M4x4Orthographic2D(ViewRect);
}

static void PushRect(render_spec* Spec, render_batch* Batch, rect2 Rect, v4 Color)
{
    assert(Batch->RectCount < Spec->MaxRectCount);

    render_rect* RenderRect = Spec->Rects + Batch->RectCount++;

    RenderRect->Rect = Rect;
    RenderRect->Color = Color;
}

static void RenderWorld(
    world*          World,  // NOTE(vak): Input
    render_spec*    Spec,   // NOTE(vak): Input
    render_batch*   Batch   // NOTE(vak): Output
)
{
    memset(Batch, 0, sizeof(render_batch));

    components* Components = &World->Components;
    camera* Camera = &World->Camera;

    PushOrthographic2D(Spec, Batch, CameraGetViewRect(Camera));

    for (sprite_id SpriteID = 1; SpriteID <= Components->SpriteSet.Count; SpriteID++)
    {
        if (!SparseSetIsSlotUsed(&Components->SpriteSet, SpriteID))
            continue;

        sprite* Sprite  = ComponentGetSprite(Components, SpriteID);
        body*   Body    = ComponentGetBody  (Components, Sprite->AttachedToBodyID);

        v2 Center   = V2Add(Body->P, Sprite->Offset);
        v2 Size     = Sprite->Size;

        PushRect(Spec, Batch, R2CenterSize(Center, Size), Sprite->Color);
    }
}

