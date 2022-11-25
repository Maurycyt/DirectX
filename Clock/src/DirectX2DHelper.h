#pragma once

#include <d2d1_3.h>
#include <windowsx.h>
#include <wincodec.h>
#include <numbers>

class BitmapHelper {
	LPCWSTR path{};

	IWICBitmapDecoder * decoder{nullptr};
	IWICBitmapFrameDecode * source{nullptr};
	IWICFormatConverter * converter{nullptr};

	ID2D1Bitmap * bitmap{nullptr};
public:

	BitmapHelper();

	BitmapHelper(IWICImagingFactory * WICFactory, ID2D1HwndRenderTarget * target, LPCWSTR path);

	~BitmapHelper();

	void reloadBitmap(ID2D1HwndRenderTarget * target);

	ID2D1Bitmap * getBitmap();
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