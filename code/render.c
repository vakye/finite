
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

    {
        camera* Camera = &World->Camera;

        PushOrthographic2D(Spec, Batch, CameraGetViewRect(Camera));
    }

    {
        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Bullets); Index++)
        {
            bullet* Bullet = World->Bullets + Index;
            if (!Bullet->Live)
                continue;

            PushRect(Spec, Batch, R2CenterSize(Bullet->P, Bullet->Size), V4(1.0f, 0.2f, 0.2f, 1.0f));
        }
    }

    {
        player* Player = &World->Player;

        PushRect(Spec, Batch, R2CenterSize(Player->P, Player->Size), V4(1.0f, 0.8f, 0.5f, 1.0f));

        v2 HealthBarSize        = V2(0.6f, 0.1f);
        v2 HealthBarOffset      = V2(0.0f, Player->Size.Y + 0.5f*HealthBarSize.Y);
        v2 HealthBarP           = V2Add(Player->P, HealthBarOffset);

        float HealthT           = Player->Health / Player->MaxHealth;
        v2 HealthSize           = V2(HealthBarSize.X * HealthT, HealthBarSize.Y);
        v2 HealthMin            = V2Sub(HealthBarP, V2ScalarMul(0.5f, HealthBarSize));

        PushRect(Spec, Batch, R2CenterSize  (HealthBarP, HealthBarSize),    V4(0.2f, 0.4f, 0.6f, 1.0f));
        PushRect(Spec, Batch, R2MinSize     (HealthMin , HealthSize),       V4(0.2f, 0.4f, 0.8f, 1.0f));

    }

    {
        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Enemies); Index++)
        {
            enemy* Enemy = World->Enemies + Index;
            if (!Enemy->Live)
                continue;

            PushRect(Spec, Batch, R2CenterSize(Enemy->P, Enemy->Size), V4(0.2f, 0.3f, 0.9f, 1.0f));

            v2 HealthBarSize        = V2(0.5f, 0.1f);
            v2 HealthBarOffset      = V2(0.0f, Enemy->Size.Y + 0.5f*HealthBarSize.Y);
            v2 HealthBarP           = V2Add(Enemy->P, HealthBarOffset);

            float HealthT           = Enemy->Health / Enemy->MaxHealth;
            v2 HealthSize           = V2(HealthBarSize.X * HealthT, HealthBarSize.Y);
            v2 HealthMin            = V2Sub(HealthBarP, V2ScalarMul(0.5f, HealthBarSize));

            PushRect(Spec, Batch, R2CenterSize  (HealthBarP, HealthBarSize),    V4(0.6f, 0.2f, 0.2f, 1.0f));
            PushRect(Spec, Batch, R2MinSize     (HealthMin , HealthSize),       V4(0.8f, 0.2f, 0.2f, 1.0f));
        }
    }
}

