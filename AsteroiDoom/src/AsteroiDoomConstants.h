#pragma once

#define WIN32_LEAN_AND_MEAN

#include <d2d1.h>

// ----------------------------- ARENA -----------------------------

const float ArenaWidth = 1920;
const float ArenaHeight = 1080;

const D2D_MATRIX_3X2_F ArenaTranslation = D2D1::Matrix3x2F::Translation(ArenaWidth / 2, ArenaHeight / 2);

const float SpawnAreaMargin = 200;
// const float SpawnAreaWidth = ArenaWidth + 2 * SpawnAreaMargin;
// const float SpawnAreaHeight = ArenaHeight + 2 * SpawnAreaMargin;
// const float SpawnAreaLeft = -SpawnAreaWidth / 2;
// const float SpawnAreaTop = -SpawnAreaHeight / 2;
// const float SpawnAreaRight = SpawnAreaWidth / 2;
// const float SpawnAreaBottom = SpawnAreaHeight / 2;

// ----------------------------- MEDIA -----------------------------

const LPCWSTR Asteroid20Path = L"../assets/Asteroid20.png";
