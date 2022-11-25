#pragma once

#include <d2d1_3.h>
#include <numbers>
#include <wincodec.h>
#include <windowsx.h>

class BitmapHelper {
	LPCWSTR path{};

	ID2D1HwndRenderTarget * target{nullptr};
	IWICBitmapDecoder * decoder{nullptr};
	IWICBitmapFrameDecode * source{nullptr};
	IWICFormatConverter * converter{nullptr};

	ID2D1Bitmap * bitmap{nullptr};

public:
	BitmapHelper();

	BitmapHelper(IWICImagingFactory * WICFactory, ID2D1HwndRenderTarget * renderTarget, LPCWSTR path);

	~BitmapHelper();

	void reloadBitmap(ID2D1HwndRenderTarget * target);

	ID2D1Bitmap * getBitmap();

	ID2D1HwndRenderTarget * getTarget();

	void draw(
	    D2D_RECT_F destinationRect,
	    float opacity = 1.f,
	    D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
	);
};

class BitmapSegment {
	BitmapHelper * bitmap{nullptr};
	D2D_RECT_F segment{};

public:
	BitmapSegment();

	BitmapSegment(BitmapHelper * bitmapHelper, D2D_RECT_F segmentRect);

	void draw(
	    D2D_RECT_F destinationRect,
	    float opacity = 1.f,
	    D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
	);
};

class DirectX2DHelper {
	// HELPER MEMBERS
	static constexpr LPCWSTR clockImagePath = L"../assets/Clock.png";
	static constexpr LPCWSTR digitsImageUri = L"../assets/Digits.png";
	static constexpr int numDigits = 10;

	BitmapHelper clockBitmap;
	ID2D1Bitmap * colonBitmap{nullptr};
	ID2D1Bitmap * digitBitmaps[numDigits]{};

	// IMPORTANT MEMBERS
	IWICImagingFactory * WICFactory{nullptr};
	ID2D1Factory * D2DFactory{nullptr};
	ID2D1HwndRenderTarget * target{nullptr};
	RECT windowSize{0, 0};

public:
	DirectX2DHelper();

	explicit DirectX2DHelper(HWND hwnd);

	~DirectX2DHelper();

	void reloadTarget(HWND hwnd);

	void draw();
};