
#pragma once

// NOTE(vak): Cheatsheet

static void GameSetup           (usize RandomSeed);
static void GameUpdateAndRender (f32 DeltaTime, u32 Width, u32 Height);

// NOTE(vak): Implementation

#if 1

#define CollisionMaskFlag_Player        (1ull << 0)
#define CollisionMaskFlag_Enemy         (1ull << 1)
#define CollisionMaskFlag_All           (U64Max)

typedef struct
{
    u32             Stage;      // NOTE(vak): Current stage
    random_state    Entropy;    // NOTE(vak): Random number generator state
    rect2           ViewRect;   // NOTE(vak): Camera view rectangle
    entity_id       PlayerID;   // NOTE(vak): Entity ID of the player
    u32             EnemyCount; // NOTE(vak): Remaining enemies still alive
    f32             DeltaTime;  // NOTE(vak): This frame's delta time
} game_state;

static game_state Game = {0};

static weapon GameWeaponRandomCooldown(weapon_kind Kind)
{
    weapon_stats* Stats = GetWeaponStats(Kind);

    f32 MaxCooldown = SafeDivide0(1.0f, Stats->FireRate);

    weapon Result = {.Kind = Kind, .Cooldown = MaxCooldown * RandomUnilateral(&Game.Entropy)};
    return (Result);
}

static v2 GameRandomEnemySpawnP(void)
{
    v2 Result = V2(
        0.0f  + 7.0f * RandomBilateral(&Game.Entropy),
        16.0f + 2.0f * RandomBilateral(&Game.Entropy)
    );

    return (Result);
}

static v2 GameRandomEnemyRestP(void)
{
    v2 Result = V2(
        0.0f  + 7.0f * RandomBilateral (&Game.Entropy),
        4.0f  + 4.0f * RandomUnilateral(&Game.Entropy)
    );

    return (Result);
}

static void GameSpawnEnemyGrunt(void)
{
    entity_id       EntityID    = AddEntity(EntityKind_EnemyGrunt);
    entity*         Entity      = GetEntity(EntityID);
    body*           Body        = GetBody(Entity->BodyID);
    enemy_grunt*    Grunt       = &Entity->Grunt;

    // NOTE(vak): Grunt
    //      + Health:       Medium
    //      + Speed:        None
    //      + Fire rate:    Medium
    //      + Damage:       Medium

    SetBodyP(Entity->BodyID, GameRandomEnemySpawnP());

    Entity->Color           = V4(0.3f, 0.4f, 0.9f, 1.0f);
    Grunt->MaxHealth        = 120.0f;
    Grunt->Weapon           = GameWeaponRandomCooldown(WeaponKind_GruntPistol);

    Body->Size              = V2(0.6f, 0.4f);
    Body->CollisionMask     = CollisionMaskFlag_Enemy;
    Grunt->CurrentHealth    = Grunt->MaxHealth;
    Grunt->RestP            = GameRandomEnemyRestP();
}

static void GameSpawnEnemyMover(void)
{
    entity_id       EntityID    = AddEntity(EntityKind_EnemyMover);
    entity*         Entity      = GetEntity(EntityID);
    body*           Body        = GetBody(Entity->BodyID);
    enemy_mover*    Mover       = &Entity->Mover;

    // NOTE(vak): Mover
    //      + Health:       Low
    //      + Speed:        High
    //      + Fire rate:    High
    //      + Damage:       Low

    SetBodyP(Entity->BodyID, GameRandomEnemySpawnP());

    Entity->Color           = V4(0.9f, 0.9f, 0.4f, 1.0f);
    Mover->ShuffleCooldown  = 0.5f;
    Mover->Weapon           = GameWeaponRandomCooldown(WeaponKind_MoverPistol);

    Body->Size              = V2(0.4f, 0.4f);
    Body->CollisionMask     = CollisionMaskFlag_Enemy;
    Mover->CurrentHealth    = Mover->MaxHealth;
    Mover->ShuffleTimer     = Mover->ShuffleCooldown * RandomUnilateral(&Game.Entropy);
    Mover->RestP            = GameRandomEnemyRestP();
}

static void GameSpawnEnemyArmored(void)
{
    entity_id       EntityID    = AddEntity(EntityKind_EnemyArmored);
    entity*         Entity      = GetEntity(EntityID);
    body*           Body        = GetBody(Entity->BodyID);
    enemy_armored*  Armored     = &Entity->Armored;

    // NOTE(vak): Armored
    //      + Health:       High
    //      + Speed:        Low
    //      + Fire rate:    Low
    //      + Damage:       High

    SetBodyP(Entity->BodyID, GameRandomEnemySpawnP());

    Entity->Color               = V4(0.5f, 0.5f, 0.5f, 1.0f);
    Armored->ShuffleCooldown    = 5.3f;
    Armored->MaxHealth          = 300.0f;
    Armored->Weapon             = GameWeaponRandomCooldown(WeaponKind_ArmoredRevolver);

    Body->Size                  = V2(0.8f, 0.8f);
    Body->CollisionMask     = CollisionMaskFlag_Enemy;
    Armored->CurrentHealth      = Armored->MaxHealth;
    Armored->ShuffleTimer       = Armored->ShuffleCooldown * RandomUnilateral(&Game.Entropy);
    Armored->RestP              = GameRandomEnemyRestP();
}

static entity_id GameSpawnPlayer(void)
{
    entity_id       EntityID    = AddEntity(EntityKind_Player);
    entity*         Entity      = GetEntity(EntityID);
    body*           Body        = GetBody(Entity->BodyID);
    player*         Player      = &Entity->Player;

    SetBodyP(Entity->BodyID, V2(0.0f, -6.5f));

    Entity->Color           = V4(1.0f, 0.8f, 0.5f, 1.0f);
    Body->Size              = V2(0.45f, 0.45f);
    Body->CollisionMask     = CollisionMaskFlag_Player;
    Player->MaxHealth       = 200.0f;
    Player->CurrentHealth   = Player->MaxHealth;
    Player->Weapon          = (weapon){.Kind = WeaponKind_PlayerPewPew};

    return (EntityID);
}

static void GameStartNewStage(void)
{
    Game.Stage++;

    u32 MaxGruntCount   = 20;
    u32 MaxMoverCount   = 10;
    u32 MaxArmoredCount = 5;

    u32 GruntCount      = 10 + (Game.Stage - 1);
    u32 MoverCount      = 0;
    u32 ArmoredCount    = 0;

    if (Game.Stage >= 2)
        MoverCount = 1 + (Game.Stage - 2);

    if (Game.Stage >= 3)
        ArmoredCount = 1 + (Game.Stage - 3);

    GruntCount      = Minimum(GruntCount,   MaxGruntCount);
    MoverCount      = Minimum(MoverCount,   MaxMoverCount);
    ArmoredCount    = Minimum(ArmoredCount, MaxArmoredCount);

    for (u32 Index = 0; Index < GruntCount; Index++)
        GameSpawnEnemyGrunt();

    for (u32 Index = 0; Index < MoverCount; Index++)
        GameSpawnEnemyMover();

    for (u32 Index = 0; Index < ArmoredCount; Index++)
        GameSpawnEnemyArmored();

    Game.EnemyCount = GruntCount + MoverCount + ArmoredCount;
}

static void GameRestart(void)
{
    RemoveAllEntities();
    memset(&Game, 0, sizeof(game_state));

    Game.PlayerID = GameSpawnPlayer();
    GameStartNewStage();
}

static void GameUpdateWeapon(weapon* Weapon)
{
    Weapon->Cooldown -= Game.DeltaTime;
    Weapon->Cooldown = Maximum(0.0f, Weapon->Cooldown);
}

static void GameWeaponTryShoot(weapon* Weapon, v2 P, v2 Direction, u64 CollisionMask)
{
    if (Weapon->Cooldown > 0.0f)
        return;

    weapon_stats* Stats = GetWeaponStats(Weapon->Kind);

    entity_id BulletID = AddEntity(EntityKind_Bullet);
    entity* BulletEntity = GetEntity(BulletID);
    body* BulletBody = GetBody(BulletEntity->BodyID);
    bullet* Bullet = &BulletEntity->Bullet;

    BulletEntity->Color = Stats->BulletColor;
    BulletBody->P = P;
    BulletBody->DP = V2MulScalar(Direction, Stats->BulletSpeed);
    BulletBody->DDP = V2MulScalar(Direction, Stats->BulletAccel);
    BulletBody->Size = Stats->BulletSize;
    BulletBody->CollisionMask = CollisionMask;
    Bullet->Damage = Stats->BulletDamage;

    u32 FireParticleCount = 4;
    for (usize Index = 0; Index < FireParticleCount; Index++)
    {
        entity_id ParticleID = AddEntity(EntityKind_Particle);
        entity* ParticleEntity = GetEntity(ParticleID);
        body* ParticleBody = GetBody(ParticleEntity->BodyID);
        particle* Particle = &ParticleEntity->Particle;

        v2 Jitter = V2(
            0.5f * RandomBilateral(&Game.Entropy),
            0.5f * RandomBilateral(&Game.Entropy)
        );

        v2 ParticleDirection = V2NormalizeOrZero(V2Add(Jitter, Direction));

        ParticleBody->P = BulletBody->P;
        ParticleBody->DP = V2MulScalar(ParticleDirection, 2.0f + 2.0f*RandomUnilateral(&Game.Entropy));
        ParticleBody->DDP = V2MulScalar(ParticleDirection, 3.0f + 3.0f*RandomUnilateral(&Game.Entropy));
        ParticleBody->Size = V2(0.05f + 0.1f*RandomUnilateral(&Game.Entropy), 0.05f + 0.1f*RandomUnilateral(&Game.Entropy));

        Particle->BaseColor = V4(1.0f, 0.9f, 0.6f, 0.9f);
        Particle->Lifetime = 0.1f + 0.2f*RandomUnilateral(&Game.Entropy);
        Particle->TimeRemaining = Particle->Lifetime;
    }

    u32 SmokeParticleCount = 8;
    for (usize Index = 0; Index < SmokeParticleCount; Index++)
    {
        entity_id ParticleID = AddEntity(EntityKind_Particle);
        entity* ParticleEntity = GetEntity(ParticleID);
        body* ParticleBody = GetBody(ParticleEntity->BodyID);
        particle* Particle = &ParticleEntity->Particle;

        v2 Jitter = V2(
            0.5f * RandomBilateral(&Game.Entropy),
            0.5f * RandomBilateral(&Game.Entropy)
        );

        v2 ParticleDirection = V2NormalizeOrZero(V2Add(Jitter, Direction));

        ParticleBody->P = BulletBody->P;
        ParticleBody->DP = V2MulScalar(ParticleDirection, 2.0f + 2.0f*RandomUnilateral(&Game.Entropy));
        ParticleBody->DDP = V2MulScalar(ParticleDirection, 4.0f + 3.0f*RandomUnilateral(&Game.Entropy));
        ParticleBody->Size = V2(0.1f + 0.05f*RandomUnilateral(&Game.Entropy), 0.1f + 0.05f*RandomUnilateral(&Game.Entropy));

        Particle->BaseColor = V4(0.4f, 0.4f, 0.4f, 0.7f);
        Particle->Lifetime = 0.2f + 0.2f*RandomUnilateral(&Game.Entropy);
        Particle->TimeRemaining = Particle->Lifetime;
    }

    Weapon->Cooldown = SafeDivide0(1.0f, Stats->FireRate);
}

static void GamePlayerTryShoot(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    body* Body = GetBody(Entity->BodyID);
    player* Player = &Entity->Player;

    v2 Direction = V2(0, 1);
    v2 BulletP = V2Add(Body->P, V2(0.0f, Body->Size.Y));

    GameWeaponTryShoot(&Player->Weapon, BulletP, Direction, CollisionMaskFlag_Enemy);
}

static void GamePlayerUpdate(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    body* Body = GetBody(Entity->BodyID);
    player* Player = &Entity->Player;

    GameUpdateWeapon(&Player->Weapon);

    if (InputIsButtonDown(InputButton_Shoot))
        GamePlayerTryShoot(EntityID);

    v2  MoveDirection = V2(0.0f, 0.0f);

    if (InputIsButtonDown(InputButton_MoveLeft))    MoveDirection.X -= 1.0f;
    if (InputIsButtonDown(InputButton_MoveRight))   MoveDirection.X += 1.0f;
    if (InputIsButtonDown(InputButton_MoveDown))    MoveDirection.Y -= 1.0f;
    if (InputIsButtonDown(InputButton_MoveUp))      MoveDirection.Y += 1.0f;

    MoveDirection = V2NormalizeOrZero(MoveDirection);

    f32 MoveFriction    = 35.0f;
    f32 MoveForce       = 250.0f;

    v2 Force = V2Sub(
        V2ScalarMul(MoveForce,      MoveDirection),
        V2ScalarMul(MoveFriction,   Body->DP)
    );

    SetForce(Entity->BodyID, Force);
}

static void GameBulletUpdate(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    body* Body = GetBody(Entity->BodyID);

    rect2 BulletRect = R2CenterSize(Body->P, Body->Size);

    if (!R2Intersects(Game.ViewRect, BulletRect))
        RemoveEntity(EntityID);

    u32 TrailParticleCount = 1;
    for (usize Index = 0; Index < TrailParticleCount; Index++)
    {
        entity_id ParticleID = AddEntity(EntityKind_Particle);
        entity* ParticleEntity = GetEntity(ParticleID);
        body* ParticleBody = GetBody(ParticleEntity->BodyID);
        particle* Particle = &ParticleEntity->Particle;

        v2 Jitter = V2(
            0.2f * RandomBilateral(&Game.Entropy),
            0.2f * RandomBilateral(&Game.Entropy)
        );

        v2 ParticleDirection = V2NormalizeOrZero(V2Sub(Jitter, V2NormalizeOrZero(Body->DP)));

        ParticleBody->P = Body->P;
        ParticleBody->DP = V2MulScalar(ParticleDirection, 2.0f + 2.0f*RandomUnilateral(&Game.Entropy));
        ParticleBody->DDP = V2MulScalar(ParticleDirection, 3.0f + 3.0f*RandomUnilateral(&Game.Entropy));
        ParticleBody->Size = V2(0.05f + 0.05f*RandomUnilateral(&Game.Entropy), 0.05f + 0.05f*RandomUnilateral(&Game.Entropy));

        Particle->BaseColor = Entity->Color;
        Particle->Lifetime = 0.02f + 0.02f*RandomUnilateral(&Game.Entropy);
        Particle->TimeRemaining = Particle->Lifetime;
    }
}

static void GameParticleUpdate(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    body* Body = GetBody(Entity->BodyID);
    particle* Particle = &Entity->Particle;

    Particle->TimeRemaining -= Game.DeltaTime;

    if (Particle->TimeRemaining <= 0.0f)
    {
        RemoveEntity(EntityID);
        return;
    }

    f32 T = Particle->TimeRemaining / Particle->Lifetime;
    v4 ColorScale = V4(1.0f, 1.0f, 1.0f, T);

    Entity->Color = V4Mul(Particle->BaseColor, ColorScale);
}

static v2 GameComputeEnemyMoveForce(v2 RestP, v2 CurrentP, v2 CurrentDP)
{
    // NOTE(vak): Proportional derivative controller to move the enemy
    // from 'CurrentP' to 'RestP' in a smooth manner.

    // NOTE(vak): Basically a mass-spring damper model

    v2 MoveDirection = V2NormalizeOrZero(V2Sub(RestP, CurrentP));
    f32 MoveFriction = 2.0f;
    f32 MoveForce = 10.0f;

    v2 JitterDirection = V2NormalizeOrZero(V2(
        RandomBilateral(&Game.Entropy),
        RandomBilateral(&Game.Entropy)
    ));

    f32 JitterStrength = 25.0f;

    v2 Force = V2Zero();

    Force = V2Add(Force, V2Sub(
        V2ScalarMul(MoveForce, MoveDirection),
        V2ScalarMul(MoveFriction, CurrentDP)
    ));

    Force = V2Add(Force, V2ScalarMul(JitterStrength, JitterDirection));

    return (Force);
}

static void GameEnemyTryShoot(weapon* Weapon, body* Body)
{
    v2 P = V2Add(Body->P, V2(0, -Body->Size.Y));
    v2 Direction = V2(0, -1);

    GameWeaponTryShoot(Weapon, P, Direction, CollisionMaskFlag_Player);
}

static void GameEnemyGruntUpdate(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    body* Body = GetBody(Entity->BodyID);
    enemy_grunt* Grunt = &Entity->Grunt;

    GameUpdateWeapon(&Grunt->Weapon);
    GameEnemyTryShoot(&Grunt->Weapon, Body);

    SetForce(Entity->BodyID, GameComputeEnemyMoveForce(Grunt->RestP, Body->P, Body->DP));
}

static void GameEnemyMoverUpdate(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    body* Body = GetBody(Entity->BodyID);
    enemy_mover* Mover = &Entity->Mover;

    GameUpdateWeapon(&Mover->Weapon);
    GameEnemyTryShoot(&Mover->Weapon, Body);

    SetForce(Entity->BodyID, GameComputeEnemyMoveForce(Mover->RestP, Body->P, Body->DP));

    Mover->ShuffleTimer -= Game.DeltaTime;
    if (Mover->ShuffleTimer <= 0.0f)
    {
        entity* PlayerEntity = GetEntity(Game.PlayerID);
        body* PlayerBody = GetBody(PlayerEntity->BodyID);

        Mover->RestP = V2(
            PlayerBody->P.X + 5.0f * RandomBilateral (&Game.Entropy),
            4.0f            + 4.0f * RandomUnilateral(&Game.Entropy)
        );

        Mover->ShuffleTimer = Mover->ShuffleCooldown;
    }
}

static void GameEnemyArmoredUpdate(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    body* Body = GetBody(Entity->BodyID);
    enemy_armored* Armored = &Entity->Armored;

    GameUpdateWeapon(&Armored->Weapon);
    GameEnemyTryShoot(&Armored->Weapon, Body);

    SetForce(Entity->BodyID, GameComputeEnemyMoveForce(Armored->RestP, Body->P, Body->DP));

    Armored->ShuffleTimer -= Game.DeltaTime;
    if (Armored->ShuffleTimer <= 0.0f)
    {
        Armored->RestP = V2(
            0.0f + 8.0f * RandomBilateral (&Game.Entropy),
            4.0f + 4.0f * RandomUnilateral(&Game.Entropy)
        );

        Armored->ShuffleTimer = Armored->ShuffleCooldown;
    }
}

static void GameHandleCollision(body_id ThisBodyID, body_id OtherBodyID)
{
    entity_id ThisEntityID = GetEntityFromBody(ThisBodyID);
    entity_id OtherEntityID = GetEntityFromBody(OtherBodyID);

    printf("%u, %u\n", ThisEntityID, OtherEntityID);

    entity* ThisEntity = GetEntity(ThisEntityID);
    entity* OtherEntity = GetEntity(OtherEntityID);

    entity_id PlayerEntityID    = NilEntityID;
    entity_id EnemyEntityID     = NilEntityID;
    entity_id BulletEntityID    = NilEntityID;

    switch (ThisEntity->Kind)
    {
        case EntityKind_Player:         PlayerEntityID  = ThisEntityID; break;
        case EntityKind_EnemyGrunt:     EnemyEntityID   = ThisEntityID; break;
        case EntityKind_EnemyMover:     EnemyEntityID   = ThisEntityID; break;
        case EntityKind_EnemyArmored:   EnemyEntityID   = ThisEntityID; break;
        case EntityKind_Bullet:         BulletEntityID  = ThisEntityID; break;
    }

    switch (OtherEntity->Kind)
    {
        case EntityKind_Player:         PlayerEntityID  = OtherEntityID; break;
        case EntityKind_EnemyGrunt:     EnemyEntityID   = OtherEntityID; break;
        case EntityKind_EnemyMover:     EnemyEntityID   = OtherEntityID; break;
        case EntityKind_EnemyArmored:   EnemyEntityID   = OtherEntityID; break;
        case EntityKind_Bullet:         BulletEntityID  = OtherEntityID; break;
    }

    if (EnemyEntityID && BulletEntityID)
    {
        RemoveEntity(BulletEntityID);
    }

    if (PlayerEntityID && BulletEntityID)
    {
        RemoveEntity(BulletEntityID);
    }
}

static void GameSetup(usize RandomSeed)
{
    Game.Entropy.State = RandomSeed;

    EquipEntityKindUpdate(EntityKind_Player,        GamePlayerUpdate);
    EquipEntityKindUpdate(EntityKind_Bullet,        GameBulletUpdate);
    EquipEntityKindUpdate(EntityKind_Particle,      GameParticleUpdate);
    EquipEntityKindUpdate(EntityKind_EnemyGrunt,    GameEnemyGruntUpdate);
    EquipEntityKindUpdate(EntityKind_EnemyMover,    GameEnemyMoverUpdate);
    EquipEntityKindUpdate(EntityKind_EnemyArmored,  GameEnemyArmoredUpdate);

    EquipCollisionHandler(GameHandleCollision);

    GameRestart();
}

static void GameUpdateAndRender(f32 DeltaTime, u32 Width, u32 Height)
{
    Game.DeltaTime = DeltaTime;

    f32 AspectRatio = (f32)Width / (f32)Height;
    f32 FocalLength = 0.05f;

    v2 ViewCenter = V2(0.0f, 0.0f);
    v2 ViewSize   = V2(
        AspectRatio / FocalLength,
        1.0f        / FocalLength
    );

    Game.ViewRect = R2CenterSize(ViewCenter, ViewSize);

    RenderOrthographic2D(Game.ViewRect);

    UpdateAllEntities();
    IntegrateAllForces(DeltaTime);
    HandleCollisions();
    RenderAllEntities();
}

#else

#define CollisionMaskFlag_Player        (1ull << 0)
#define CollisionMaskFlag_Enemy         (1ull << 1)
#define CollisionMaskFlag_All       (U64Max)

typedef struct
{
    body_id BodyID;
    v4      Color;
    float   ShootTimer;
    float   CurrentHealth;
    float   MaxHealth;
} player;

typedef struct
{
    b32                 Alive;
    f32                 Damage;
    body_id             BodyID;
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
    body_id     BodyID;
    v4          Color;
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
    f32         Lifetime;
    f32         Remaining;
    body_id     BodyID;
    v4          Color;
} particle;

typedef enum
{
    EntityKind_Player   = 0,
    EntityKind_Enemy,
    EntityKind_Bullet,
    EntityKind_COUNT,
} entity_kind;

typedef struct
{
    entity_kind Kind;
    union
    {
        player* Player;
        enemy*  Enemy;
        bullet* Bullet;
    };
} entity;

typedef struct
{
    u32             Stage;
    random_state    Entropy;
    player          Player;
    bullet          Bullets[256];
    enemy           Enemies[256];
    entity          Entities[1024];
    particle        Particles[1024];
} game_state;

static game_state Game = {0};

static void GameSpawnBullet(f32 Damage, v2 P, v2 DP, v2 DDP, v2 Size, v4 Color, u64 CollisionMask)
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
        Bullet->BodyID      = AddBody(P, DP, DDP, Size, 1.0f);
        Bullet->Color       = Color;

        EquipBodyCollisionMask(Bullet->BodyID, CollisionMask);
    }
}

static void GameKillBullet(bullet* Bullet)
{
    Bullet->Alive = false;
    RemoveBody(Bullet->BodyID);
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
        Enemy->BodyID           = AddBody(P, DP, V2Zero(), Size, 1.0f);
        Enemy->CurrentHealth    = MaxHealth;
        Enemy->ShootTimer       = ShootCooldown * RandomUnilateral(&Game.Entropy);
        Enemy->MaxHealth        = MaxHealth;
        Enemy->ShootCooldown    = ShootCooldown;

        EquipBodyCollisionMask(Enemy->BodyID, CollisionMaskFlag_Enemy);
    }
}

static void GameEnemyMoverThink(enemy* Enemy, f32 DeltaTime)
{
    Enemy->Mover.ShuffleTimer -= DeltaTime;

    if (Enemy->Mover.ShuffleTimer <= 0.0f)
    {
        player* Player = &Game.Player;
        body* PlayerBody = GetBody(Player->BodyID);

        Enemy->RestP = V2(
            PlayerBody->P.X + 1.0f * RandomBilateral(&Game.Entropy),
            6.0f            + 2.0f * RandomBilateral(&Game.Entropy)
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
        Particle->BodyID    = AddBody(P, DP, DDP, Size, 1.0f);
        Particle->Color     = Color;
    }
}

static void GamePlayerTryShoot(void)
{
    player* Player = &Game.Player;

    if (Player->ShootTimer > 0.0f)
        return;

    body* PlayerBody = GetBody(Player->BodyID);

    v2  BulletSize = V2(0.1f, 0.2f);
    f32 Damage = 20.0f;
    v2  SpawnP = V2(PlayerBody->P.X, PlayerBody->P.Y + PlayerBody->Size.Y + 0.5f*BulletSize.Y);
    v2  SpawnDP = V2(0.0f, Maximum(0.0f, PlayerBody->DP.Y) + 8.0f);
    v2  SpawnDDP = V2(0.0f, 20.0f);
    v4  BulletColor = V4(0.5f, 0.8f, 1.0f, 1.0f);

    v2 SpreadDP[3] =
    {
        V2(+0.0f, 0.0f),
        V2(-1.5f, 0.0f),
        V2(+1.5f, 0.0f),
    };

    GameSpawnBullet(Damage, SpawnP, V2Add(SpawnDP, SpreadDP[0]), SpawnDDP, BulletSize, BulletColor, CollisionMaskFlag_Enemy);
    GameSpawnBullet(Damage, SpawnP, V2Add(SpawnDP, SpreadDP[1]), SpawnDDP, BulletSize, BulletColor, CollisionMaskFlag_Enemy);
    GameSpawnBullet(Damage, SpawnP, V2Add(SpawnDP, SpreadDP[2]), SpawnDDP, BulletSize, BulletColor, CollisionMaskFlag_Enemy);

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

    body* EnemyBody = GetBody(Enemy->BodyID);

    v2  BulletSize = V2(0.1f, 0.1f);
    f32 Damage = 20.0f;
    v2  SpawnP = V2(EnemyBody->P.X, EnemyBody->P.Y - EnemyBody->Size.Y - 0.5f*BulletSize.Y);
    v2  SpawnDP = V2(0.0f, -5.0f);
    v2  SpawnDDP = V2(0.0f, -10.0f);
    v4  BulletColor = V4(1.0f, 0.4f, 0.2f, 1.0f);

    GameSpawnBullet(Damage, SpawnP, SpawnDP, SpawnDDP, BulletSize, BulletColor, CollisionMaskFlag_Player);

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

    v2 InitialP = V2(0.0f, -6.5f);
    v2 Size = V2(0.6f, 0.3f);

    Player->BodyID = AddBody(InitialP, V2Zero(), V2Zero(), Size, 1.0f);
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

            if (Enemy->Alive == false)
            {
                usize DeathParticleCount = 16;

                for (usize Index = 0; Index < DeathParticleCount; Index++)
                {
                    float   ParticleLifetime   = 0.3f + 0.2f*RandomUnilateral(&Game.Entropy);
                    v2      ParticleP          = Enemy->P;
                    v2      ParticleDP         = V2(3.0f*RandomBilateral(&Game.Entropy), 3.0f*RandomBilateral(&Game.Entropy));
                    v2      ParticleDDP        = ParticleDP;
                    v2      ParticleSize       = V2Scalar(0.4f + 0.1f*RandomUnilateral(&Game.Entropy));
                    v4      ParticleColor      = V4(1.0f, 1.0f, 1.0f, 1.0f);

                    GameSpawnParticle(ParticleLifetime, ParticleP, ParticleDP, ParticleDDP, ParticleSize, ParticleColor);
                }
            }

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

            if (Enemy->Alive == false)
            {
                usize DeathParticleCount = 16;

                for (usize Index = 0; Index < DeathParticleCount; Index++)
                {
                    float   ParticleLifetime   = 0.3f + 0.2f*RandomUnilateral(&Game.Entropy);
                    v2      ParticleP          = Enemy->P;
                    v2      ParticleDP         = V2(3.0f*RandomBilateral(&Game.Entropy), 3.0f*RandomBilateral(&Game.Entropy));
                    v2      ParticleDDP        = ParticleDP;
                    v2      ParticleSize       = V2Scalar(0.4f + 0.1f*RandomUnilateral(&Game.Entropy));
                    v4      ParticleColor      = V4(1.0f, 1.0f, 1.0f, 1.0f);

                    GameSpawnParticle(ParticleLifetime, ParticleP, ParticleDP, ParticleDDP, ParticleSize, ParticleColor);
                }
            }
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

#endif

