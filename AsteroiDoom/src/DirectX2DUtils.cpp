#include "DirectX2DUtils.h"

#include <stdexcept>

using namespace D2D1;

DirectX2DHelper::DirectX2DHelper() = default;

DirectX2DHelper::DirectX2DHelper(HWND hwnd) {
	if (CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED) != S_OK ||
	    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&WICFactory)) != S_OK ||
	    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory) != S_OK ||
	    GetClientRect(hwnd, &windowSize) == 0 ||
	    D2DFactory->CreateHwndRenderTarget(
	        RenderTargetProperties(),
	        HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	        &target
	    ) != S_OK) {
		throw std::runtime_error("Failed to initialize DirectX2DHelper.");
	}

	new (&Asteroid20Bitmap) BitmapHelper(WICFactory, target, Asteroid20Path);

	arena.addAsteroid(
	    20, {{-ArenaWidth / 2 - 100, -ArenaHeight / 2 + 50}, 0, {100, -51}, 50}, Asteroid20BitmapSegment, 50, 50
	);
}

void DirectX2DHelper::reloadTarget(HWND hwnd) {
	if (target)
		target->Release();

	if (GetClientRect(hwnd, &windowSize) == 0 ||
	    D2DFactory->CreateHwndRenderTarget(
	        RenderTargetProperties(),
	        HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	        &target
	    ) != S_OK) {
		throw std::runtime_error("Failed to reload target.");
	}

	Asteroid20Bitmap.reloadBitmap(target);
}

void DirectX2DHelper::draw() {
	// DRAWING
	static unsigned long long previousTimestamp = GetTickCount64();
	unsigned long long timestamp = GetTickCount64();
	unsigned long long timestampDiff = timestamp - previousTimestamp;
	previousTimestamp = timestamp;

	target->BeginDraw();
	target->Clear(ColorF(ColorF::Black));

	target->SetTransform(ArenaTranslation);

	arena.move(timestampDiff);
	arena.draw();

	target->EndDraw();
}

DirectX2DHelper::~DirectX2DHelper() {
	target->Release();
	WICFactory->Release();
	D2DFactory->Release();
}
