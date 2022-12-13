#pragma once

#define WIN32_LEAN_AND_MEAN

#include <d2d1.h>
#include <numbers>

// ----------------------------- UTILS -----------------------------

const float RadiansInDegree = std::numbers::pi_v<float> / 180;

// ----------------------------- ARENA -----------------------------

const float ArenaWidth = float(GetSystemMetrics(SM_CXSCREEN));
const float ArenaHeight = float(GetSystemMetrics(SM_CYSCREEN));

const D2D_MATRIX_3X2_F ArenaTranslation = D2D1::Matrix3x2F::Translation(ArenaWidth / 2, ArenaHeight / 2);

const float SpawnAreaMargin = 200;

const float SpaceshipHitPoints = 100;
const float SpaceshipDeceleration = 0.1;
const float SpaceshipThrust = 600;
const float SpaceshipTorque = 400;
const float SpaceshipGunOffset = 20;
const float SpaceshipGunCooldown = 250;
const float ProjectileSpeed = 600;

const unsigned int AsteroidSpawnDelay = 3000;

// ----------------------------- MEDIA -----------------------------

const float SpaceshipSize = 25;
const LPCWSTR SpaceshipPath = L"../assets/Spaceship.png";
const LPCWSTR ProjectilePath = L"../assets/Projectile.png";
const LPCWSTR Asteroid20Path = L"../assets/Asteroid20.png";
