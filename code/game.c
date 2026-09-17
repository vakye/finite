
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
    float   CurrentHealth;
    float   MaxHealth;
} player;

typedef enum
{
    BulletHitFlag_None      = (0),
    BulletHitFlag_Player    = (1 << 0),
    BulletHitFlag_Enemy     = (1 << 1),
} bullet_hit_flags;

typedef struct
{
    b32                 Alive;
    f32                 Damage;
    bullet_hit_flags    HitFlags;
    v2                  P;
    v2                  DP;
    v2                  DDP;
    v2                  Size;
    v4                  Color;
} bullet;

typedef enum
{
    EnemyKind_Grunt     = 0,
    EnemyKind_Mover,
    EnemyKind_Armored,
    EnemyKind_COUNT,
} enemy_kind;

typedef struct enemy enemy;

typedef void enemy_think(enemy* Enemy, f32 DeltaTime);

struct enemy
{
    enemy_kind  Kind;
    b32         Alive;
    v2          RestP;
    v2          P;
    v2          DP;
    v2          Size;
    f32         CurrentHealth;
    f32         ShootTimer;
    f32         MaxHealth;
    f32         ShootCooldown;

    union
    {
        struct { f32 ShuffleTimer; } Mover;
    };
};

typedef struct
{
    f32 Lifetime;
    f32 Remaining;
    v2  P;
    v2  DP;
    v2  DDP;
    v2  Size;
    v4  Color;
} particle;

typedef struct
{
    u32             Stage;
    random_state    Entropy;
    player          Player;
    bullet          Bullets[256];
    enemy           Enemies[256];
    particle        Particles[1024];
} game_state;

static game_state Game = {0};

static void GameSpawnBullet(f32 Damage, v2 P, v2 DP, v2 DDP, v2 Size, v4 Color, bullet_hit_flags HitFlags)
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
        Bullet->Alive       = true;
        Bullet->Damage      = Damage;
        Bullet->HitFlags    = HitFlags;
        Bullet->P           = P;
        Bullet->DP          = DP;
        Bullet->DDP         = DDP;
        Bullet->Size        = Size;
        Bullet->Color       = Color;
    }
}

static void GameSpawnEnemy(enemy_kind Kind, v2 RestP, v2 P, v2 DP, v2 Size, f32 MaxHealth, f32 ShootCooldown)
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
        memset(Enemy, 0, sizeof(enemy));

        Enemy->Kind             = Kind;
        Enemy->Alive            = true;
        Enemy->RestP            = RestP;
        Enemy->P                = P;
        Enemy->DP               = DP;
        Enemy->Size             = Size;
        Enemy->CurrentHealth    = MaxHealth;
        Enemy->ShootTimer       = ShootCooldown * RandomUnilateral(&Game.Entropy);
        Enemy->MaxHealth        = MaxHealth;
        Enemy->ShootCooldown    = ShootCooldown;
    }
}

static void GameEnemyMoverThink(enemy* Enemy, f32 DeltaTime)
{
    Enemy->Mover.ShuffleTimer -= DeltaTime;

    if (Enemy->Mover.ShuffleTimer <= 0.0f)
    {
        player* Player = &Game.Player;

        Enemy->RestP = V2(
            Player->P.X + 1.0f * RandomBilateral(&Game.Entropy),
            6.0f        + 2.0f * RandomBilateral(&Game.Entropy)
        );

        Enemy->Mover.ShuffleTimer = 1.2f;
    }
}

static void GameSpawnParticle(f32 Lifetime, v2 P, v2 DP, v2 DDP, v2 Size, v4 Color)
{
    particle* Particle = 0;

    for (usize Index = 0; Index < ArrayCount(Game.Particles); Index++)
    {
        particle* Slot = Game.Particles + Index;
        if (Slot->Remaining <= 0.0f)
        {
            Particle = Slot;
            break;
        }
    }

    if (Particle)
    {
        Particle->Lifetime  = Lifetime;
        Particle->Remaining = Lifetime;
        Particle->P         = P;
        Particle->DP        = DP;
        Particle->DDP       = DDP;
        Particle->Size      = Size;
        Particle->Color     = Color;
    }
}

static void GamePlayerTryShoot(void)
{
    player* Player = &Game.Player;

    if (Player->ShootTimer > 0.0f)
        return;

    bullet_hit_flags HitFlags = BulletHitFlag_Enemy;
    v2  BulletSize = V2(0.1f, 0.2f);
    f32 Damage = 20.0f;
    v2  SpawnP = V2(Player->P.X, Player->P.Y + Player->Size.Y + 0.5f*BulletSize.Y);
    v2  SpawnDP = V2(0.0f, Maximum(0.0f, Player->DP.Y) + 8.0f);
    v2  SpawnDDP = V2(0.0f, 20.0f);
    v4  BulletColor = V4(0.5f, 0.8f, 1.0f, 1.0f);

    v2 SpreadDP[3] =
    {
        V2(+0.0f, 0.0f),
        V2(-1.5f, 0.0f),
        V2(+1.5f, 0.0f),
    };

    GameSpawnBullet(Damage, SpawnP, V2Add(SpawnDP, SpreadDP[0]), SpawnDDP, BulletSize, BulletColor, HitFlags);
    GameSpawnBullet(Damage, SpawnP, V2Add(SpawnDP, SpreadDP[1]), SpawnDDP, BulletSize, BulletColor, HitFlags);
    GameSpawnBullet(Damage, SpawnP, V2Add(SpawnDP, SpreadDP[2]), SpawnDDP, BulletSize, BulletColor, HitFlags);

    Player->ShootTimer = 0.1f;

    usize SmokeParticleCount    = 8;
    usize FireParticleCount     = 4;

    for (usize Index = 0; Index < SmokeParticleCount; Index++)
    {
        float   SmokeLifetime   = 0.3f + 0.2f*RandomUnilateral(&Game.Entropy);
        v2      SmokeP          = SpawnP;
        v2      SmokeDP         = V2(2.0f*RandomBilateral(&Game.Entropy), 5.0f*RandomUnilateral(&Game.Entropy));
        v2      SmokeDDP        = SmokeDP;
        v2      SmokeSize       = V2Scalar(0.05f + 0.05f*RandomUnilateral(&Game.Entropy));
        v4      SmokeColor      = V4(0.3f, 0.3f, 0.3f, 1.0f);

        GameSpawnParticle(SmokeLifetime, SmokeP, SmokeDP, SmokeDDP, SmokeSize, SmokeColor);
    }

    for (usize Index = 0; Index < FireParticleCount; Index++)
    {
        float   FireLifetime   = 0.2f + 0.1f*RandomUnilateral(&Game.Entropy);
        v2      FireP          = SpawnP;
        v2      FireDP         = V2(2.0f*RandomBilateral(&Game.Entropy), 3.0f*RandomUnilateral(&Game.Entropy));
        v2      FireDDP        = FireDP;
        v2      FireSize       = V2Scalar(0.15f + 0.05f*RandomUnilateral(&Game.Entropy));
        v4      FireColor      = V4(0.8f, 0.7f, 0.3f, 1.0f);

        GameSpawnParticle(FireLifetime, FireP, FireDP, FireDDP, FireSize, FireColor);
    }
}

static void GameEnemyTryShoot(enemy* Enemy)
{
    if (Enemy->ShootTimer > 0.0f)
        return;

    bullet_hit_flags HitFlags = BulletHitFlag_Player;
    v2  BulletSize = V2(0.1f, 0.1f);
    f32 Damage = 20.0f;
    v2  SpawnP = V2(Enemy->P.X, Enemy->P.Y - Enemy->Size.Y - 0.5f*BulletSize.Y);
    v2  SpawnDP = V2(0.0f, -5.0f);
    v2  SpawnDDP = V2(0.0f, -10.0f);
    v4  BulletColor = V4(1.0f, 0.4f, 0.2f, 1.0f);

    GameSpawnBullet(Damage, SpawnP, SpawnDP, SpawnDDP, BulletSize, BulletColor, HitFlags);

    usize SmokeParticleCount    = 8;
    usize FireParticleCount     = 4;

    for (usize Index = 0; Index < SmokeParticleCount; Index++)
    {
        float   SmokeLifetime   = 0.3f + 0.2f*RandomUnilateral(&Game.Entropy);
        v2      SmokeP          = SpawnP;
        v2      SmokeDP         = V2(2.0f*RandomBilateral(&Game.Entropy), -5.0f*RandomUnilateral(&Game.Entropy));
        v2      SmokeDDP        = SmokeDP;
        v2      SmokeSize       = V2Scalar(0.05f + 0.05f*RandomUnilateral(&Game.Entropy));
        v4      SmokeColor      = V4(0.3f, 0.3f, 0.3f, 1.0f);

        GameSpawnParticle(SmokeLifetime, SmokeP, SmokeDP, SmokeDDP, SmokeSize, SmokeColor);
    }

    for (usize Index = 0; Index < FireParticleCount; Index++)
    {
        float   FireLifetime   = 0.2f + 0.1f*RandomUnilateral(&Game.Entropy);
        v2      FireP          = SpawnP;
        v2      FireDP         = V2(2.0f*RandomBilateral(&Game.Entropy), -3.0f*RandomUnilateral(&Game.Entropy));
        v2      FireDDP        = FireDP;
        v2      FireSize       = V2Scalar(0.1f + 0.05f*RandomUnilateral(&Game.Entropy));
        v4      FireColor      = V4(0.8f, 0.7f, 0.3f, 1.0f);

        GameSpawnParticle(FireLifetime, FireP, FireDP, FireDDP, FireSize, FireColor);
    }

    Enemy->ShootTimer = Enemy->ShootCooldown;
}

static b32 GameIsPlayerHitByBullet(bullet* Bullet)
{
    if ((Bullet->HitFlags & BulletHitFlag_Player) == 0)
        return (false);

    player* Player = &Game.Player;

    rect2 BulletRect = R2CenterSize(Bullet->P, Bullet->Size);
    rect2 PlayerRect = R2CenterSize(Player->P, Player->Size);

    b32 Result = R2Intersects(PlayerRect, BulletRect);
    return (Result);
}

static enemy* GameGetEnemyHitByBullet(bullet* Bullet)
{
    if ((Bullet->HitFlags & BulletHitFlag_Enemy) == 0)
        return (0);

    enemy* HitEnemy = 0;

    for (usize Index = 0; Index < ArrayCount(Game.Enemies); Index++)
    {
        enemy* Enemy = Game.Enemies + Index;

        if (!Enemy->Alive)
            continue;

        rect2 BulletRect = R2CenterSize(Bullet->P, Bullet->Size);
        rect2 EnemyRect  = R2CenterSize(Enemy->P,  Enemy->Size);

        if (R2Intersects(EnemyRect, BulletRect))
        {
            HitEnemy = Enemy;
            break;
        }
    }

    return (HitEnemy);
}

static b32 GameIsPlayerHittingEnemy(enemy* Enemy)
{
    player* Player = &Game.Player;

    rect2 EnemyRect  = R2CenterSize(Enemy->P,  Enemy->Size);
    rect2 PlayerRect = R2CenterSize(Player->P, Player->Size);

    b32 Result = R2Intersects(PlayerRect, EnemyRect);
    return (Result);
}

static b32 GameAreAllEnemiesDead(void)
{
    b32 AllDead = true;

    for (usize Index = 0; Index < ArrayCount(Game.Enemies); Index++)
    {
        enemy* Enemy = Game.Enemies + Index;

        if (Enemy->Alive)
        {
            AllDead = false;
            break;
        }
    }

    return (AllDead);
}

static void GameStartNewStage(void)
{
    usize GruntCount = 10 + 1*Game.Stage;

    for (usize Index = 0; Index < GruntCount; Index++)
    {
        v2 RandomRestP = V2(
            0.0f + 6.0f * RandomBilateral(&Game.Entropy),
            6.0f + 2.0f * RandomBilateral(&Game.Entropy)
        );

        v2 RandomP = V2(
            0.0f  + 6.0f * RandomBilateral(&Game.Entropy),
            20.0f + 2.0f * RandomBilateral(&Game.Entropy)
        );

        v2 EnemySize = V2(0.6f, 0.4f);

        f32 EnemyHealth = 100.0f;
        f32 ShootCooldown = 1.7f;

        GameSpawnEnemy(EnemyKind_Grunt, RandomRestP, RandomP, V2Zero(), EnemySize, EnemyHealth, ShootCooldown);
    }

    usize MoverCount = Game.Stage;

    for (usize Index = 0; Index < MoverCount; Index++)
    {
        v2 RandomRestP = V2(
            0.0f + 6.0f * RandomBilateral(&Game.Entropy),
            6.0f + 2.0f * RandomBilateral(&Game.Entropy)
        );

        v2 RandomP = V2(
            0.0f  + 6.0f * RandomBilateral(&Game.Entropy),
            20.0f + 2.0f * RandomBilateral(&Game.Entropy)
        );

        v2 EnemySize = V2(0.4f, 0.4f);

        f32 EnemyHealth = 120.0f;
        f32 ShootCooldown = 1.2f;

        GameSpawnEnemy(EnemyKind_Mover, RandomRestP, RandomP, V2Zero(), EnemySize, EnemyHealth, ShootCooldown);
    }

    usize ArmoredCount = Maximum(0, (ssize)Game.Stage - 1);

    for (usize Index = 0; Index < ArmoredCount; Index++)
    {
        v2 RandomRestP = V2(
            0.0f + 6.0f * RandomBilateral(&Game.Entropy),
            6.0f + 2.0f * RandomBilateral(&Game.Entropy)
        );

        v2 RandomP = V2(
            0.0f  + 6.0f * RandomBilateral(&Game.Entropy),
            20.0f + 2.0f * RandomBilateral(&Game.Entropy)
        );

        v2 EnemySize = V2(0.8f, 0.6f);

        f32 EnemyHealth = 300.0f;
        f32 ShootCooldown = 2.1f;

        GameSpawnEnemy(EnemyKind_Armored, RandomRestP, RandomP, V2Zero(), EnemySize, EnemyHealth, ShootCooldown);
    }

    Game.Stage++;
}

static void GameRestart(void)
{
    usize SavedEntropy = Game.Entropy.State;

    memset(&Game, 0, sizeof(game_state));

    Game.Entropy.State = SavedEntropy;

    player* Player = &Game.Player;

    Player->P = V2(0.0f, -6.5f);
    Player->Size = V2(0.6f, 0.3f);
    Player->MaxHealth = 120.0f;
    Player->CurrentHealth = Player->MaxHealth;

    GameStartNewStage();
}

static void GameSetup(usize RandomSeed)
{
    Game.Entropy.State = RandomSeed;
    GameRestart();
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

    player* Player = &Game.Player;

    // NOTE(vak): Restart game if player is dead

    if (Player->CurrentHealth <= 0.0f)
        GameRestart();

    // NOTE(vak): Start new stage if all enemies are dead

    if (GameAreAllEnemiesDead())
        GameStartNewStage();

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

    // NOTE(vak): Update particles

    for (usize Index = 0; Index < ArrayCount(Game.Particles); Index++)
    {
        particle* Particle = Game.Particles + Index;

        if (Particle->Remaining <= 0.0f)
            continue;

        Particle->Remaining -= DeltaTime;
        Particle->Remaining = Maximum(Particle->Remaining, 0.0f);

        Particle->DP = V2Add(Particle->DP, V2MulScalar(Particle->DDP, DeltaTime));
        Particle->P  = V2Add(Particle->P,  V2MulScalar(Particle->DP,  DeltaTime));
    }

    // NOTE(vak): Update bullets

    for (usize Index = 0; Index < ArrayCount(Game.Bullets); Index++)
    {
        bullet* Bullet = Game.Bullets + Index;

        if (!Bullet->Alive)
            continue;

        Bullet->DP  = V2Add(Bullet->DP, V2MulScalar(Bullet->DDP,    DeltaTime));
        Bullet->P   = V2Add(Bullet->P,  V2MulScalar(Bullet->DP,     DeltaTime));

        enemy* Enemy = GameGetEnemyHitByBullet(Bullet);
        if (Enemy)
        {
            Enemy->CurrentHealth -= Bullet->Damage;
            Enemy->Alive = (Enemy->CurrentHealth > 0.0f);
            Bullet->Alive = false;

            v2 KnockbackDirection = V2NormalizeOrZero(V2Add(Bullet->DP, V2Sub(Enemy->P, Bullet->P)));
            f32 KnockbackStrength = 5.0f;

            Enemy->DP = V2Add(Enemy->DP, V2MulScalar(KnockbackDirection, KnockbackStrength));

            usize HitParticleCount = 8;

            for (usize Index = 0; Index < HitParticleCount; Index++)
            {
                float   ParticleLifetime   = 0.3f + 0.2f*RandomUnilateral(&Game.Entropy);
                v2      ParticleP          = Bullet->P;
                v2      ParticleDP         = V2(2.0f*RandomBilateral(&Game.Entropy), 2.0f*RandomBilateral(&Game.Entropy));
                v2      ParticleDDP        = ParticleDP;
                v2      ParticleSize       = V2Scalar(0.15f + 0.1f*RandomUnilateral(&Game.Entropy));
                v4      ParticleColor      = V4(0.8f, 0.8f, 0.8f, 0.8f);

                GameSpawnParticle(ParticleLifetime, ParticleP, ParticleDP, ParticleDDP, ParticleSize, ParticleColor);
            }
        }

        if (GameIsPlayerHitByBullet(Bullet))
        {
            Player->CurrentHealth -= Bullet->Damage;
            Player->CurrentHealth = Maximum(0.0f, Player->CurrentHealth);
            Bullet->Alive = false;

            usize HitParticleCount = 8;

            for (usize Index = 0; Index < HitParticleCount; Index++)
            {
                float   ParticleLifetime   = 0.3f + 0.2f*RandomUnilateral(&Game.Entropy);
                v2      ParticleP          = Bullet->P;
                v2      ParticleDP         = V2(2.0f*RandomBilateral(&Game.Entropy), 2.0f*RandomBilateral(&Game.Entropy));
                v2      ParticleDDP        = ParticleDP;
                v2      ParticleSize       = V2Scalar(0.15f + 0.1f*RandomUnilateral(&Game.Entropy));
                v4      ParticleColor      = V4(0.9f, 0.8f, 0.7f, 0.8f);

                GameSpawnParticle(ParticleLifetime, ParticleP, ParticleDP, ParticleDDP, ParticleSize, ParticleColor);
            }
        }
    }

    // NOTE(vak): Update enemies

    for (usize Index = 0; Index < ArrayCount(Game.Enemies); Index++)
    {
        enemy* Enemy = Game.Enemies + Index;

        if (!Enemy->Alive)
            continue;

        if (GameIsPlayerHittingEnemy(Enemy))
        {
            float PlayerHealth = Maximum(0, Player->CurrentHealth);

            Player->CurrentHealth -= Maximum(0, Enemy->CurrentHealth);
            Enemy->CurrentHealth  -= PlayerHealth;
            Enemy->Alive           = (Enemy->CurrentHealth >= 0.0f);
        }

        static enemy_think* ThinkFor[EnemyKind_COUNT] =
        {
            [EnemyKind_Grunt]   = 0,
            [EnemyKind_Mover]   = GameEnemyMoverThink,
            [EnemyKind_Armored] = 0,
        };

        if (ThinkFor[Enemy->Kind])
        {
            ThinkFor[Enemy->Kind](Enemy, DeltaTime);
        }

        Enemy->ShootTimer -= DeltaTime;
        Enemy->ShootTimer = Maximum(0.0f, Enemy->ShootTimer);

        GameEnemyTryShoot(Enemy);

        v2  MoveDirection   = V2NormalizeOrZero(V2Sub(Enemy->RestP, Enemy->P));
        f32 MoveFriction    = 7.0f;
        f32 MoveForce       = 25.0f;

        v2 EnemyDDP = V2Sub(
            V2ScalarMul(MoveForce,      MoveDirection),
            V2ScalarMul(MoveFriction,   Enemy->DP)
        );

        f32 JitterStrength = 0.3f;

        v2 JitterDirection = V2NormalizeOrZero(V2(
            RandomBilateral(&Game.Entropy),
            RandomBilateral(&Game.Entropy)
        ));

        Enemy->DP = V2Add(Enemy->DP, V2Add(
            V2MulScalar(EnemyDDP, DeltaTime),
            V2MulScalar(JitterDirection, JitterStrength)
        ));

        Enemy->P = V2Add(Enemy->P,  V2MulScalar(Enemy->DP, DeltaTime));
    }

    // NOTE(vak): Update player

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

        f32 MoveFriction    = 25.0f;
        f32 MoveForce       = 180.0f;

        v2 PlayerDDP = V2Sub(
            V2ScalarMul(MoveForce,      MoveDirection),
            V2ScalarMul(MoveFriction,   Player->DP)
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

        RenderRect(R2CenterSize(Bullet->P, Bullet->Size), Bullet->Color);
    }

    // NOTE(vak): Render enemies

    for (usize Index = 0; Index < ArrayCount(Game.Enemies); Index++)
    {
        enemy* Enemy = Game.Enemies + Index;

        if (!Enemy->Alive)
            continue;

        static v4 EnemyColors[EnemyKind_COUNT] =
        {
            [EnemyKind_Grunt]   = {.E = {0.3f, 0.4f, 0.9f, 1.0f}},
            [EnemyKind_Mover]   = {.E = {0.9f, 0.9f, 0.2f, 1.0f}},
            [EnemyKind_Armored] = {.E = {0.8f, 0.8f, 0.8f, 1.0f}},
        };

        RenderRect(R2CenterSize(Enemy->P, Enemy->Size), EnemyColors[Enemy->Kind]);

        v2 HealthBarSize = V2(0.75f, 0.1f);

        v2 HealthBarMin = V2(
            Enemy->P.X - 0.5f*HealthBarSize.X,
            Enemy->P.Y + 1.0f*Enemy->Size.Y
        );

        f32 HealthPercent = Enemy->CurrentHealth / Enemy->MaxHealth;

        v2 HealthSize = V2(HealthBarSize.X * HealthPercent, HealthBarSize.Y);

        RenderRect(R2MinSize(HealthBarMin, HealthBarSize),  V4(0.6f, 0.2f, 0.1f, 1.0f));
        RenderRect(R2MinSize(HealthBarMin, HealthSize),     V4(0.9f, 0.2f, 0.1f, 1.0f));
    }

    // NOTE(vak): Render player

    {
        RenderRect(R2CenterSize(Player->P, Player->Size), V4(1.0f, 0.8f, 0.5f, 1.0f));

        v2 HealthBarSize = V2(0.75f, 0.1f);

        v2 HealthBarMin = V2(
            Player->P.X - 0.5f*HealthBarSize.X,
            Player->P.Y + 1.0f*Player->Size.Y
        );

        f32 HealthPercent = Player->CurrentHealth / Player->MaxHealth;

        v2 HealthSize = V2(HealthBarSize.X * HealthPercent, HealthBarSize.Y);

        RenderRect(R2MinSize(HealthBarMin, HealthBarSize),  V4(0.4f, 0.4f, 0.6f, 1.0f));
        RenderRect(R2MinSize(HealthBarMin, HealthSize),     V4(0.2f, 0.5f, 0.9f, 1.0f));
    }

    // NOTE(vak): Render particles

    for (usize Index = 0; Index < ArrayCount(Game.Particles); Index++)
    {
        particle* Particle = Game.Particles + Index;

        if (Particle->Remaining <= 0.0f)
            continue;

        v4 AlphaScale = V4(1.0f, 1.0f, 1.0f, Particle->Remaining / Particle->Lifetime);
        v4 Color = V4Mul(Particle->Color, AlphaScale);

        RenderRect(R2MinSize(Particle->P, Particle->Size), Color);
    }

    // NOTE(vak): Test text rendering

    {
        RenderOrthographic2D(R2MinMax(
            V2(0.0f, (f32)Height),
            V2((f32)Width, 0.0f)
        ));

        v4 Colors[4] =
        {
            {.E = {1.0f, 1.0f, 1.0f, 1.0f}},
            {.E = {1.0f, 0.0f, 0.0f, 1.0f}},
            {.E = {0.0f, 1.0f, 0.0f, 1.0f}},
            {.E = {0.0f, 0.0f, 1.0f, 1.0f}},
        };

        v2 Position = V2(0, 0);
        string Text = Str("The quick brown fox jumps over the lazy dog.");

        for (usize Index = 0; Index < ArrayCount(Colors); Index++)
        {
            RenderText(Text, Position, Colors[Index]);

            Position.Y += RenderGetTextSizeY(Text);
        }

        {
            char BufferTextFPS[64] = {0};
            usize CountTextFPS = snprintf(BufferTextFPS, sizeof(BufferTextFPS), "FPS: %f", 1.0f/DeltaTime);

            string TextFPS = StrData(BufferTextFPS, CountTextFPS);

            RenderText(TextFPS, Position, V4(1.0f, 1.0f, 1.0f, 1.0f));
            Position.Y += RenderGetTextSizeY(TextFPS);
        }

        {
            char BufferTextStage[64] = {0};
            usize CountTextStage = snprintf(BufferTextStage, sizeof(BufferTextStage), "Stage: %u", Game.Stage);

            string TextStage = StrData(BufferTextStage, CountTextStage);

            RenderText(TextStage, Position, V4(1.0f, 1.0f, 1.0f, 1.0f));
            Position.Y += RenderGetTextSizeY(TextStage);
        }
    }
}

