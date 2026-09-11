
typedef struct
{
    float MouseX;
    float MouseY;

    float WindowSizeX;
    float WindowSizeY;

    int MovePlayerLeft;
    int MovePlayerRight;
    int MovePlayerUp;
    int MovePlayerDown;

    float DeltaTime;
} input;

typedef struct
{
    float X, Y;
    float DX, DY;
    float SizeX, SizeY;
} player;

typedef struct
{
    float ViewCenterX;
    float ViewCenterY;
    float AspectRatio;
    float FocalLength;
} camera;

typedef struct
{
    player Player;
    camera Camera;
} world;

static float GetCameraViewSizeX(camera* Camera)
{
    float Result = Camera->AspectRatio / Camera->FocalLength;
    return (Result);
}

static float GetCameraViewSizeY(camera* Camera)
{
    float Result = 1.0f / Camera->FocalLength;
    return (Result);
}

static float ToWorldX(input* Input, camera* Camera, float ScreenX)
{
    float Normalized = -1.0f + 2.0f*(ScreenX / Input->WindowSizeX);
    float Result = (GetCameraViewSizeX(Camera) * 0.5f * Normalized) - Camera->ViewCenterX;
    return (Result);
}

static float ToWorldY(input* Input, camera* Camera, float ScreenY)
{
    float Normalized = -1.0f + 2.0f*(ScreenY / Input->WindowSizeY);
    float Result = (GetCameraViewSizeY(Camera) * 0.5f * Normalized) - Camera->ViewCenterY;
    return (Result);
}

static void SetupWorld(world* World)
{
    memset(World, 0, sizeof(world));

    {
        camera* Camera = &World->Camera;

        Camera->ViewCenterX = 0.0f;
        Camera->ViewCenterY = 0.0f;
        Camera->FocalLength = 0.05f;
    }

    {
        player* Player = &World->Player;

        Player->X           = +0.0f;
        Player->Y           = -6.5f;
        Player->SizeX       = +1.2f;
        Player->SizeY       = +0.3f;
    }
}

static void UpdateWorld(
    input* Input,       // NOTE(vak): Input
    world* World        // NOTE(vak): Input/Output
)
{
    {
        camera* Camera = &World->Camera;
        Camera->AspectRatio = (float)Input->WindowSizeX / (float)Input->WindowSizeY;
    }

    float MouseWorldX = ToWorldX(Input, &World->Camera, Input->MouseX);
    float MouseWorldY = ToWorldY(Input, &World->Camera, Input->MouseY);

    {
        player* Player = &World->Player;

        float DirectionX = 0.0f;
        float DirectionY = 0.0f;

        if (Input->MovePlayerLeft)  DirectionX -= 1.0f;
        if (Input->MovePlayerRight) DirectionX += 1.0f;
        if (Input->MovePlayerDown)  DirectionY -= 1.0f;
        if (Input->MovePlayerUp)    DirectionY += 1.0f;

        if ((DirectionX != 0.0f) && (DirectionY != 0.0f))
        {
            DirectionX *= 0.7071067811865475244f; // NOTE(vak): 1.0 / sqrt(2)
            DirectionY *= 0.7071067811865475244f; // NOTE(vak): 1.0 / sqrt(2)
        }

        float Friction  = 0.12f;
        float MoveForce = 160.0f;

        float ImpulseX = (-Player->DX * Friction) + (DirectionX * MoveForce)*Input->DeltaTime;
        float ImpulseY = (-Player->DY * Friction) + (DirectionY * MoveForce)*Input->DeltaTime;

        Player->DX += ImpulseX;
        Player->DY += ImpulseY;
        Player->X += Player->DX * Input->DeltaTime;
        Player->Y += Player->DY * Input->DeltaTime;
    }
}

