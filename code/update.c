
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

// NOTE(vak): Bit masks

static intptr_t SearchMasksForUnsetBit(size_t* Masks, unsigned int BitCount)
{
    intptr_t UnsetBitIndex = INTPTR_MIN;
    unsigned int MaskCount = (BitCount + 63) / 64;

    for (unsigned int MaskIndex = 0; MaskIndex < MaskCount; MaskIndex++)
    {
        size_t Mask = Masks[MaskIndex];

        if (Mask == ~0ULL)
            continue;

        for (unsigned int BitIndex = 0; BitIndex < 64; BitIndex++)
        {
            if ((Mask & (1ull << BitIndex)) == 0)
            {
                UnsetBitIndex = (int)(MaskIndex*64 + BitIndex);
                break;
            }
        }

        if (UnsetBitIndex != -1)
            break;
    }

    if (UnsetBitIndex >= BitCount)
        UnsetBitIndex = -1;

    return (UnsetBitIndex);
}

static void SetBitInMasks(size_t* Masks, unsigned int BitCount, intptr_t BitIndex)
{
    if ((BitIndex >= 0) && (BitIndex < BitCount))
    {
        unsigned int MaskIndex      = BitIndex / 64;
        unsigned int LocalBitIndex  = BitIndex % 64;

        Masks[MaskIndex] |= (1ull << LocalBitIndex);
    }
}

static void UnsetBitInMasks(size_t* Masks, unsigned int BitCount, intptr_t BitIndex)
{
    if ((BitIndex >= 0) && (BitIndex < BitCount))
    {
        unsigned int MaskIndex      = BitIndex / 64;
        unsigned int LocalBitIndex  = BitIndex % 64;

        Masks[MaskIndex] &= ~(1ull << LocalBitIndex);
    }
}

static int GetBitFromMasks(size_t* Masks, unsigned int BitCount, intptr_t BitIndex)
{
    int Result = false;

    if ((BitIndex >= 0) && (BitIndex < BitCount))
    {
        unsigned int MaskIndex      = BitIndex / 64;
        unsigned int LocalBitIndex  = BitIndex % 64;

        Result = (Masks[MaskIndex] >> LocalBitIndex) & 1;
    }

    return (Result);
}

// NOTE(vak): Sparse set

// NOTE(vak): 0 is considered an invalid sparse set ID.
// Valid sparse set IDs start at 1.

typedef signed int sparse_set_id;

#define sparse_set(type) \
    struct \
    { \
        type*               E;          \
        size_t*             TakenMasks; \
        signed long long    Count;      \
    } \

#define SparseSetSetup(Set, ElementMemory, ElementCount, MaskMemory, MaskCount) \
    assert((MaskCount)*64 >= (ElementCount)); \
    (Set)->E            = ElementMemory; \
    (Set)->Count        = ElementCount; \
    (Set)->TakenMasks   = MaskMemory

#define SparseSetGet(Set, ID) (((ID) > 0) && ((ID <= (intptr_t)(Set)->Count)) ? ((Set)->E + ID - 1) : (0))

#define SparseSetIndexToID(Index) ((Index) + 1)

#define SparseSetIDToIndex(ID) ((ID) - 1)

#define SparseSetGetFreeSlotID(Set) (signed int)(1 + SearchMasksForUnsetBit((Set)->TakenMasks, (Set)->Count))

#define SparseSetIsSlotUsed(Set, ID) GetBitFromMasks((Set)->TakenMasks, (Set)->Count, (ID) - 1)

#define SparseSetIsSlotFree(Set, ID) (!SparseSetIsSlotUsed(Set, ID))

#define SparseSetTakeSlot(Set, ID) SetBitInMasks((Set)->TakenMasks, (Set)->Count, (ID) - 1)

#define SparseSetFreeSlot(Set, ID) UnsetBitInMasks((Set)->TakenMasks, (Set)->Count, (ID) - 1)

// NOTE(vak): Components

// NOTE(vak): 0 is considered an invalid component ID.
// Valid component IDs are always larger than 1.

typedef sparse_set_id body_id;
typedef sparse_set_id timer_id;
typedef sparse_set_id sprite_id;
typedef sparse_set_id health_id;

typedef struct
{
    v2 P;
    v2 DP;
    v2 DDP;
} body;

typedef struct
{
    float RemainingSeconds;
} timer;

typedef struct
{
    body_id     AttachedToBodyID;
    v2          Offset;             // NOTE(vak): Relative to attached body position
    v2          Size;
    v4          Color;
} sprite;

typedef struct
{
    float       Current;
    float       Max;
} health;

typedef struct
{
    sparse_set(body)    BodySet;
    sparse_set(timer)   TimerSet;
    sparse_set(sprite)  SpriteSet;
    sparse_set(health)  HealthSet;
} components;

static void ComponentSetup(components* Components)
{
    // TODO(vak): Suballocate from arena allocator
    // TODO(vak): Actually write an allocator so we don't have to do this stupid shit

    static body   BodyMemory[512] = {0};
    static timer  TimerMemory[512] = {0};
    static sprite SpriteMemory[512] = {0};
    static health HealthMemory[512] = {0};

    static size_t BodyMaskMemory[512 / 64] = {0};
    static size_t TimerMaskMemory[512 / 64] = {0};
    static size_t SpriteMaskMemory[512 / 64] = {0};
    static size_t HealthMaskMemory[512 / 64] = {0};

    #define PerformSetupFor(Name) \
        SparseSetSetup(&Components->Name##Set, Name##Memory, ARRAY_COUNT(Name##Memory), Name##MaskMemory, ARRAY_COUNT(Name##MaskMemory))

    PerformSetupFor(Body);
    PerformSetupFor(Timer);
    PerformSetupFor(Sprite);
    PerformSetupFor(Health);

    #undef PerformSetupFor
}

static body_id ComponentAddBody(components* Components)
{
    body_id BodyID = SparseSetGetFreeSlotID(&Components->BodySet);

    if (BodyID)
    {
        memset(SparseSetGet(&Components->BodySet, BodyID), 0, sizeof(body));
        SparseSetTakeSlot(&Components->BodySet, BodyID);
    }

    return (BodyID);
}

static body* ComponentGetBody(components* Components, body_id BodyID)
{
    body* Body = SparseSetGet(&Components->BodySet, BodyID);
    return (Body);
}

static void ComponentRemoveBody(components* Components, body_id BodyID)
{
    SparseSetFreeSlot(&Components->BodySet, BodyID);
}

static sprite_id ComponentAddTimer(components* Components)
{
    timer_id TimerID = SparseSetGetFreeSlotID(&Components->TimerSet);

    if (TimerID)
    {
        memset(SparseSetGet(&Components->TimerSet, TimerID), 0, sizeof(timer));
        SparseSetTakeSlot(&Components->TimerSet, TimerID);
    }

    return (TimerID);
}

static timer* ComponentGetTimer(components* Components, timer_id TimerID)
{
    timer* Timer = SparseSetGet(&Components->TimerSet, TimerID);
    return (Timer);
}

static void ComponentRemoveTimer(components* Components, timer_id TimerID)
{
    SparseSetFreeSlot(&Components->TimerSet, TimerID);
}

static sprite_id ComponentAddSprite(components* Components)
{
    sprite_id SpriteID = SparseSetGetFreeSlotID(&Components->SpriteSet);

    if (SpriteID)
    {
        memset(SparseSetGet(&Components->SpriteSet, SpriteID), 0, sizeof(sprite));
        SparseSetTakeSlot(&Components->SpriteSet, SpriteID);
    }

    return (SpriteID);
}

static sprite* ComponentGetSprite(components* Components, sprite_id SpriteID)
{
    sprite* Sprite = SparseSetGet(&Components->SpriteSet, SpriteID);
    return (Sprite);
}

static void ComponentRemoveSprite(components* Components, sprite_id SpriteID)
{
    SparseSetFreeSlot(&Components->SpriteSet, SpriteID);
}

static sprite_id ComponentAddHealth(components* Components)
{
    health_id HealthID = SparseSetGetFreeSlotID(&Components->HealthSet);

    if (HealthID)
    {
        memset(SparseSetGet(&Components->HealthSet, HealthID), 0, sizeof(health));
        SparseSetTakeSlot(&Components->HealthSet, HealthID);
    }

    return (HealthID);
}

static health* ComponentGetHealth(components* Components, health_id HealthID)
{
    health* Health = SparseSetGet(&Components->HealthSet, HealthID);
    return (Health);
}

static void ComponentRemoveHealth(components* Components, health_id HealthID)
{
    SparseSetFreeSlot(&Components->HealthSet, HealthID);
}

// NOTE(vak): Entities

typedef sparse_set_id enemy_id;
typedef sparse_set_id bullet_id;

typedef struct
{
    body_id     BodyID;
    health_id   HealthID;
    sprite_id   SpriteID;
    timer_id    ShootTimerID;
} player;

typedef struct
{
    body_id     BodyID;
    health_id   HealthID;
    sprite_id   SpriteID;
    timer_id    ShootTimerID;
} enemy;

typedef enum
{
    BulletHitFlag_None   = (0),
    BulletHitFlag_Player = (1 << 0),
    BulletHitFlag_Enemy  = (1 << 1),
} bullet_hit_flags;

typedef struct
{
    body_id             BodyID;
    sprite_id           SpriteID;
    bullet_hit_flags    HitFlags;
} bullet;

typedef struct
{
    player              Player;
    sparse_set(enemy)   EnemySet;
    sparse_set(bullet)  BulletSet;
} entities;

static void EntitySetup(entities* Entities)
{
    // TODO(vak): Suballocate from arena allocator

    static enemy EnemyMemory[128];
    static bullet BulletMemory[256];

    static size_t EnemyMaskMemory[128 / 64];
    static size_t BulletMaskMemory[256 / 64];

    #define PerformSetupFor(Name) \
        SparseSetSetup(&Entities->Name##Set, Name##Memory, ARRAY_COUNT(Name##Memory), Name##MaskMemory, ARRAY_COUNT(Name##MaskMemory))

    PerformSetupFor(Enemy);
    PerformSetupFor(Bullet);

    #undef PerformSetupFor
}

static void EntityAddPlayer(entities* Entities, components* Components)
{
    player* Player = &Entities->Player;

    Player->BodyID          = ComponentAddBody      (Components);
    Player->SpriteID        = ComponentAddSprite    (Components);
    Player->HealthID        = ComponentAddHealth    (Components);
    Player->ShootTimerID    = ComponentAddTimer     (Components);
}

static enemy_id EntityAddEnemy(entities* Entities, components* Components)
{
    enemy_id EnemyID = SparseSetGetFreeSlotID(&Entities->EnemySet);

    if (EnemyID)
    {
        enemy* Enemy = SparseSetGet(&Entities->EnemySet, EnemyID);

        Enemy->BodyID          = ComponentAddBody      (Components);
        Enemy->SpriteID        = ComponentAddSprite    (Components);
        Enemy->HealthID        = ComponentAddHealth    (Components);
        Enemy->ShootTimerID    = ComponentAddTimer     (Components);

        SparseSetTakeSlot(&Entities->EnemySet, EnemyID);
    }

    return (EnemyID);
}

static bullet_id EntityAddBullet(entities* Entities, components* Components)
{
    bullet_id BulletID = SparseSetGetFreeSlotID(&Entities->BulletSet);

    if (BulletID)
    {
        bullet* Bullet = SparseSetGet(&Entities->BulletSet, BulletID);

        Bullet->BodyID   = ComponentAddBody   (Components);
        Bullet->SpriteID = ComponentAddSprite (Components);
        Bullet->HitFlags = BulletHitFlag_None;

        SparseSetTakeSlot(&Entities->BulletSet, BulletID);
    }

    return (BulletID);
}

// NOTE(vak): Camera

typedef struct
{
    v2      ViewCenter;
    float   AspectRatio;
    float   FocalLength;
} camera;

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

// NOTE(vak): World

typedef struct
{
    random_state    Entropy;
    camera          Camera;
    components      Components;
    entities        Entities;
} world;

static void SpawnPlayer(world* World)
{
    components* Components = &World->Components;
    entities* Entities = &World->Entities;
    player* Player = &Entities->Player;

    EntityAddPlayer(Entities, Components);

    body*   Body        = ComponentGetBody  (Components, Player->BodyID);
    sprite* Sprite      = ComponentGetSprite(Components, Player->SpriteID);
    health* Health      = ComponentGetHealth(Components, Player->HealthID);
    timer*  ShootTimer  = ComponentGetTimer (Components, Player->ShootTimerID);

    Body->P = V2(0.0f, -6.5f);

    Sprite->AttachedToBodyID    = Player->BodyID;
    Sprite->Offset              = V2(0.0f, 0.0f);
    Sprite->Size                = V2(0.4f, 0.2f);
    Sprite->Color               = V4(1.0f, 0.8f, 0.5f, 1.0f);

    Health->Max     = 200;
    Health->Current = Health->Max;
}

static void SpawnRandomEnemy(world* World)
{
    components* Components = &World->Components;
    entities* Entities = &World->Entities;

    enemy_id EnemyID = EntityAddEnemy(Entities, Components);
    if (!EnemyID)
        return;

    enemy* Enemy = SparseSetGet(&Entities->EnemySet, EnemyID);

    body*   Body        = ComponentGetBody  (Components, Enemy->BodyID);
    sprite* Sprite      = ComponentGetSprite(Components, Enemy->SpriteID);
    health* Health      = ComponentGetHealth(Components, Enemy->HealthID);
    timer*  ShootTimer  = ComponentGetTimer (Components, Enemy->ShootTimerID);

    Body->P = V2(
        0.0f + 6.0f*RandomBilateral(&World->Entropy),
        5.0f + 2.0f*RandomUnilateral(&World->Entropy)
    );

    Sprite->AttachedToBodyID    = Enemy->BodyID;
    Sprite->Offset              = V2(0.0f, 0.0f);
    Sprite->Size                = V2(0.4f, 0.4f);
    Sprite->Color               = V4(0.9f, 0.3f, 0.2f, 1.0f);

    Health->Max     = 100 + 20*RandomUnilateral(&World->Entropy);
    Health->Current = Health->Max;
}


// NOTE(vak): Setup

static void SetupWorld(world* World, size_t RandomSeed)
{
    memset(World, 0, sizeof(world));
    World->Entropy.State = RandomSeed;

    ComponentSetup(&World->Components);
    EntitySetup(&World->Entities);

    SpawnPlayer(World);

    for (unsigned int Index = 0; Index < 10; Index++)
        SpawnRandomEnemy(World);
}

// NOTE(vak): Systems

static void UpdateCamera(world* World, unsigned int WindowSizeX, unsigned int WindowSizeY)
{
    camera* Camera = &World->Camera;
    Camera->ViewCenter = V2Zero();
    Camera->FocalLength = 0.05f;
    Camera->AspectRatio = (float)WindowSizeX / (float)WindowSizeY;
}

static void UpdateTimers(world* World, float DeltaTime)
{
    components* Components = &World->Components;

    for (timer_id TimerID = 1; TimerID <= Components->TimerSet.Count; TimerID++)
    {
        if (!SparseSetIsSlotUsed(&Components->TimerSet, TimerID))
            continue;

        timer* Timer = ComponentGetTimer(Components, TimerID);

        Timer->RemainingSeconds -= DeltaTime;
        Timer->RemainingSeconds = Maximum(Timer->RemainingSeconds, 0.0f);
    }
}

static void UpdatePhysics(world* World, float DeltaTime)
{
    components* Components = &World->Components;

    for (body_id BodyID = 1; BodyID <= Components->BodySet.Count; BodyID++)
    {
        if (!SparseSetIsSlotUsed(&Components->BodySet, BodyID))
            continue;

        body* Body = ComponentGetBody(Components, BodyID);

        // NOTE(vak): Integrate

        Body->DP = V2Add(Body->DP, V2MulScalar(Body->DDP, DeltaTime));
        Body->P  = V2Add(Body->P,  V2MulScalar(Body->DP,  DeltaTime));

        // NOTE(vak): Reset forces

        Body->DDP = V2Zero();
    }
}

static void UpdatePlayer(world* World, platform_input* Input)
{
}

static void UpdateEnemies(world* World)
{
    components* Components = &World->Components;
    entities* Entities = &World->Entities;

    for (enemy_id EnemyID = 1; EnemyID <= Entities->EnemySet.Count; EnemyID++)
    {
        if (!SparseSetIsSlotUsed(&Entities->EnemySet, EnemyID))
            continue;

        enemy* Enemy = SparseSetGet(&Entities->EnemySet, EnemyID);
        body* Body = SparseSetGet(&Components->BodySet, Enemy->BodyID);

        Body->DP = V2(
            2.0f * RandomBilateral(&World->Entropy),
            2.0f * RandomBilateral(&World->Entropy)
        );
    }
}

static void UpdateBullets(world* World)
{
}

static void UpdateBulletHits(world* World)
{
}

// NOTE(vak): Update

static void UpdateWorld(
    platform*   Platform,   // NOTE(vak): Input
    world*      World       // NOTE(vak): Input/Output
)
{
    UpdateCamera        (World, Platform->WindowSizeX, Platform->WindowSizeY);
    UpdateTimers        (World, Platform->DeltaTime);
    UpdatePlayer        (World, &Platform->Input);
    UpdateEnemies       (World);
    UpdateBullets       (World);
    UpdatePhysics       (World, Platform->DeltaTime);
    UpdateBulletHits    (World);
}

