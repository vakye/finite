
typedef struct
{
    unsigned long long State;
} random_state;

static unsigned int RotateRight32(unsigned int Value, unsigned int Shift)
{
	unsigned int Result = (Value >> Shift) | (Value << (-Shift & 31));
    return (Result);
}

static unsigned int RandomU32(random_state* Entropy)
{
    unsigned long long X = Entropy->State;
    unsigned int Count = (unsigned int)(X >> 59);

    Entropy->State = X * 6364136223846793005u + 1442695040888963407u;
    X ^= (X >> 18);

    unsigned int Result = RotateRight32((unsigned int)(X >> 27), Count);
    return (Result);
}

static float RandomUnilateral(random_state* Entropy)
{
    float Result = (float)RandomU32(Entropy) / (float)U32Max;
    return (Result);
}

static float RandomBilateral(random_state* Entropy)
{
    float Result = -1.0f + 2.0f*RandomUnilateral(Entropy);
    return (Result);
}

typedef enum
{
    InputButton_MoveLeft,
    InputButton_MoveRight,
    InputButton_MoveUp,
    InputButton_MoveDown,
    InputButton_Shoot,

    InputButton_COUNT,
} input_button;

typedef struct
{
    int IsDown;     // NOTE(vak): This frame
    int WasDown;    // NOTE(vak): Last frame
} input_button_state;

typedef struct
{
    input_button_state ButtonStates[InputButton_COUNT];
} platform_input;

static int IsInputButtonDown(platform_input* Input, input_button Button)
{
    return Input->ButtonStates[Button].IsDown;
}

typedef struct
{
    unsigned int    WindowSizeX;
    unsigned int    WindowSizeY;
    float           DeltaTime;
    platform_input  Input;
} platform;

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
    int   ShotByEnemy;
    float X, Y;
    float DX, DY;
    float DDX, DDY;
    float SizeX, SizeY;
} bullet;

typedef struct
{
    int   Live;
    float X, Y;
    float DX, DY;
    float SizeX, SizeY;
    float ShootCooldown;
} enemy;

typedef struct
{
    random_state    Entropy;
    player          Player;
    camera          Camera;
    bullet          Bullets[128];
    enemy           Enemies[128];
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

static float ToWorldX(platform* Platform, camera* Camera, float ScreenX)
{
    float Normalized = -1.0f + 2.0f*(ScreenX / Platform->WindowSizeX);
    float Result = (GetCameraViewSizeX(Camera) * 0.5f * Normalized) - Camera->ViewCenterX;
    return (Result);
}

static float ToWorldY(platform* Platform, camera* Camera, float ScreenY)
{
    float Normalized = -1.0f + 2.0f*(ScreenY / Platform->WindowSizeY);
    float Result = (GetCameraViewSizeY(Camera) * 0.5f * Normalized) - Camera->ViewCenterY;
    return (Result);
}

static enemy* GrabDeadEnemySlot(world* World)
{
    enemy* Enemy = 0;

    for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Enemies); Index++)
    {
        enemy* Candidate = World->Enemies + Index;
        if (!Candidate->Live)
        {
            Enemy = Candidate;
            break;
        }
    }

    return (Enemy);
}

static void SpawnEnemy(world* World)
{
    enemy* Enemy = GrabDeadEnemySlot(World);
    if (!Enemy)
        return;

    Enemy->Live = 1;
    Enemy->X = 7.0f*RandomBilateral(&World->Entropy);
    Enemy->Y = 6.0f + 2.0f*RandomBilateral(&World->Entropy);
    Enemy->SizeX = 0.4f;
    Enemy->SizeY = 0.4f;
    Enemy->ShootCooldown = 1.0f + 2.0f*RandomUnilateral(&World->Entropy);
}

static void SetupWorld(world* World)
{
    memset(World, 0, sizeof(world));

    {
        World->Entropy.State = __rdtsc();
    }

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
        Player->SizeX       = +0.6f;
        Player->SizeY       = +0.3f;
    }

    {
        for (unsigned int Index = 0; Index < 10; Index++)
            SpawnEnemy(World);
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

static void PlayerTryShootBullet(world* World, player* Player)
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

static void EnemyTryShootBullet(world* World, enemy* Enemy)
{
    if (Enemy->ShootCooldown > 0.0f)
        return;

    bullet* Bullet = GrabDeadBulletSlot(World);
    if (!Bullet)
        return;

    memset(Bullet, 0, sizeof(bullet));

    Bullet->Live = 1;
    Bullet->ShotByEnemy = 1;

    Bullet->SizeX = 0.1f;
    Bullet->SizeY = 0.1f;

    Bullet->X = Enemy->X;
    Bullet->Y = Enemy->Y - 0.5f*Enemy->SizeY - Bullet->SizeY;

    Bullet->DX = 0.0f;
    Bullet->DY = -5.0f;
    Bullet->DDX = 0.0f;
    Bullet->DDY = -10.0f;

    Enemy->ShootCooldown = 1.0f + 1.0f*RandomUnilateral(&World->Entropy);
}

static void UpdateWorld(
    platform*   Platform,   // NOTE(vak): Input
    world*      World       // NOTE(vak): Input/Output
)
{
    platform_input* Input = &Platform->Input;

    {
        camera* Camera = &World->Camera;
        Camera->AspectRatio = (float)Platform->WindowSizeX / (float)Platform->WindowSizeY;
    }

    {
        camera* Camera = &World->Camera;

        float ViewSizeX = GetCameraViewSizeX(Camera);
        float ViewSizeY = GetCameraViewSizeY(Camera);

        float ViewMinX  = Camera->ViewCenterX - 0.5f*ViewSizeX;
        float ViewMinY  = Camera->ViewCenterY - 0.5f*ViewSizeY;
        float ViewMaxX  = Camera->ViewCenterX + 0.5f*ViewSizeX;
        float ViewMaxY  = Camera->ViewCenterY + 0.5f*ViewSizeY;

        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Bullets); Index++)
        {
            bullet* Bullet = World->Bullets + Index;
            if (!Bullet->Live)
                continue;

            float BulletMinX  = Bullet->X - 0.5f*Bullet->SizeX;
            float BulletMinY  = Bullet->Y - 0.5f*Bullet->SizeY;
            float BulletMaxX  = Bullet->X + 0.5f*Bullet->SizeX;
            float BulletMaxY  = Bullet->Y + 0.5f*Bullet->SizeY;

            int OutsideScreen =
                (BulletMinY > ViewMaxY) ||
                (BulletMinX > ViewMaxX) ||
                (BulletMaxX < ViewMinX) ||
                (BulletMaxY < ViewMinY);

            if (OutsideScreen)
                Bullet->Live = 0;
        }
    }

    {
        player* Player = &World->Player;

        Player->ShootCooldown -= Platform->DeltaTime;
        Player->ShootCooldown = Maximum(0, Player->ShootCooldown);

        if (IsInputButtonDown(Input, InputButton_Shoot))        PlayerTryShootBullet(World, Player);

        float DirectionX = 0.0f;
        float DirectionY = 0.0f;

        if (IsInputButtonDown(Input, InputButton_MoveLeft))     DirectionX -= 1.0f;
        if (IsInputButtonDown(Input, InputButton_MoveRight))    DirectionX += 1.0f;
        if (IsInputButtonDown(Input, InputButton_MoveDown))     DirectionY -= 1.0f;
        if (IsInputButtonDown(Input, InputButton_MoveUp))       DirectionY += 1.0f;

        if ((DirectionX != 0.0f) && (DirectionY != 0.0f))
        {
            DirectionX *= 0.7071067811865475244f; // NOTE(vak): 1.0 / sqrt(2)
            DirectionY *= 0.7071067811865475244f; // NOTE(vak): 1.0 / sqrt(2)
        }

        float Friction  = 50.0f;
        float MoveForce = 400.0f;

        float DDX = (-Player->DX * Friction) + (DirectionX * MoveForce);
        float DDY = (-Player->DY * Friction) + (DirectionY * MoveForce);

        Player->DX += DDX * Platform->DeltaTime;
        Player->DY += DDY * Platform->DeltaTime;
        Player->X += Player->DX * Platform->DeltaTime;
        Player->Y += Player->DY * Platform->DeltaTime;
    }

    {
        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Enemies); Index++)
        {
            enemy* Enemy = World->Enemies + Index;
            if (!Enemy->Live)
                continue;

            Enemy->DX = 1.5f*RandomBilateral(&World->Entropy);
            Enemy->DY = 1.5f*RandomBilateral(&World->Entropy);

            Enemy->X += Enemy->DX * Platform->DeltaTime;
            Enemy->Y += Enemy->DY * Platform->DeltaTime;

            Enemy->ShootCooldown -= Platform->DeltaTime;
            Enemy->ShootCooldown = Maximum(0, Enemy->ShootCooldown);

            EnemyTryShootBullet(World, Enemy);
        }
    }

    {
        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Bullets); Index++)
        {
            bullet* Bullet = World->Bullets + Index;
            if (!Bullet->Live)
                continue;

            if (Bullet->ShotByEnemy)
            {
                float TargetX = World->Player.X;
                float TargetY = World->Player.Y;

                Bullet->DDX = 0.8f*(TargetX - Bullet->X);
            }

            Bullet->DX += Bullet->DDX * Platform->DeltaTime;
            Bullet->DY += Bullet->DDY * Platform->DeltaTime;
            Bullet->X += Bullet->DX * Platform->DeltaTime;
            Bullet->Y += Bullet->DY * Platform->DeltaTime;
        }
    }
}

