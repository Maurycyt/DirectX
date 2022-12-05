#pragma once

#define WIN32_LEAN_AND_MEAN

#include "AsteroiDoomConstants.h"
#include "collidable/Arena.h"

#include <d2d1_3.h>
#include <numbers>
#include <wincodec.h>

class DirectX2DHelper {
	// IMPORTANT MEMBERS
	IWICImagingFactory * WICFactory{nullptr};
	ID2D1Factory * D2DFactory{nullptr};
	ID2D1HwndRenderTarget * target{nullptr};
	RECT windowSize{0, 0};

	Arena arena{ArenaWidth, ArenaHeight, SpawnAreaMargin, nullptr};

	BitmapHelper Asteroid20Bitmap{};
	BitmapSegment Asteroid20BitmapSegment{&Asteroid20Bitmap, {0, 0, 40, 40}};

public:
	DirectX2DHelper();

	explicit DirectX2DHelper(HWND hwnd);

	~DirectX2DHelper();

	void reloadTarget(HWND hwnd);

	void draw();
};