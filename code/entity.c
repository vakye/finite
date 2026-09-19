
#pragma once

// NOTE(vak): Cheatsheet

typedef u32 entity_id;
#define NilEntityID (0)

typedef enum
{
    EntityKind_Nil = 0,

    EntityKind_Player,
    EntityKind_Bullet,
    EntityKind_Particle,

    // NOTE(vak): Enemies

    EntityKind_EnemyGrunt,
    EntityKind_EnemyMover,
    EntityKind_EnemyArmored,

    EntityKind_COUNT,
} entity_kind;

typedef struct
{
    weapon Weapon;
    f32 CurrentHealth;
    f32 MaxHealth;
} player;

typedef struct
{
    f32 Damage;
} bullet;

typedef struct
{
    weapon Weapon;
    f32 CurrentHealth;
    f32 MaxHealth;
    v2 RestP;
} enemy_grunt;

typedef struct
{
    weapon Weapon;
    f32 CurrentHealth;
    f32 ShuffleTimer;
    f32 ShuffleCooldown;
    f32 MaxHealth;
    v2 RestP;
} enemy_mover;

typedef struct
{
    weapon Weapon;
    f32 CurrentHealth;
    f32 ShuffleTimer;
    f32 ShuffleCooldown;
    f32 MaxHealth;
    v2 RestP;
} enemy_armored;

typedef struct
{
    f32 TimeRemaining;
    f32 Lifetime;
    v4  BaseColor;
} particle;

typedef struct
{
    entity_kind Kind;
    body_id     BodyID;
    v4          Color;
    union
    {
        player              Player;
        enemy_grunt         Grunt;
        enemy_mover         Mover;
        enemy_armored       Armored;
        bullet              Bullet;
        particle            Particle;
    };
} entity;

typedef void entity_kind_update(entity_id EntityID);

static void         EquipEntityKindUpdate   (entity_kind Kind, entity_kind_update* Update); // NOTE(vak): Equips an update function for an entity kind

static entity_id    AddEntity               (entity_kind Kind);             // NOTE(vak): Adds an entity with the specified entity kind
static entity*      GetEntity               (entity_id EntityID);           // NOTE(vak): Get the underlying memory backing an entity_id
static void         RemoveEntity            (entity_id EntityID);           // NOTE(vak): Removes an entity
static void         UpdateEntity            (entity_id EntityID);           // NOTE(vak): Calls the equipped update function depending on entity kind
static void         RenderEntity            (entity_id EntityID);           // NOTE(vak): Adds a colored rectangle representing the entity to the render list
static entity_id    GetEntityFromBody       (body_id BodyID);               // NOTE(vak): Retrieves the entity that is associated with the specified body_id

static void         RemoveAllEntities       (void);                         // NOTE(vak): Calls RemoveEntity() on all entities
static void         UpdateAllEntities       (void);                         // NOTE(vak): Calls UpdateEntity() on all entities
static void         RenderAllEntities       (void);                         // NOTE(vak): Calls RenderEntity() on all entities

// NOTE(vak): Implementation

typedef struct
{
    entity_kind_update* Updates[EntityKind_COUNT];
    entity              Entities[2048];
    u64                 ActiveEntityMasks[2048 / 64];
} entity_state;

static entity_state EntitySet = {0};

static void EquipEntityKindUpdate(entity_kind Kind, entity_kind_update* Update)
{
    EntitySet.Updates[Kind] = Update;
}

static b32 IsEntityActive(entity_id EntityID)
{
    usize BitIndex  = EntityID % 64;
    usize MaskIndex = EntityID / 64;

    b32 Result = (EntitySet.ActiveEntityMasks[MaskIndex] >> BitIndex) & 1;
    return (Result);
}

static void MarkEntityActive(entity_id EntityID)
{
    usize BitIndex  = EntityID % 64;
    usize MaskIndex = EntityID / 64;

    EntitySet.ActiveEntityMasks[MaskIndex] |= (1ull << BitIndex);
}

static void MarkEntityInactive(entity_id EntityID)
{
    usize BitIndex  = EntityID % 64;
    usize MaskIndex = EntityID / 64;

    EntitySet.ActiveEntityMasks[MaskIndex] &= ~(1ull << BitIndex);
}

static entity_id AddEntity(entity_kind Kind)
{
    entity_id EntityID = NilEntityID;

    for (entity_id CandidateID = 1; CandidateID < ArrayCount(EntitySet.Entities); CandidateID++)
    {
        if (!IsEntityActive(CandidateID))
        {
            EntityID = CandidateID;
            MarkEntityActive(EntityID);
            break;
        }
    }

    if (EntityID == NilEntityID)
        return (EntityID);

    entity* Entity  = GetEntity(EntityID);
    Entity->Kind    = Kind;
    Entity->BodyID  = AddBody(V2Zero(), V2Zero(), V2Zero(), V2Zero(), 1.0f);
    Entity->Color   = V4Zero();

    return (EntityID);
}

static entity* GetEntity(entity_id EntityID)
{
    entity* Entity = EntitySet.Entities + EntityID;
    return (Entity);
}

static void RemoveEntity(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);

    RemoveBody(Entity->BodyID);
    MarkEntityInactive(EntityID);
    memset(GetEntity(EntityID), 0, sizeof(entity));
}

static void UpdateEntity(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    entity_kind Kind = Entity->Kind;    

    if (EntitySet.Updates[Kind])
        EntitySet.Updates[Kind](EntityID);
}

static void RenderEntity(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    body* Body = GetBody(Entity->BodyID);
    v4 Color = Entity->Color;

    RenderRect(R2CenterSize(Body->P, Body->Size), Color);
}

static entity_id GetEntityFromBody(body_id BodyID)
{
    entity_id FoundEntityID = NilEntityID;

    if (BodyID == NilBodyID)
        return (FoundEntityID);

    for (entity_id ID = 1; ID < ArrayCount(EntitySet.Entities); ID++)
    {
        entity* Entity = GetEntity(ID);

        if (Entity->BodyID == BodyID)
        {
            FoundEntityID = ID;
            break;
        }
    }

    return (FoundEntityID);
}

static void RemoveAllEntities(void)
{
    for (entity_id ID = 1; ID < ArrayCount(EntitySet.Entities); ID++)
        RemoveEntity(ID);
}

static void UpdateAllEntities(void)
{
    for (entity_id ID = 1; ID < ArrayCount(EntitySet.Entities); ID++)
        UpdateEntity(ID);
}

static void RenderAllEntities(void)
{
    for (entity_id ID = 1; ID < ArrayCount(EntitySet.Entities); ID++)
        RenderEntity(ID);
}

