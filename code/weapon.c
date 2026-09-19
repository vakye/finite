
#pragma once

typedef enum
{
    WeaponKind_None     = 0,

    WeaponKind_PlayerPewPew,    // NOTE(vak): Dev weapon :]

    // NOTE(vak): Enemy weapons

    WeaponKind_GruntPistol,
    WeaponKind_MoverPistol,
    WeaponKind_ArmoredRevolver,

    WeaponKind_COUNT,
} weapon_kind;

typedef struct
{
    f32 FireRate;       // NOTE(vak): Bullets / Second
    f32 BulletDamage;    
    f32 BulletSpeed;
    f32 BulletAccel;
    v2  BulletSize;
    v4  BulletColor;
} weapon_stats;

typedef struct
{
    weapon_kind Kind;
    f32 Cooldown;       // NOTE(vak): Remaining seconds until this weapon can shoot again
} weapon;

static weapon_stats AllWeaponStats[WeaponKind_COUNT] =
{
    [WeaponKind_PlayerPewPew] =
        {.FireRate = 10.0f, .BulletDamage = 15.0f, .BulletSpeed = 10.0f, .BulletAccel = 20.0f, .BulletSize = {.E = {0.1f, 0.2f}}, .BulletColor = {.E = {0.5f, 0.8f, 1.0f, 1.0f}}},

    [WeaponKind_GruntPistol] =
        {.FireRate = 1.0f, .BulletDamage = 10.0f, .BulletSpeed = 6.0f, .BulletAccel = 10.0f, .BulletSize = {.E = {0.1f, 0.1f}}, .BulletColor = {.E = {1.0f, 0.4f, 0.2f, 1.0f}}},

    [WeaponKind_MoverPistol] =
        {.FireRate = 2.0f, .BulletDamage = 5.0f, .BulletSpeed = 8.0f, .BulletAccel = 8.0f, .BulletSize = {.E = {0.075f, 0.075f}}, .BulletColor = {.E = {1.0f, 0.4f, 0.2f, 1.0f}}},

    [WeaponKind_ArmoredRevolver] =
        {.FireRate = 0.5f, .BulletDamage = 30.0f, .BulletSpeed = 3.0f, .BulletAccel = 12.0f, .BulletSize = {.E = {0.15f, 0.15f}}, .BulletColor = {.E = {1.0f, 0.4f, 0.2f, 1.0f}}},
};

#define GetWeaponStats(WeaponKind) (&AllWeaponStats[WeaponKind])

