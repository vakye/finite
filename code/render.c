
typedef struct
{
    float MinX, MinY;
    float MaxX, MaxY;
    float R, G, B, A;
} render_rect;

typedef struct
{
    unsigned int    MaxRectCount;
    render_rect*    Rects;
} render_spec;

typedef struct
{
    float           Projection[16];
    unsigned int    RectCount;
} render_batch;

static void PushOrthographic2D(
    render_spec* Spec,
    render_batch* Batch,
    float ViewMinX, float ViewMinY,
    float ViewMaxX, float ViewMaxY
)
{
    float ScaleX = 2.0f / (ViewMaxX - ViewMinX);
    float ScaleY = 2.0f / (ViewMaxY - ViewMinY);

    float TranslateX = ScaleX * -0.5f * (ViewMinX + ViewMaxX);
    float TranslateY = ScaleY * -0.5f * (ViewMinY + ViewMaxY);

    memset(Batch->Projection, 0, sizeof(Batch->Projection));

    Batch->Projection[0]    = ScaleX;
    Batch->Projection[5]    = ScaleY;
    Batch->Projection[10]   = 1.0f;
    Batch->Projection[15]   = 1.0f;

    Batch->Projection[12]   = TranslateX;
    Batch->Projection[13]   = TranslateY;
}

static void PushRect(
    render_spec* Spec,
    render_batch* Batch,
    float CenterX, float CenterY,
    float SizeX, float SizeY,
    float R, float G, float B, float A
)
{
    assert(Batch->RectCount < Spec->MaxRectCount);

    render_rect* Rect = Spec->Rects + Batch->RectCount++;

    Rect->MinX = CenterX - 0.5f*SizeX;
    Rect->MinY = CenterY - 0.5f*SizeY;
    Rect->MaxX = CenterX + 0.5f*SizeX;
    Rect->MaxY = CenterY + 0.5f*SizeY;

    Rect->R = R;
    Rect->G = G;
    Rect->B = B;
    Rect->A = A;
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

        float ViewSizeX = GetCameraViewSizeX(Camera);
        float ViewSizeY = GetCameraViewSizeY(Camera);

        float ViewMinX = Camera->ViewCenterX - 0.5f * ViewSizeX;
        float ViewMinY = Camera->ViewCenterY - 0.5f * ViewSizeY;
        float ViewMaxX = Camera->ViewCenterX + 0.5f * ViewSizeX;
        float ViewMaxY = Camera->ViewCenterY + 0.5f * ViewSizeY;

        PushOrthographic2D(
            Spec, Batch,
            ViewMinX, ViewMinY,
            ViewMaxX, ViewMaxY
        );
    }

    {
        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Bullets); Index++)
        {
            bullet* Bullet = World->Bullets + Index;
            if (!Bullet->Live)
                continue;

            PushRect(
                Spec, Batch,
                Bullet->X, Bullet->Y,
                Bullet->SizeX, Bullet->SizeY,
                1.0f, 0.2f, 0.2f, 1.0f
            );
        }
    }

    {
        player* Player = &World->Player;

        PushRect(
            Spec, Batch,
            Player->X, Player->Y,
            Player->SizeX, Player->SizeY,
            1.0f, 0.8f, 0.5f, 1.0f
        );
    }

    {
        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Enemies); Index++)
        {
            enemy* Enemy = World->Enemies + Index;
            if (!Enemy->Live)
                continue;

            PushRect(
                Spec, Batch,
                Enemy->X, Enemy->Y,
                Enemy->SizeX, Enemy->SizeY,
                0.2f, 0.3f, 0.9f, 1.0f
            );
        }
    }
}

