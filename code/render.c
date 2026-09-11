
typedef struct
{
    float MinX, MinY;
    float MaxX, MaxY;
    float R, G, B, A;
} render_rect;

typedef struct
{
    // NOTE(vak): Platform sets these for Render()

    unsigned int    RenderSizeX;        // NOTE(vak): Render target width
    unsigned int    RenderSizeY;        // NOTE(vak): Render target height
    unsigned int    MaxRectCount;

    // NOTE(vak): Render() function sets these for
    // the platform

    float           Projection[16];     // NOTE(vak): Reset to identity matrix before Render() by platform
    unsigned int    RectCount;          // NOTE(vak): Reset to 0 before Render() by platform
    render_rect*    Rects;              // NOTE(vak): Allocated by platform
} render_batch;

static void Orthographic2D(
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
    render_batch* Batch,
    float MinX, float MinY,
    float MaxX, float MaxY,
    float R, float G, float B, float A
)
{
    assert(Batch->RectCount < Batch->MaxRectCount);

    render_rect* Rect = Batch->Rects + Batch->RectCount++;

    Rect->MinX = MinX;
    Rect->MinY = MinY;

    Rect->MaxX = MaxX;
    Rect->MaxY = MaxY;

    Rect->R = R;
    Rect->G = G;
    Rect->B = B;
    Rect->A = A;
}

static void Render(render_batch* Batch)
{
    float AspectRatio = (float)Batch->RenderSizeX / (float)Batch->RenderSizeY;
    float FocalLength = 0.05f;

    float ViewSizeY = 1.0f / FocalLength;
    float ViewSizeX = AspectRatio * ViewSizeY;
    float ViewCenterX = 0.0f;
    float ViewCenterY = 0.0f;

    float ViewMinX = ViewCenterX - 0.5f*ViewSizeX;
    float ViewMinY = ViewCenterY - 0.5f*ViewSizeY;
    float ViewMaxX = ViewCenterX + 0.5f*ViewSizeX;
    float ViewMaxY = ViewCenterY + 0.5f*ViewSizeY;

    Orthographic2D(Batch, ViewMinX, ViewMinY, ViewMaxX, ViewMaxY);

    PushRect(
        Batch,
        -1.0f, -1.0f,
        +1.0f, +1.0f,
        1.0f, 0.8f, 0.5f, 1.0f
    );
}

