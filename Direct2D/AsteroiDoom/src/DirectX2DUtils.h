#pragma once

#define WIN32_LEAN_AND_MEAN

#include "collidable/Arena.h"
#include "utils/AsteroiDoomConstants.h"
#include "utils/TextUtils.h"

#include <d2d1_3.h>
#include <memory>
#include <numbers>
#include <wincodec.h>
#include <random>

class DirectX2DHelper {
	HWND hwnd{};
	IWICImagingFactory * WICFactory{nullptr};
	ID2D1Factory * D2DFactory{nullptr};
	ID2D1HwndRenderTarget * target{nullptr};
	RECT windowSize{0, 0};

	BitmapHelper SpaceshipBitmap{};
	BitmapHelper ProjectileBitmap{};
	BitmapHelper AsteroidBitmaps[2]{};
	const BitmapSegment SpaceshipBitmapSegment{&SpaceshipBitmap, {0, 0, 60, 60}};
	const BitmapSegment ProjectileBitmapSegment{&ProjectileBitmap, {0, 0, 10, 10}};
	static const unsigned int numOfAsteroidTypes = 2;
	const BitmapSegment AsteroidBitmapSegments[numOfAsteroidTypes]{
	    {&AsteroidBitmaps[0], {0, 0, 40, 40}}, {&AsteroidBitmaps[1], {0, 0, 60, 60}}};
	static constexpr float asteroidSizes[numOfAsteroidTypes]{20, 30};
	static constexpr unsigned int asteroidHealth[numOfAsteroidTypes]{50, 150};

	TextHelper ScoreText{};
	TextHelper HealthText{};

	std::shared_ptr<Spaceship> spaceship{};

	Arena arena;

	std::mt19937 rng{std::random_device{}()};

	unsigned int randomAsteroidType();
public:
	DirectX2DHelper();

	explicit DirectX2DHelper(HWND hwnd);

	~DirectX2DHelper();

	void reloadTarget(HWND newHwnd);

	void reloadTarget();

	void nextFrame();
};