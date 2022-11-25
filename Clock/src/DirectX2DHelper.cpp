#include "DirectX2DHelper.h"
#include <stdexcept>

using namespace D2D1;

BitmapHelper::BitmapHelper() = default;

BitmapHelper::BitmapHelper(IWICImagingFactory * WICFactory, ID2D1HwndRenderTarget * target, LPCWSTR path) : path(path) {
	if (WICFactory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder) !=
	        S_OK ||
	    decoder->GetFrame(0, &source) != S_OK || WICFactory->CreateFormatConverter(&converter) != S_OK ||
	    converter->Initialize(
	        source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut
	    ) != S_OK ||
	    (reloadBitmap(target), false)) {
		char message[1024];
		sprintf_s(message, "Failed to create bitmap for path '%ls'", path);
		throw std::runtime_error(message);
	}
}

BitmapHelper::~BitmapHelper() {
	if (decoder)
		decoder->Release();
	if (source)
		source->Release();
	if (converter)
		converter->Release();
	if (bitmap)
		bitmap->Release();
}

void BitmapHelper::reloadBitmap(ID2D1HwndRenderTarget * target) {
	if (bitmap)
		bitmap->Release();

	if (target->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap) != S_OK) {
		char message[1024];
		sprintf_s(message, "Failed to reload bitmap for path '%ls'", path);
		throw std::runtime_error(message);
	}
}

ID2D1Bitmap * BitmapHelper::getBitmap() {
	return bitmap;
}

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

	new(&clockBitmap) BitmapHelper(WICFactory, target, clockImagePath);
}

void DirectX2DHelper::reloadTarget(HWND hwnd) {
	if (target)
		target->Release();

	if (D2DFactory->CreateHwndRenderTarget(
	        RenderTargetProperties(),
	        HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	        &target
	    ) != S_OK ||
	    GetClientRect(hwnd, &windowSize) == 0) {
		throw std::runtime_error("Failed to reload target.");
	}

	clockBitmap.reloadBitmap(target);
}

void DirectX2DHelper::draw() {
	//	static D2D1::Matrix3x2F transform{};
	const D2D1::Matrix3x2F translationToCentre{
	    D2D1::Matrix3x2F::Translation(float(windowSize.right) / 2.f, float(windowSize.bottom) / 2.f)};

	target->BeginDraw();
	target->Clear(D2D1::ColorF(D2D1::ColorF::Purple));

	target->SetTransform(translationToCentre);

	target->DrawBitmap(clockBitmap.getBitmap(), RectF(-385, -200, 385, 200));

	target->EndDraw();
}

DirectX2DHelper::~DirectX2DHelper() {
	target->Release();
	WICFactory->Release();
	D2DFactory->Release();
}
