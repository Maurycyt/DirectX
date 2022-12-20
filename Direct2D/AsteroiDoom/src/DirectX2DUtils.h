#pragma once

#define WIN32_LEAN_AND_MEAN

#include "collidable/Arena.h"
#include "utils/AsteroiDoomConstants.h"
#include "utils/TextUtils.h"

#include <d2d1_3.h>
#include <memory>
#include <numbers>
#include <wincodec.h>

class DirectX2DHelper {
	// IMPORTANT MEMBERS
	IWICImagingFactory * WICFactory{nullptr};
	ID2D1Factory * D2DFactory{nullptr};
	ID2D1HwndRenderTarget * target{nullptr};
	RECT windowSize{0, 0};

	BitmapHelper SpaceshipBitmap{};
	BitmapHelper ProjectileBitmap{};
	BitmapHelper Asteroid20Bitmap{};
	const BitmapSegment SpaceshipBitmapSegment{&SpaceshipBitmap, {0, 0, 60, 60}};
	const BitmapSegment ProjectileBitmapSegment{&ProjectileBitmap, {0, 0, 10, 10}};
	const BitmapSegment Asteroid20BitmapSegment{&Asteroid20Bitmap, {0, 0, 40, 40}};

	TextHelper ScoreText{};
	TextHelper HealthText{};

	std::shared_ptr<Spaceship> spaceship{};

	Arena arena;

public:
	DirectX2DHelper();

	explicit DirectX2DHelper(HWND hwnd);

	~DirectX2DHelper();

	void reloadTarget(HWND hwnd);

	void nextFrame();
};