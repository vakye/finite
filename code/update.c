
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
    int PlayerShoot;

    float DeltaTime;
} input;

typedef struct
{
    float X, Y;
    float DX, DY;
    float SizeX, SizeY;
    float ShootCooldown;
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
    int   Live;
    float X, Y;
    float DX, DY;
    float DDX, DDY;
    float SizeX, SizeY;
} bullet;

typedef struct
{
    player Player;
    camera Camera;
    bullet Bullets[128];
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

static bullet* GrabDeadBulletSlot(world* World)
{
    bullet* Bullet = 0;

    for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Bullets); Index++)
    {
        bullet* Candidate = World->Bullets + Index;
        if (!Candidate->Live)
        {
            Bullet = Candidate;
            break;
        }
    }

    return (Bullet);
}

static void PlayerShootBullet(world* World, player* Player)
{
    if (Player->ShootCooldown > 0.0f)
        return;

    bullet* Bullet = GrabDeadBulletSlot(World);
    if (!Bullet)
        return;

    memset(Bullet, 0, sizeof(bullet));

    Bullet->Live = 1;

    Bullet->SizeX = 0.075f;
    Bullet->SizeY = 0.5f;

    Bullet->X = Player->X;
    Bullet->Y = Player->Y + 0.5f*Player->SizeY + Bullet->SizeY;

    Bullet->DX = 0.0f;
    Bullet->DY = 10.0f;
    Bullet->DDX = 0.0f;
    Bullet->DDY = 30.0f;

    Player->ShootCooldown = 0.12f;
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

    {
        camera* Camera = &World->Camera;

        float ViewSizeY = GetCameraViewSizeY(Camera);
        float ViewMaxY  = Camera->ViewCenterY + 0.5f*ViewSizeY;

        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Bullets); Index++)
        {
            bullet* Bullet = World->Bullets + Index;
            if (!Bullet->Live)
                continue;

            float BulletMinY = Bullet->Y - 0.5f*Bullet->SizeY;

            if (BulletMinY > ViewMaxY)
                Bullet->Live = 0;
        }
    }

    {
        player* Player = &World->Player;

        Player->ShootCooldown -= Input->DeltaTime;
        Player->ShootCooldown = Maximum(0, Player->ShootCooldown);

        if (Input->PlayerShoot)     PlayerShootBullet(World, Player);

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

        float Friction  = 0.3f;
        float MoveForce = 400.0f;

        float ImpulseX = (-Player->DX * Friction) + (DirectionX * MoveForce)*Input->DeltaTime;
        float ImpulseY = (-Player->DY * Friction) + (DirectionY * MoveForce)*Input->DeltaTime;

        Player->DX += ImpulseX;
        Player->DY += ImpulseY;
        Player->X += Player->DX * Input->DeltaTime;
        Player->Y += Player->DY * Input->DeltaTime;
    }

    {
        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Bullets); Index++)
        {
            bullet* Bullet = World->Bullets + Index;
            if (!Bullet->Live)
                continue;

            Bullet->DX += Bullet->DDX * Input->DeltaTime;
            Bullet->DY += Bullet->DDY * Input->DeltaTime;
            Bullet->X += Bullet->DX * Input->DeltaTime;
            Bullet->Y += Bullet->DY * Input->DeltaTime;
        }
    }
}

