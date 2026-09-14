
#pragma once

// NOTE(vak): Cheatsheet

static void GameSetup           (usize RandomSeed);
static void GameUpdateAndRender (f32 DeltaTime, u32 Width, u32 Height);

// NOTE(vak): Implementation

typedef struct
{
    v2      P;
    v2      DP;
    v2      Size;
    float   ShootTimer;
} player;

typedef struct
{
    b32 Alive;
    v2  P;
    v2  DP;
    v2  DDP;
    v2  Size;
} bullet;

typedef struct
{
    b32 Alive;
    v2  P;
    v2  DP;
    v2  DDP;
    v2  Size;
} enemy;

typedef struct
{
    random_state    Entropy;
    player          Player;
    bullet          Bullets[256];
    enemy           Enemies[256];
} game_state;

static game_state Game = {0};

static void GameSpawnBullet(v2 P, v2 DP, v2 DDP, v2 Size)
{
    bullet* Bullet = 0;

    for (usize Index = 0; Index < ArrayCount(Game.Bullets); Index++)
    {
        bullet* Slot = Game.Bullets + Index;
        if (!Slot->Alive)
        {
            Bullet = Slot;
            break;
        }
    }

    if (Bullet)
    {
        Bullet->Alive   = true;
        Bullet->P       = P;
        Bullet->DP      = DP;
        Bullet->DDP     = DDP;
        Bullet->Size    = Size;
    }
}

static void GameSpawnEnemy(v2 P, v2 DP, v2 DDP, v2 Size)
{
    enemy* Enemy = 0;

    for (usize Index = 0; Index < ArrayCount(Game.Enemies); Index++)
    {
        enemy* Slot = Game.Enemies + Index;
        if (!Slot->Alive)
        {
            Enemy = Slot;
            break;
        }
    }

    if (Enemy)
    {
        Enemy->Alive   = true;
        Enemy->P       = P;
        Enemy->DP      = DP;
        Enemy->DDP     = DDP;
        Enemy->Size    = Size;
    }
}

static void GamePlayerTryShoot(void)
{
    player* Player = &Game.Player;

    if (Player->ShootTimer > 0.0f)
        return;

    v2 BulletSize   = V2(0.2f, 0.2f);
    v2 SpawnP       = V2(Player->P.X, Player->P.Y + Player->Size.Y + 0.5f*BulletSize.Y);
    v2 SpawnDP      = V2(0.0f, Maximum(0.0f, Player->DP.Y) + 8.0f);
    v2 SpawnDDP     = V2(0.0f, 20.0f);

    GameSpawnBullet(SpawnP, SpawnDP, SpawnDDP, BulletSize);

    Player->ShootTimer = 0.2f;
}

static void GameSetup(usize RandomSeed)
{
    Game.Entropy.State = RandomSeed;

    player* Player = &Game.Player;

    Player->P       = V2(0.0f, -6.5f);
    Player->Size    = V2(0.6f, 0.3f);

    for (usize Index = 0; Index < 10; Index++)
    {
        v2 RandomP = V2(
            0.0f + 6.0f * RandomBilateral(&Game.Entropy),
            6.0f + 2.0f * RandomBilateral(&Game.Entropy)
        );

        v2 EnemySize = V2(0.6f, 0.4f);

        GameSpawnEnemy(RandomP, V2Zero(), V2Zero(), EnemySize);
    }
}

static void GameUpdateAndRender(f32 DeltaTime, u32 Width, u32 Height)
{
    f32 AspectRatio = (f32)Width / (f32)Height;
    f32 FocalLength = 0.05f;

    v2 ViewCenter = V2(0.0f, 0.0f);
    v2 ViewSize   = V2(
        AspectRatio / FocalLength,
        1.0f        / FocalLength
    );

    rect2 ViewRect = R2CenterSize(ViewCenter, ViewSize);

    RenderOrthographic2D(ViewRect);

    // NOTE(vak): Kill all bullets outside of view

    for (usize Index = 0; Index < ArrayCount(Game.Bullets); Index++)
    {
        bullet* Bullet = Game.Bullets + Index;

        if (!Bullet->Alive)
            continue;

        rect2 BulletRect = R2CenterSize(Bullet->P, Bullet->Size);
        if (!R2Intersects(BulletRect, ViewRect))
            Bullet->Alive = false;
    }

    // NOTE(vak): Update bullets

    for (usize Index = 0; Index < ArrayCount(Game.Bullets); Index++)
    {
        bullet* Bullet = Game.Bullets + Index;

        if (!Bullet->Alive)
            continue;

        Bullet->DP  = V2Add(Bullet->DP, V2MulScalar(Bullet->DDP,    DeltaTime));
        Bullet->P   = V2Add(Bullet->P,  V2MulScalar(Bullet->DP,     DeltaTime));
    }

    // NOTE(vak): Update enemies

    for (usize Index = 0; Index < ArrayCount(Game.Enemies); Index++)
    {
        enemy* Enemy = Game.Enemies + Index;

        if (!Enemy->Alive)
            continue;

        f32 JitterStrength = 1.25f;
        v2 JitterDirection = V2NormalizeOrZero(V2(
            RandomBilateral(&Game.Entropy),
            RandomBilateral(&Game.Entropy)
        ));

        Enemy->DP  = V2ScalarMul(JitterStrength, JitterDirection);
        Enemy->P   = V2Add(Enemy->P,  V2MulScalar(Enemy->DP, DeltaTime));
    }

    // NOTE(vak): Update player

    player* Player = &Game.Player;

    {
        Player->ShootTimer -= DeltaTime;
        Player->ShootTimer = Maximum(0, Player->ShootTimer);

        v2 MoveDirection = V2(0, 0);

        if (InputIsButtonDown(InputButton_Shoot))       GamePlayerTryShoot();

        if (InputIsButtonDown(InputButton_MoveLeft))    MoveDirection.X -= 1.0f;
        if (InputIsButtonDown(InputButton_MoveRight))   MoveDirection.X += 1.0f;
        if (InputIsButtonDown(InputButton_MoveDown))    MoveDirection.Y -= 1.0f;
        if (InputIsButtonDown(InputButton_MoveUp))      MoveDirection.Y += 1.0f;

        MoveDirection = V2NormalizeOrZero(MoveDirection);

        f32 Friction    = 25.0f;
        f32 MoveForce   = 200.0f;

        v2 PlayerDDP = V2Sub(
            V2ScalarMul(MoveForce, MoveDirection),
            V2ScalarMul(Friction,  Player->DP)
        );

        Player->DP  = V2Add(Player->DP, V2MulScalar(PlayerDDP,  DeltaTime));
        Player->P   = V2Add(Player->P,  V2MulScalar(Player->DP, DeltaTime));
    }

    // NOTE(vak): Render bullets

    for (usize Index = 0; Index < ArrayCount(Game.Bullets); Index++)
    {
        bullet* Bullet = Game.Bullets + Index;

        if (!Bullet->Alive)
            continue;

        RenderRect(R2CenterSize(Bullet->P, Bullet->Size), V4(1.0f, 0.4f, 0.2f, 1.0f));
    }

    // NOTE(vak): Render enemies

    for (usize Index = 0; Index < ArrayCount(Game.Enemies); Index++)
    {
        enemy* Enemy = Game.Enemies + Index;

        if (!Enemy->Alive)
            continue;

        RenderRect(R2CenterSize(Enemy->P, Enemy->Size), V4(0.3f, 0.4f, 0.9f, 1.0f));
    }

    // NOTE(vak): Render player

    RenderRect(R2CenterSize(Player->P, Player->Size), V4(1.0f, 0.8f, 0.5f, 1.0f));
}

