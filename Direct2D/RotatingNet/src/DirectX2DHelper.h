#pragma once

#include <d2d1_3.h>

class DirectX2DHelper {
	ID2D1Factory * d2d1Factory {nullptr};
	ID2D1HwndRenderTarget * hwndRenderTarget {nullptr};
	RECT windowSize {};

public:
	static D2D1_COLOR_F white;
	static D2D1_COLOR_F darkBlue;

	DirectX2DHelper();

	explicit DirectX2DHelper(HWND hwnd);

	void reloadTarget(HWND hwnd);

	~DirectX2DHelper();

	RECT getWindowSize();

	void createSolidColorBrush(D2D1_COLOR_F color, ID2D1SolidColorBrush ** brush);

	void beginDraw();

	void clear(D2D1_COLOR_F color);

	void drawLine(D2D1_POINT_2F point0, D2D1_POINT_2F point1, ID2D1Brush * brush, float strokeWidth);

	void endDraw();
};