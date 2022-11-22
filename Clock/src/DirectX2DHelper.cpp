#include "DirectX2DHelper.h"
#include <stdexcept>

using namespace D2D1;

DirectX2DHelper::DirectX2DHelper() = default;

DirectX2DHelper::DirectX2DHelper(HWND hwnd) {
	if (D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory) != S_OK || GetClientRect(hwnd, &windowSize) == 0 ||
	    factory->CreateHwndRenderTarget(
	        RenderTargetProperties(),
	        HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	        &target
	    ) != S_OK) {
		throw std::runtime_error("Failed to initialize DirectX2DHelper.");
	}
}

void DirectX2DHelper::reloadTarget(HWND hwnd) {
	if (target)
		target->Release();

	if (factory->CreateHwndRenderTarget(
	        RenderTargetProperties(),
	        HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	        &target
	    ) != S_OK ||
	    GetClientRect(hwnd, &windowSize) == 0) {
		throw std::runtime_error("Failed to reload target.");
	}
}

void DirectX2DHelper::draw(const bool smile, const D2D1_POINT_2F mousePosition) {
	target->BeginDraw();

	target->Clear(D2D1::ColorF(D2D1::ColorF::Purple));

	target->EndDraw();
}

DirectX2DHelper::~DirectX2DHelper() {
	target->Release();
	factory->Release();
}
