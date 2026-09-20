
#pragma once

// NOTE(vak): Cheatsheet

static void GameSetup           (usize RandomSeed);
static void GameUpdateAndRender (f32 DeltaTime, u32 Width, u32 Height);

// NOTE(vak): Implementation

typedef struct
{
    v2 P;
    f32 FocalLength;
    f32 AspectRatio;
    u32 WindowWidth;
    u32 WindowHeight;
} camera;

typedef struct
{
    b32 Alive;
    f32 SecondsRemaining;
} timer;

typedef struct
{
    b32 Alive;
    b32 ShotByPlayer;
    f32 Damage;
    v2 P;
    v2 DP;
    v2 DDP;
    v2 Size;
    v4 Color;
} bullet;

typedef struct
{
    v2 P;
    v2 DP;
    v2 DDP;
    v2 Size;
    v4 Color;
    timer* ShootTimer;
    f32 ShootCooldown;

    f32 LastHealth;
    f32 Health;
    f32 MaxHealth;

    f32 DamagedFadeRemaining;
    f32 DamagedFadeTime;
} player;

typedef struct
{
    b32 Alive;
    v2 P;
    v2 DP;
    v2 DDP;
    v2 Size;
    v4 Color;
    f32 TotalSeconds;
    f32 RemainingSeconds;
} particle;

typedef struct
{
    b32 Alive;
    v2 RestP;
    v2 P;
    v2 DP;
    v2 DDP;
    v2 Size;
    v4 Color;
    timer* ShootTimer;
    f32 ShootCooldown;

    f32 LastHealth;
    f32 Health;
    f32 MaxHealth;

    f32 DamagedFadeRemaining;
    f32 DamagedFadeTime;
} enemy;

typedef struct
{
    v2 P;
    v2 DP;
    v2 DDP;
    v4 Color;
    f32 Value;
    f32 TimeRemaining;
    f32 TotalTime;
} damage_text;

static random_state Entropy = {0};
static u32 Stage = 0;
static f32 StageFadeRemaining = 0.0f;
static f32 StageFadeTime = 0.0f;
static camera Camera = {0};
static player Player = {0};
static timer Timers[4096] = {0};
static bullet Bullets[4096] = {0};
static particle Particles[4096] = {0};
static enemy Enemies[4096] = {0};
static damage_text DamageTexts[512] = {0};

static timer* GameAddTimer(f32 InitialSecondsRemaining)
{
    timer* Timer = 0;

    for (usize Index = 0; Index < ArrayCount(Timers); Index++)
    {
        if (!Timers[Index].Alive)
        {
            Timer = Timers + Index;
            memset(Timer, 0, sizeof(timer));
            Timer->Alive = true;
            Timer->SecondsRemaining = InitialSecondsRemaining;
            break;
        }
    }

    return (Timer);
}

static void GameRemoveTimer(timer* Timer)
{
    if (!Timer) return;
    Timer->Alive = false;
}

static bullet* GameAddBullet(void)
{
    bullet* Bullet = 0;

    for (usize Index = 0; Index < ArrayCount(Bullets); Index++)
    {
        if (!Bullets[Index].Alive)
        {
            Bullet = Bullets + Index;
            memset(Bullet, 0, sizeof(bullet));
            Bullet->Alive = true;
            break;
        }
    }

    return (Bullet);
}

static void GameRemoveBullet(bullet* Bullet)
{
    if (!Bullet) return;
    Bullet->Alive = false;
}

static particle* GameAddParticle(void)
{
    particle* Particle = 0;

    for (usize Index = 0; Index < ArrayCount(Particles); Index++)
    {
        if (!Particles[Index].Alive)
        {
            Particle = Particles + Index;
            memset(Particle, 0, sizeof(particle));
            Particle->Alive = true;
            break;
        }
    }

    return (Particle);
}

static void GameRemoveParticle(particle* Particle)
{
    if (!Particle) return;
    Particle->Alive = false;
}

static enemy* GameAddEnemy(void)
{
    enemy* Enemy = 0;

    for (usize Index = 0; Index < ArrayCount(Enemies); Index++)
    {
        if (!Enemies[Index].Alive)
        {
            timer* ShootTimer = GameAddTimer(0.0f);
            if (!ShootTimer)
                return (0);

            Enemy = Enemies + Index;
            memset(Enemy, 0, sizeof(enemy));
            Enemy->Alive = true;
            Enemy->ShootTimer = ShootTimer;
            break;
        }
    }

    return (Enemy);
}

static void GameRemoveEnemy(enemy* Enemy)
{
    if (!Enemy) return;
    GameRemoveTimer(Enemy->ShootTimer);
    Enemy->Alive = false;
}

static damage_text* GameAddDamageText(void)
{
    damage_text* DamageText = 0;

    for (usize Index = 0; Index < ArrayCount(DamageTexts); Index++)
    {
        if (DamageTexts[Index].TimeRemaining <= 0.0f)
        {
            DamageText = DamageTexts + Index;
            memset(DamageText, 0, sizeof(damage_text));
            break;
        }
    }

    return (DamageText);
}

static void GameRemoveDamageText(damage_text* DamageText)
{
    if (!DamageText) return;
    DamageText->TimeRemaining = 0.0f;
}

static void GameSpawnEnemy(void)
{
    enemy* Enemy = GameAddEnemy();
    if (!Enemy)
        return;

    Enemy->RestP = V2(
        0.0f + 6.0f * RandomBilateral(&Entropy),
        7.0f + 2.0f * RandomBilateral(&Entropy)
    );

    Enemy->P = V2(
        0.0f  + 6.0f * RandomBilateral(&Entropy),
        16.0f + 4.0f * RandomBilateral(&Entropy)
    );

    Enemy->Size = V2(0.45f, 0.45f);
    Enemy->Color = V4(0.7f, 0.5f, 0.9f, 1.0f);

    Enemy->ShootCooldown = 3.5f;
    Enemy->ShootTimer->SecondsRemaining = Enemy->ShootCooldown * RandomUnilateral(&Entropy);

    Enemy->MaxHealth = 100.0f;
    Enemy->Health = Enemy->MaxHealth;
    Enemy->LastHealth = Enemy->MaxHealth;

    Enemy->DamagedFadeTime = 0.2f;
}

static void GamePlayerShoot(void)
{
    if (Player.ShootTimer->SecondsRemaining > 0.0f)
        return;

    bullet* Bullet = GameAddBullet();
    if (!Bullet)
        return;

    Bullet->ShotByPlayer = true;
    Bullet->Damage = 40.0f;
    Bullet->Size = V2(0.15f, 0.25f);
    Bullet->P = V2(Player.P.X, Player.P.Y + 0.5f*(Player.Size.Y + Bullet->Size.Y));
    Bullet->DP = V2(0.0f, Maximum(0, Player.DP.Y) + 16.0f);
    Bullet->DDP = V2(0.0f, 26.0f);
    Bullet->Color = V4(0.4f, 0.7f, 0.9f, 1.0f);

    u32 SparkCount = 4;
    for (u32 Index = 0; Index < SparkCount; Index++)
    {
        particle* Particle = GameAddParticle();

        Particle->TotalSeconds = 0.1f + 0.2f*RandomUnilateral(&Entropy);
        Particle->RemainingSeconds = Particle->TotalSeconds;

        Particle->P = Bullet->P;

        Particle->DP = V2(
            0.0f + 2.0f * RandomBilateral(&Entropy),
            2.0f + 3.0f * RandomUnilateral(&Entropy) + Maximum(0, Player.DP.Y)
        );

        Particle->DDP = V2(
            0.0f + 10.0f * RandomBilateral(&Entropy),
            5.0f + 10.0f * RandomUnilateral(&Entropy)
        );

        Particle->Size = V2Scalar(0.15f + 0.05f*RandomUnilateral(&Entropy));

        Particle->Color = V4(1.0f, 0.9f, 0.5f, 1.0f);
    }

    u32 SmokeCount = 8;
    for (u32 Index = 0; Index < SmokeCount; Index++)
    {
        particle* Particle = GameAddParticle();

        Particle->TotalSeconds = 0.2f + 0.2f*RandomUnilateral(&Entropy);
        Particle->RemainingSeconds = Particle->TotalSeconds;

        Particle->P = Bullet->P;

        Particle->DP = V2(
            0.0f + 3.0f * RandomBilateral(&Entropy),
            2.0f + 5.0f * RandomUnilateral(&Entropy) + Maximum(0, Player.DP.Y)
        );

        Particle->DDP = V2(
            0.0f + 5.0f * RandomBilateral(&Entropy),
            5.0f + 5.0f * RandomUnilateral(&Entropy)
        );

        Particle->Size = V2Scalar(0.05f + 0.1f*RandomUnilateral(&Entropy));

        Particle->Color = V4(0.6f, 0.6f, 0.6f, 1.0f);
    }

    Player.ShootTimer->SecondsRemaining = Player.ShootCooldown;
}

static void GameEnemyShoot(enemy* Enemy)
{
    if (Enemy->ShootTimer->SecondsRemaining > 0.0f)
        return;

    bullet* Bullet = GameAddBullet();
    if (!Bullet)
        return;

    Enemy->RestP = V2(
        0.0f + 6.0f * RandomBilateral(&Entropy),
        7.0f + 2.0f * RandomBilateral(&Entropy)
    );

    Bullet->ShotByPlayer = false;
    Bullet->Damage = 30.0f;
    Bullet->Size = V2(0.1f, 0.2f);
    Bullet->P = V2(Enemy->P.X, Enemy->P.Y - 0.5f*(Enemy->Size.Y + Bullet->Size.Y));
    Bullet->DP = V2(0.0f, Minimum(0, Enemy->DP.Y) - 8.0f);
    Bullet->DDP = V2(0.0f, -4.0f);
    Bullet->Color = V4(0.9f, 0.9f, 0.9f, 1.0f);

    u32 SparkCount = 4;
    for (u32 Index = 0; Index < SparkCount; Index++)
    {
        particle* Particle = GameAddParticle();

        Particle->TotalSeconds = 0.2f + 0.2f*RandomUnilateral(&Entropy);
        Particle->RemainingSeconds = Particle->TotalSeconds;

        Particle->P = Bullet->P;

        Particle->DP = V2(
            +0.0f + 2.0f * RandomBilateral(&Entropy),
            -2.0f - 3.0f * RandomUnilateral(&Entropy)
        );

        Particle->DDP = V2(
            +0.0f + 10.0f * RandomBilateral(&Entropy),
            -5.0f - 10.0f * RandomUnilateral(&Entropy)
        );

        Particle->Size = V2Scalar(0.05f + 0.05f*RandomUnilateral(&Entropy));

        Particle->Color = V4(0.9f, 0.8f, 0.5f, 1.0f);
    }

    u32 SmokeCount = 4;
    for (u32 Index = 0; Index < SmokeCount; Index++)
    {
        particle* Particle = GameAddParticle();

        Particle->TotalSeconds = 0.2f + 0.2f*RandomUnilateral(&Entropy);
        Particle->RemainingSeconds = Particle->TotalSeconds;

        Particle->P = Bullet->P;

        Particle->DP = V2(
            +0.0f + 3.0f * RandomBilateral(&Entropy),
            -2.0f - 5.0f * RandomUnilateral(&Entropy)
        );

        Particle->DDP = V2(
            +0.0f + 5.0f * RandomBilateral(&Entropy),
            -5.0f - 5.0f * RandomUnilateral(&Entropy)
        );

        Particle->Size = V2Scalar(0.05f + 0.1f*RandomUnilateral(&Entropy));

        Particle->Color = V4(0.6f, 0.6f, 0.6f, 1.0f);
    }

    Enemy->ShootTimer->SecondsRemaining = Enemy->ShootCooldown;
}

static void GameDoEnemyDeathParticles(enemy* Enemy)
{
    u32 Count = 8;
    for (u32 Index = 0; Index < Count; Index++)
    {
        particle* Particle = GameAddParticle();

        Particle->TotalSeconds = 0.2f + 0.1f*RandomUnilateral(&Entropy);
        Particle->RemainingSeconds = Particle->TotalSeconds;

        Particle->P = Enemy->P;

        Particle->DP = V2(
            7.0f*RandomBilateral(&Entropy),
            7.0f*RandomBilateral(&Entropy)
        );

        Particle->DDP = Particle->DP;

        Particle->Size = V2Scalar(0.5f + 0.2f*RandomUnilateral(&Entropy));

        Particle->Color = V4(0.9f, 0.9f, 0.9f, 1.0f);
    }
}

static void GameBulletHitPlayer(bullet* Bullet)
{
    f32 Knockback = 0.1f;
    Player.DP = V2Add(Player.DP, V2MulScalar(Bullet->DP, Knockback));

    Player.LastHealth = Player.Health;
    Player.Health -= Bullet->Damage;
    Player.DamagedFadeRemaining = Player.DamagedFadeTime;

    damage_text* DamageText = GameAddDamageText();

    DamageText->P = Player.P;
    DamageText->DP = V2(4.0f*RandomBilateral(&Entropy), 5.0f);
    DamageText->DDP = V2(DamageText->DP.X, -12.0f);
    DamageText->Value = Bullet->Damage;
    DamageText->Color = V4(1.0f, 0.5f, 0.5f, 1.0f);
    DamageText->TotalTime = 0.8f;
    DamageText->TimeRemaining = DamageText->TotalTime;

    u32 SplatCount = 8;
    for (u32 Index = 0; Index < SplatCount; Index++)
    {
        particle* Particle = GameAddParticle();

        Particle->TotalSeconds = 0.3f + 0.1f*RandomUnilateral(&Entropy);
        Particle->RemainingSeconds = Particle->TotalSeconds;

        Particle->P = Bullet->P;

        Particle->DP = V2(
            +0.0f + 4.0f*RandomBilateral(&Entropy),
            +1.0f + 1.0f*RandomUnilateral(&Entropy)
        );

        Particle->DDP = V2(
            +0.0f + 4.0f*RandomBilateral(&Entropy),
            +1.0f + 2.0f*RandomUnilateral(&Entropy)
        );

        Particle->Size = V2Scalar(0.1f + 0.3f*RandomUnilateral(&Entropy));

        Particle->Color = V4(1.0f, 1.0f, 1.0f, 0.8f);
    }

    GameRemoveBullet(Bullet);
}

static void GameBulletHitEnemy(enemy* Enemy, bullet* Bullet)
{
    f32 Knockback = 0.1f;
    Enemy->DP = V2Add(Enemy->DP, V2MulScalar(Bullet->DP, Knockback));

    Enemy->LastHealth = Enemy->Health;
    Enemy->Health -= Bullet->Damage;
    Enemy->DamagedFadeRemaining = Enemy->DamagedFadeTime;

    damage_text* DamageText = GameAddDamageText();

    DamageText->P = Enemy->P;
    DamageText->DP = V2(4.0f*RandomBilateral(&Entropy), 5.0f);
    DamageText->DDP = V2(DamageText->DP.X, -12.0f);
    DamageText->Color = V4(1.0f, 1.0f, 1.0f, 1.0f);
    DamageText->Value = Bullet->Damage;
    DamageText->TotalTime = 0.8f;
    DamageText->TimeRemaining = DamageText->TotalTime;

    u32 SplatCount = 8;
    for (u32 Index = 0; Index < SplatCount; Index++)
    {
        particle* Particle = GameAddParticle();

        Particle->TotalSeconds = 0.3f + 0.1f*RandomUnilateral(&Entropy);
        Particle->RemainingSeconds = Particle->TotalSeconds;

        Particle->P = Bullet->P;

        Particle->DP = V2(
            +0.0f + 4.0f*RandomBilateral(&Entropy),
            -1.0f - 1.0f*RandomUnilateral(&Entropy)
        );

        Particle->DDP = V2(
            +0.0f + 4.0f*RandomBilateral(&Entropy),
            -1.0f - 2.0f*RandomUnilateral(&Entropy)
        );

        Particle->Size = V2Scalar(0.1f + 0.3f*RandomUnilateral(&Entropy));

        Particle->Color = V4(1.0f, 1.0f, 1.0f, 0.8f);
    }

    if (Enemy->Health <= 0.0f)
    {
        GameDoEnemyDeathParticles(Enemy);
        GameRemoveEnemy(Enemy);
    }

    GameRemoveBullet(Bullet);
}

static b32 GameAreAllEnemiesDead(void)
{
    b32 AllDead = true;

    for (usize Index = 0; Index < ArrayCount(Enemies); Index++)
    {
        if (Enemies[Index].Alive)
        {
            AllDead = false;
            break;
        }
    }

    return (AllDead);
}

static void GameStartNewStage(void)
{
    u32 EnemyCount = 10 + Stage;
    for (u32 Index = 0; Index < EnemyCount; Index++)
        GameSpawnEnemy();

    Stage++;
    StageFadeTime = 2.75f;
    StageFadeRemaining = StageFadeTime;
}

static void GameRestart(void)
{
    for (usize Index = 0; Index < ArrayCount(Enemies); Index++)
        GameRemoveEnemy(Enemies + Index);

    for (usize Index = 0; Index < ArrayCount(Bullets); Index++)
        GameRemoveBullet(Bullets + Index);

    for (usize Index = 0; Index < ArrayCount(Particles); Index++)
        GameRemoveParticle(Particles + Index);

    for (usize Index = 0; Index < ArrayCount(DamageTexts); Index++)
        memset(DamageTexts + Index, 0, sizeof(damage_text));

    Player.P = V2(0.0f, -6.5f);
    Player.Size = V2(0.6f, 0.5f);
    Player.Color = V4(1.0f, 0.8f, 0.5f, 1.0f);
    Player.ShootTimer = GameAddTimer(0.0f);
    Player.ShootCooldown = 0.1f;
    Player.MaxHealth = 200.0f;
    Player.Health = Player.MaxHealth;

    Stage = 0;
    GameStartNewStage();
}

static void GameSetup(usize RandomSeed)
{
    Entropy.State = RandomSeed;
    GameRestart();
}

// NOTE(vak): Assumes screen origin (0, 0) at bottom-left
static v2 ConvertWorldToScreenP(v2 WorldP)
{
    v2 ViewSize = V2(
        Camera.AspectRatio / Camera.FocalLength,
        1.0f / Camera.FocalLength
    );

    rect2 ViewRect = R2CenterSize(Camera.P, ViewSize);

    v2 Normalized = V2Div(V2Sub(WorldP, ViewRect.Min), ViewSize);
    v2 ScreenP = V2Mul(Normalized, V2((f32)Camera.WindowWidth, (f32)Camera.WindowHeight));

    return (ScreenP);
}

// NOTE(vak): Assumes screen origin (0, 0) at bottom-left
static v2 ConvertScreenToWorldP(v2 ScreenP)
{
    v2 ViewSize = V2(
        Camera.AspectRatio / Camera.FocalLength,
        1.0f / Camera.FocalLength
    );

    rect2 ViewRect = R2CenterSize(Camera.P, ViewSize);

    v2 Normalized = V2Div(ScreenP, V2((f32)Camera.WindowWidth, (f32)Camera.WindowHeight));
    v2 WorldP = V2Add(ViewRect.Min, V2Mul(Normalized, ViewSize));

    return (WorldP);
}

static void GameUpdateAndRender(f32 DeltaTime, u32 Width, u32 Height)
{
    Camera.P = V2(0, 0);
    Camera.FocalLength = 0.05f;
    Camera.AspectRatio = (f32)Width / (f32)Height;
    Camera.WindowWidth = Width;
    Camera.WindowHeight = Height;

    v2 ViewSize = V2(
        Camera.AspectRatio / Camera.FocalLength,
        1.0f / Camera.FocalLength
    );

    rect2 ViewRect = R2CenterSize(Camera.P, ViewSize);

    RenderOrthographic2D(ViewRect);

    if (GameAreAllEnemiesDead())
        GameStartNewStage();

    if (Player.Health <= 0.0f)
        GameRestart();

    for (usize Index = 0; Index < ArrayCount(Timers); Index++)
    {
        if (!Timers[Index].Alive) continue;

        timer* Timer = Timers + Index;

        Timer->SecondsRemaining -= DeltaTime;
        Timer->SecondsRemaining = Maximum(0, Timer->SecondsRemaining);
    }

    for (usize Index = 0; Index < ArrayCount(DamageTexts); Index++)
    {
        damage_text* DamageText = DamageTexts + Index;
        DamageText->TimeRemaining -= DeltaTime;
        DamageText->TimeRemaining = Maximum(0, DamageText->TimeRemaining);

        DamageText->DP = V2Add(DamageText->DP, V2MulScalar(DamageText->DDP, DeltaTime));
        DamageText->P = V2Add(DamageText->P, V2MulScalar(DamageText->DP, DeltaTime));
    }

    for (usize Index = 0; Index < ArrayCount(Particles); Index++)
    {
        if (!Particles[Index].Alive) continue;

        particle* Particle = Particles + Index;

        Particle->RemainingSeconds -= DeltaTime;

        if (Particle->RemainingSeconds <= 0.0f)
        {
            GameRemoveParticle(Particle);
            continue;
        }

        Particle->DP = V2Add(Particle->DP, V2MulScalar(Particle->DDP, DeltaTime));
        Particle->P = V2Add(Particle->P, V2MulScalar(Particle->DP, DeltaTime));

        f32 T = Particle->RemainingSeconds / Particle->TotalSeconds;

        v4 FadedColor = V4Mul(Particle->Color, V4(1, 1, 1, T));

        RenderRect(R2CenterSize(Particle->P, Particle->Size), FadedColor);
    }

    for (usize Index = 0; Index < ArrayCount(Bullets); Index++)
    {
        if (!Bullets[Index].Alive) continue;

        bullet* Bullet = Bullets + Index;

        Bullet->DP = V2Add(Bullet->DP, V2MulScalar(Bullet->DDP, DeltaTime));
        Bullet->P = V2Add(Bullet->P, V2MulScalar(Bullet->DP, DeltaTime));

        rect2 BulletRect = R2CenterSize(Bullet->P, Bullet->Size);
        if (!R2Intersects(ViewRect, BulletRect))
        {
            GameRemoveBullet(Bullet);
            continue;
        }

        RenderRect(BulletRect, Bullet->Color);
    }

    for (usize EnemyIndex = 0; EnemyIndex < ArrayCount(Enemies); EnemyIndex++)
    {
        if (!Enemies[EnemyIndex].Alive) continue;
        enemy* Enemy = Enemies + EnemyIndex;

        rect2 EnemyRect = R2CenterSize(Enemy->P, Enemy->Size);

        for (usize BulletIndex = 0; BulletIndex < ArrayCount(Bullets); BulletIndex++)
        {
            if (!Bullets[BulletIndex].Alive) continue;
            if (!Bullets[BulletIndex].ShotByPlayer) continue;

            bullet* Bullet = Bullets + BulletIndex;

            rect2 BulletRect = R2CenterSize(Bullet->P, Bullet->Size);

            if (R2Intersects(EnemyRect, BulletRect))
                GameBulletHitEnemy(Enemy, Bullet);
        }
    }

    {
        rect2 PlayerRect = R2CenterSize(Player.P, Player.Size);

        for (usize BulletIndex = 0; BulletIndex < ArrayCount(Bullets); BulletIndex++)
        {
            if (!Bullets[BulletIndex].Alive) continue;
            if (Bullets[BulletIndex].ShotByPlayer) continue;

            bullet* Bullet = Bullets + BulletIndex;

            rect2 BulletRect = R2CenterSize(Bullet->P, Bullet->Size);

            if (R2Intersects(PlayerRect, BulletRect))
                GameBulletHitPlayer(Bullet);
        }
    }

    for (usize Index = 0; Index < ArrayCount(Enemies); Index++)
    {
        if (!Enemies[Index].Alive) continue;

        enemy* Enemy = Enemies + Index;

        GameEnemyShoot(Enemy);

        v2 Direction = V2NormalizeOrZero(V2Sub(Enemy->RestP, Enemy->P));
        f32 Friction = 2.5f;
        f32 Force = 10.0f;

        Enemy->DDP = V2Sub(
            V2ScalarMul(Force, Direction),
            V2ScalarMul(Friction, Enemy->DP)
        );

        f32 JitterStrength = 30.0f;

        v2 Jitter = V2(
            JitterStrength * RandomBilateral(&Entropy),
            JitterStrength * RandomBilateral(&Entropy)
        );

        Enemy->DDP = V2Add(Enemy->DDP, Jitter);

        Enemy->DP = V2Add(Enemy->DP, V2MulScalar(Enemy->DDP, DeltaTime));
        Enemy->P = V2Add(Enemy->P, V2MulScalar(Enemy->DP, DeltaTime));

        v4 DamagedColor = V4(1.0f, 0.3f, 0.3f, 1.0f);
        f32 DamagedT = Enemy->DamagedFadeRemaining / Enemy->DamagedFadeTime;
        f32 CurveT = -4.0f*Square(DamagedT) + 4.0f*DamagedT;

        v4 Color = V4Add(V4MulScalar(Enemy->Color, 1.0f-CurveT), V4MulScalar(DamagedColor, CurveT));

        rect2 EnemyRect = R2CenterSize(Enemy->P, Enemy->Size);
        v2 EnemyOutlineApron = V2Scalar(0.1f);
        v4 EnemyOutlineColor = V4(0.0f, 0.0f, 0.0f, 1.0f);

        RenderRect(R2Expand(EnemyRect, EnemyOutlineApron), EnemyOutlineColor);
        RenderRect(EnemyRect, Color);

        Enemy->DamagedFadeRemaining -= DeltaTime;
        Enemy->DamagedFadeRemaining = Maximum(0, Enemy->DamagedFadeRemaining);

        v4 HealthBarBorderColor = V4(0.0f, 0.0f, 0.0f, 1.0f);
        v4 HealthBarColor = V4(0.4f, 0.2f, 0.2f, 1.0f);
        v4 HealthColor = V4(0.9f, 0.3f, 0.4f, 1.0f);

        f32 HealthBarBorderApron = 0.05f;

        v2 HealthBarSize = V2(0.8f, 0.1f);

        v2 HealthBarMin = V2(
            Enemy->P.X - 0.5f*HealthBarSize.X,
            Enemy->P.Y + 0.5f*Enemy->Size.Y + 2.0f*HealthBarSize.Y + HealthBarBorderApron
        );

        v2 HealthBarMax = V2Add(HealthBarMin, HealthBarSize);

        f32 LastHealthPercent = Enemy->LastHealth / Enemy->MaxHealth;
        f32 CurHealthPercent = Enemy->Health / Enemy->MaxHealth;
        f32 HealthPercent = LastHealthPercent + (CurHealthPercent - LastHealthPercent)*Square(1.0f - DamagedT);

        v2 HealthMin = HealthBarMin;
        v2 HealthMax = V2Add(HealthMin, V2(HealthBarSize.X * HealthPercent, HealthBarSize.Y));

        v2 HealthBarBorderMin = V2Sub(HealthBarMin, V2Scalar(HealthBarBorderApron));
        v2 HealthBarBorderMax = V2Add(HealthBarMax, V2Scalar(HealthBarBorderApron));

        RenderRect(R2MinMax(HealthBarBorderMin, HealthBarBorderMax), HealthBarBorderColor);
        RenderRect(R2MinMax(HealthBarMin, HealthBarMax), HealthBarColor);
        RenderRect(R2MinMax(HealthMin, HealthMax), HealthColor);
    }

    {
        if (InputIsButtonDown(InputButton_Shoot))
            GamePlayerShoot();

        v2 Direction = V2(0, 0);

        Direction.X -= (f32)InputIsButtonDown(InputButton_MoveLeft);
        Direction.X += (f32)InputIsButtonDown(InputButton_MoveRight);
        Direction.Y -= (f32)InputIsButtonDown(InputButton_MoveDown);
        Direction.Y += (f32)InputIsButtonDown(InputButton_MoveUp);

        Direction = V2NormalizeOrZero(Direction);

        f32 Friction = 40.0f;
        f32 Force = 310.0f;

        Player.DDP = V2Sub(
            V2ScalarMul(Force, Direction),
            V2ScalarMul(Friction, Player.DP)
        );

        Player.DP = V2Add(Player.DP, V2MulScalar(Player.DDP, DeltaTime));
        Player.P = V2Add(Player.P, V2MulScalar(Player.DP, DeltaTime));

        rect2 PlayerRect = R2CenterSize(Player.P, Player.Size);
        v2 PlayerOutlineApron = V2Scalar(0.1f);
        v4 PlayerOutlineColor = V4(0.0f, 0.0f, 0.0f, 1.0f);

        RenderRect(R2Expand(PlayerRect, PlayerOutlineApron), PlayerOutlineColor);
        RenderRect(PlayerRect, Player.Color);

        v4 HealthBarBorderColor = V4(0.0f, 0.0f, 0.0f, 1.0f);
        v4 HealthBarColor = V4(0.4f, 0.2f, 0.2f, 1.0f);
        v4 HealthColor = V4(0.9f, 0.3f, 0.4f, 1.0f);

        f32 HealthBarBorderApron = 0.05f;

        v2 HealthBarSize = V2(1.0f, 0.1f);

        v2 HealthBarMin = V2(
            Player.P.X - 0.5f*HealthBarSize.X,
            Player.P.Y + 0.5f*Player.Size.Y + 2.0f*HealthBarSize.Y + HealthBarBorderApron
        );

        v2 HealthBarMax = V2Add(HealthBarMin, HealthBarSize);

        f32 HealthPercent = Player.Health / Player.MaxHealth;

        v2 HealthMin = HealthBarMin;
        v2 HealthMax = V2Add(HealthMin, V2(HealthBarSize.X * HealthPercent, HealthBarSize.Y));

        v2 HealthBarBorderMin = V2Sub(HealthBarMin, V2Scalar(HealthBarBorderApron));
        v2 HealthBarBorderMax = V2Add(HealthBarMax, V2Scalar(HealthBarBorderApron));

        RenderRect(R2MinMax(HealthBarBorderMin, HealthBarBorderMax), HealthBarBorderColor);
        RenderRect(R2MinMax(HealthBarMin, HealthBarMax), HealthBarColor);
        RenderRect(R2MinMax(HealthMin, HealthMax), HealthColor);
    }

    RenderOrthographic2D(R2MinMax(
        V2(0.0f, (f32)Height),
        V2((f32)Width, 0.0f)
    ));

    for (usize Index = 0; Index < ArrayCount(DamageTexts); Index++)
    {
        damage_text* DamageText = DamageTexts + Index;
        if (DamageText->TimeRemaining <= 0.0f) continue;

        v2 ScreenP = ConvertWorldToScreenP(DamageText->P);
        ScreenP.Y = (f32)Camera.WindowHeight - ScreenP.Y;

        char Buffer[64];
        u32 Count = snprintf(Buffer, sizeof(Buffer), "%.1f", DamageText->Value);

        f32 Alpha = DamageText->TimeRemaining / DamageText->TotalTime;
        v4 Tint = V4(1, 1, 1, Alpha);

        RenderText(StrData(Buffer, Count), ScreenP, V4Mul(DamageText->Color, Tint));
    }

    if (StageFadeRemaining > 0.0f)
    {
        char Buffer[64];
        u32 Count = snprintf(Buffer, sizeof(Buffer), "STAGE %u", Stage);

        string StageText = StrData(Buffer, Count);

        v2 StageFadeScreenP = V2(0.5f*Camera.WindowWidth, 0.75f*Camera.WindowHeight);
        StageFadeScreenP.Y = Camera.WindowHeight - StageFadeScreenP.Y;

        f32 T = StageFadeRemaining / StageFadeTime;
        f32 Alpha = 0.0f;

        if (T <= 0.10f)
            Alpha = Square(10.0f*T);
        else if (T <= 0.90f)
            Alpha = 1.0f;
        else
            Alpha = Square(10.0f - 10.0f*T);

        StageFadeScreenP = V2Sub(StageFadeScreenP, V2ScalarMul(0.5f, RenderGetTextSize(StageText)));

        RenderText(StageText, StageFadeScreenP, V4(0.9f, 0.9f, 0.9f, Alpha));

        StageFadeRemaining -= DeltaTime;
        StageFadeRemaining = Maximum(0, StageFadeRemaining);
    }
}

