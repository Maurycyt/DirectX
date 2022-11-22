#pragma once

#include <d2d1_3.h>
#include <numbers>

class DirectX2DHelper {
	// IMPORTANT MEMBERS
	ID2D1Factory * factory{nullptr};
	ID2D1HwndRenderTarget * target{nullptr};
	RECT windowSize{0, 0};

	ID2D1SolidColorBrush * solidBackgroundBrush{nullptr};

public:
	DirectX2DHelper();

	explicit DirectX2DHelper(HWND hwnd);

	~DirectX2DHelper();

	void reloadTarget(HWND hwnd);

	void draw(bool smile, D2D1_POINT_2F mousePosition);
};