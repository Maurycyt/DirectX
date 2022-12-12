#pragma once

#define WIN32_LEAN_AND_MEAN

#include <d2d1_3.h>
#include <wincodec.h>

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
};

class BitmapSegment {
	BitmapHelper * bitmap{nullptr};
	D2D_RECT_F segment{};

	[[nodiscard]] D2D_RECT_F getCenteredRect() const;

public:
	BitmapSegment();

	BitmapSegment(BitmapHelper * bitmapHelper, D2D_RECT_F segmentRect);

	void draw(
	    D2D_POINT_2F translation,
	    float rotation,
	    float opacity = 1.f,
	    D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
	) const;
};
