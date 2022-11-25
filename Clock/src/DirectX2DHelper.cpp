#include "DirectX2DHelper.h"
#include <stdexcept>

using namespace D2D1;

BitmapHelper::BitmapHelper() = default;

BitmapHelper::BitmapHelper(IWICImagingFactory * WICFactory, ID2D1HwndRenderTarget * renderTarget, LPCWSTR path) :
    path(path), target(renderTarget) {
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

void BitmapHelper::reloadBitmap(ID2D1HwndRenderTarget * renderTarget) {
	if (bitmap)
		bitmap->Release();

	target = renderTarget;
	if (target->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap) != S_OK) {
		char message[1024];
		sprintf_s(message, "Failed to reload bitmap for path '%ls'", path);
		throw std::runtime_error(message);
	}
}

ID2D1Bitmap * BitmapHelper::getBitmap() {
	return bitmap;
}

ID2D1HwndRenderTarget * BitmapHelper::getTarget() {
	return target;
}

void BitmapHelper::draw(D2D_RECT_F destinationRect, float opacity, D2D1_BITMAP_INTERPOLATION_MODE interpolationMode) {
	target->DrawBitmap(bitmap, destinationRect, opacity, interpolationMode);
}

BitmapSegment::BitmapSegment() = default;

BitmapSegment::BitmapSegment(BitmapHelper * bitmapHelper, D2D_RECT_F segmentRect) :
    bitmap(bitmapHelper), segment(segmentRect) {
}

void BitmapSegment::draw(D2D_RECT_F destinationRect, float opacity, D2D1_BITMAP_INTERPOLATION_MODE interpolationMode) {
	bitmap->getTarget()->DrawBitmap(bitmap->getBitmap(), destinationRect, opacity, interpolationMode, segment);
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

	new (&clockBitmap) BitmapHelper(WICFactory, target, clockImagePath);
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

	clockBitmap.reloadBitmap(target);
}

void DirectX2DHelper::draw() {
	Matrix3x2F transform{};
	const Matrix3x2F translationToCentre{
	    Matrix3x2F::Translation(float(windowSize.right) / 2.f, float(windowSize.bottom) / 2.f)};

	target->BeginDraw();
	target->Clear(ColorF(ColorF::Purple));

	transform.SetProduct(Matrix3x2F::Rotation(-10.f), translationToCentre);
	target->SetTransform(transform);

	clockBitmap.draw(RectF(-385, -200, 385, 200));

	target->EndDraw();
}

DirectX2DHelper::~DirectX2DHelper() {
	target->Release();
	WICFactory->Release();
	D2DFactory->Release();
}
