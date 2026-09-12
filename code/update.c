
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
    v2      P;
    v2      DP;
    v2      Size;
    float   ShootCooldown;
} player;

typedef struct
{
    v2      ViewCenter;
    float   AspectRatio;
    float   FocalLength;
} camera;

typedef struct
{
    int     Live;
    int     ShotByEnemy;
    v2      P;
    v2      DP;
    v2      DDP;
    v2      Size;
} bullet;

typedef struct
{
    int     Live;
    v2      P;
    v2      DP;
    v2      Size;
    float   ShootCooldown;
} enemy;

typedef struct
{
    random_state    Entropy;
    player          Player;
    camera          Camera;
    bullet          Bullets[128];
    enemy           Enemies[128];
} world;

static v2 CameraGetViewSize(camera* Camera)
{
    v2 Result = V2(
        Camera->AspectRatio / Camera->FocalLength,
        1.0f                / Camera->FocalLength
    );

    return (Result);
}

static rect2 CameraGetViewRect(camera* Camera)
{
    v2 ViewCenter   = Camera->ViewCenter;
    v2 ViewSize     = CameraGetViewSize(Camera);
    rect2 ViewRect  = R2CenterSize(ViewCenter, ViewSize);

    return (ViewRect);
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

    Enemy->P = V2(
        7.0f*RandomBilateral(&World->Entropy),
        6.0f + 2.0f*RandomBilateral(&World->Entropy)
    );

    Enemy->Size = V2(0.4f, 0.4f);
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

        Camera->ViewCenter = V2Zero();
        Camera->FocalLength = 0.05f;
    }

    {
        player* Player = &World->Player;

        Player->P       = V2(0.0f, -6.5f);
        Player->Size    = V2(0.6f, 0.3f);
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

    Bullet->Size = V2(0.075f, 0.5f);
    Bullet->P = V2(
        Player->P.X,
        Player->P.Y + 0.5f*Player->Size.Y + Bullet->Size.Y
    );

    Bullet->DP = V2(0.0f, 10.0f);
    Bullet->DDP = V2(0.0f, 30.0f);

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

    Bullet->Size = V2(0.1f, 0.1f);
    Bullet->P = V2(
        Enemy->P.X,
        Enemy->P.Y - 0.5f*Enemy->Size.Y - Bullet->Size.Y
    );

    Bullet->DP = V2(0.0f, -5.0f);
    Bullet->DDP = V2(0.0f, -10.0f);

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
        rect2 ViewRect = CameraGetViewRect(Camera);

        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Bullets); Index++)
        {
            bullet* Bullet = World->Bullets + Index;
            if (!Bullet->Live)
                continue;

            rect2 BulletRect = R2CenterSize(Bullet->P, Bullet->Size);

            if (!R2Intersect(BulletRect, ViewRect))
                Bullet->Live = 0;
        }
    }

    {
        player* Player = &World->Player;

        Player->ShootCooldown -= Platform->DeltaTime;
        Player->ShootCooldown = Maximum(0, Player->ShootCooldown);

        if (IsInputButtonDown(Input, InputButton_Shoot))        PlayerTryShootBullet(World, Player);

        v2 MoveDirection = V2(0, 0);

        if (IsInputButtonDown(Input, InputButton_MoveLeft))     MoveDirection.X -= 1.0f;
        if (IsInputButtonDown(Input, InputButton_MoveRight))    MoveDirection.X += 1.0f;
        if (IsInputButtonDown(Input, InputButton_MoveDown))     MoveDirection.Y -= 1.0f;
        if (IsInputButtonDown(Input, InputButton_MoveUp))       MoveDirection.Y += 1.0f;

        MoveDirection = V2NormalizeOrZero(MoveDirection);

        float Friction  = 50.0f;
        float MoveForce = 400.0f;

        v2 DDP = V2Sub(
            V2MulScalar(MoveDirection,  MoveForce),
            V2MulScalar(Player->DP,     Friction)
        );

        Player->DP = V2Add(Player->DP, V2MulScalar(DDP,         Platform->DeltaTime));
        Player->P  = V2Add(Player->P,  V2MulScalar(Player->DP,  Platform->DeltaTime));
    }

    {
        for (unsigned int Index = 0; Index < ARRAY_COUNT(World->Enemies); Index++)
        {
            enemy* Enemy = World->Enemies + Index;
            if (!Enemy->Live)
                continue;

            Enemy->DP = V2(
                1.5f*RandomBilateral(&World->Entropy),
                1.5f*RandomBilateral(&World->Entropy)
            );

            Enemy->P = V2Add(Enemy->P, V2MulScalar(Enemy->DP, Platform->DeltaTime));

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
                v2 TargetP = World->Player.P;

                Bullet->DDP.X = 0.8f*(TargetP.X - Bullet->P.X);
            }

            Bullet->DP = V2Add(Bullet->DP, V2MulScalar(Bullet->DDP, Platform->DeltaTime));
            Bullet->P  = V2Add(Bullet->P,  V2MulScalar(Bullet->DP,  Platform->DeltaTime));
        }
    }
}

