
#pragma once

// NOTE(vak): Cheatsheet

typedef u32 body_id;
#define NilBodyID (0)

typedef struct
{
    v2  P;
    v2  DP;
    v2  DDP;
    v2  Size;
    f32 InvMass;
    u64 CollisionMask;  // NOTE(vak): Bodies whose masks both share a bit is identified to be within the same collision layer, which means that the collision handler will be called.
} body;

typedef void collision_handler(body_id BodyID0, body_id BodyID1);

static void     EquipCollisionHandler   (collision_handler* Handler);               // NOTE(vak): Equips a collision handler that will be called by HandleCollisions().

static body_id  AddBody                 (v2 P, v2 DP, v2 DDP, v2 Size, f32 Mass);   // NOTE(vak): Adds and initializes a new body
static void     EquipBodyCollisionMask  (body_id BodyID, u64 Mask);                 // NOTE(vak): Equips a body with a collision mask.
static body*    GetBody                 (body_id BodyID);                           // NOTE(vak): Get body memory from body_id
static void     RemoveBody              (body_id BodyID);                           // NOTE(vak): Removes body
static void     SetBodyP                (body_id BodyID, v2 P);                     // NOTE(vak): Sets a body's position to the specified position
static void     ResetForces             (body_id BodyID);                           // NOTE(vak): Reset a body's acceleration to 0
static void     ApplyForce              (body_id BodyID, v2 Force);                 // NOTE(vak): Adds to a body's acceleration due to the applied force
static void     SetForce                (body_id BodyID, v2 Force);                 // NOTE(vak): Sets a body's acceleration due to the applied force
static void     IntegrateForces         (body_id BodyID, f32 DeltaTime);            // NOTE(vak): Updates velocity and position. Leaves acceleration untouched.

static void     IntegrateAllForces      (f32 DeltaTime);                            // NOTE(vak): Update every body's position and velocities
static void     HandleCollisions        (void);                                     // NOTE(vak): Check for collisions and calls collision handler set by EquipCollisionHandler()

// NOTE(vak): Implementation

typedef struct
{
    body Bodies         [2048];
    u64  ActiveBodyMasks[2048 / 64]; // NOTE(vak): A body_id corresponds to a bit in this bit array, which determines whether a body slot is active or not.

    collision_handler*  OnCollision;
} physics_state;

static physics_state Physics = {0};

static void EquipCollisionHandler(collision_handler* Handler)
{
    Physics.OnCollision = Handler;
}

static b32 IsBodyActive(body_id BodyID)
{
    usize BitIndex  = BodyID % 64;
    usize MaskIndex = BodyID / 64;

    b32 Result = (Physics.ActiveBodyMasks[MaskIndex] >> BitIndex) & 0x1;
    return (Result);
}

static void MarkBodyActive(body_id BodyID)
{
    usize BitIndex  = BodyID % 64;
    usize MaskIndex = BodyID / 64;

    Physics.ActiveBodyMasks[MaskIndex] |= (1ull << BitIndex);
}

static void MarkBodyInactive(body_id BodyID)
{
    usize BitIndex  = BodyID % 64;
    usize MaskIndex = BodyID / 64;

    Physics.ActiveBodyMasks[MaskIndex] &= ~(1ull << BitIndex);
}

static body_id AddBody(v2 P, v2 DP, v2 DDP, v2 Size, f32 Mass)
{
    body_id BodyID = NilBodyID;

    for (body_id CandidateID = 1; CandidateID < ArrayCount(Physics.Bodies); CandidateID++)
    {
        if (!IsBodyActive(CandidateID))
        {
            BodyID = CandidateID;
            MarkBodyActive(BodyID);
            break;
        }
    }

    if (BodyID == NilBodyID)
        return (BodyID);

    body* Body = GetBody(BodyID);

    Body->P         = P;
    Body->DP        = DP;
    Body->DDP       = DDP;
    Body->Size      = Size;
    Body->InvMass   = (Mass > 0.0f) ? (1.0f / Mass) : (0.0f);

    return (BodyID);
}

static void EquipBodyCollisionMask(body_id BodyID, u64 Mask)
{
    body* Body = GetBody(BodyID);
    Body->CollisionMask = Mask;
}

static body* GetBody(body_id BodyID)
{
    body* Body = Physics.Bodies + BodyID;
    return (Body);
}

static void RemoveBody(body_id BodyID)
{
    MarkBodyInactive(BodyID);
    memset(GetBody(BodyID), 0, sizeof(body));
}

static void SetBodyP(body_id BodyID, v2 P)
{
    body* Body = GetBody(BodyID);
    Body->P = P;
}

static void ResetForces(body_id BodyID)
{
    body* Body = GetBody(BodyID);
    Body->DDP = V2(0, 0);
}

static void ApplyForce(body_id BodyID, v2 Force)
{
    body* Body = GetBody(BodyID);
    Body->DDP = V2Add(Body->DDP, V2MulScalar(Force, Body->InvMass));
}

static void SetForce(body_id BodyID, v2 Force)
{
    body* Body = GetBody(BodyID);
    Body->DDP = V2MulScalar(Force, Body->InvMass);
}

static void IntegrateForces(body_id BodyID, f32 DeltaTime)
{
    body* Body = GetBody(BodyID);

    Body->DP = V2Add(Body->DP, V2MulScalar(Body->DDP, DeltaTime));
    Body->P = V2Add(Body->P, V2MulScalar(Body->DP, DeltaTime));
}

static void IntegrateAllForces(f32 DeltaTime)
{
    for (body_id ID = 1; ID < ArrayCount(Physics.Bodies); ID++)
        IntegrateForces(ID, DeltaTime);
}

static void HandleCollisions(void)
{
    if (!Physics.OnCollision)
        return;

    // TODO(vak): This is O(n^2) right now (n is the number of active bodies), which is
    // pretty terrible.

    // In the future, we can construct a dynamic AABB tree, which takes O(n) time. Then,
    // we enumerate over every body, which takes O(n) time, and then walk down the AABB
    // tree for collisions in O(lg(n)) time.

    // Overall, it takes O(n + n * lg(n)), which simplifies to O(n * lg(n)) time, which
    // offers a nice speed up from O(n^2).

    // Note that the dynamic AABB tree will be constructed once and updated incrementally,
    // so the speed up is even better.

    for (body_id ThisID = 1; ThisID < ArrayCount(Physics.Bodies); ThisID++)
    {
        if (!IsBodyActive(ThisID))
            continue;

        // NOTE(vak): Starting the search from (ThisID + 1) guarantees that we won't generate
        // identical collision pairs multiple times.

        for (body_id OtherID = ThisID + 1; OtherID < ArrayCount(Physics.Bodies); OtherID++)
        {
            if (!IsBodyActive(OtherID))
                continue;

            body* ThisBody  = GetBody(ThisID);
            body* OtherBody = GetBody(OtherID);

            if ((ThisBody->CollisionMask & OtherBody->CollisionMask) == 0)
                continue;

            rect2 ThisRect  = R2CenterSize(ThisBody->P,  ThisBody->Size);
            rect2 OtherRect = R2CenterSize(OtherBody->P, OtherBody->Size);

            if (R2Intersects(ThisRect, OtherRect))
                Physics.OnCollision(ThisID, OtherID);
        }
    }
}

