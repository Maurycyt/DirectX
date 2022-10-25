#include "DirectX2DHelper.h"
#include <exception>

using namespace D2D1;

DirectX2DHelper::DirectX2DHelper() = default;

DirectX2DHelper::DirectX2DHelper(HWND hwnd) {
	if (D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1Factory) != S_OK) {
		throw std::exception("DirectX2DHelper construction failed due to D2D1 Factory creation failure.");
	}

	if (GetClientRect(hwnd, &windowSize) == 0) {
		d2d1Factory->Release();
		throw std::exception("DirectX2DHelper construction failed due to GetClientRect failure.");
	}

	if (d2d1Factory->CreateHwndRenderTarget(
	        RenderTargetProperties(),
	        HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	        &hwndRenderTarget
	    ) != S_OK) {
		d2d1Factory->Release();
		throw std::exception("DirectX2DHelper construction failed due to CreateHwndRenderTarget failure.");
	}
}

void DirectX2DHelper::reloadTarget(HWND hwnd) {
	if(hwndRenderTarget) {
		hwndRenderTarget->Release();
	}

	if (GetClientRect(hwnd, &windowSize) == 0) {
		d2d1Factory->Release();
		throw std::exception("DirectX2DHelper target reload failed due to GetClientRect failure.");
	}

	if (d2d1Factory->CreateHwndRenderTarget(
	        RenderTargetProperties(),
	        HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	        &hwndRenderTarget
	    ) != S_OK) {
		d2d1Factory->Release();
		throw std::exception("DirectX2DHelper target reload failed due to CreateHwndRenderTarget failure.");
	}
}

DirectX2DHelper::~DirectX2DHelper() {
	d2d1Factory->Release();
	hwndRenderTarget->Release();
}

RECT DirectX2DHelper::getWindowSize() {
	return windowSize;
}

void DirectX2DHelper::createSolidColorBrush(D2D1_COLOR_F color, ID2D1SolidColorBrush ** brush) {
	hwndRenderTarget->CreateSolidColorBrush(color, brush);
}

void DirectX2DHelper::beginDraw() {
	hwndRenderTarget->BeginDraw();
}

void DirectX2DHelper::clear(D2D1_COLOR_F color) {
	hwndRenderTarget->Clear(color);
}

void DirectX2DHelper::drawLine(D2D1_POINT_2F point0, D2D1_POINT_2F point1, ID2D1Brush * brush, float strokeWidth) {
	hwndRenderTarget->DrawLine(point0, point1, brush, strokeWidth, nullptr);
}

void DirectX2DHelper::endDraw() {
	hwndRenderTarget->EndDraw();
}

D2D1_COLOR_F DirectX2DHelper::white = ColorF(1., 1., 1.);
D2D1_COLOR_F DirectX2DHelper::darkBlue = ColorF(.0, .0, .5);
