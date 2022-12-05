#include "BitmapUtils.h"

#include <stdexcept>

BitmapHelper::BitmapHelper() = default;

BitmapHelper::BitmapHelper(IWICImagingFactory * WICFactory, ID2D1HwndRenderTarget * renderTarget, LPCWSTR path) :
    path(path),
    target(renderTarget) {
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

BitmapSegment::BitmapSegment() = default;

BitmapSegment::BitmapSegment(BitmapHelper * bitmapHelper, D2D_RECT_F segmentRect) :
    bitmap(bitmapHelper),
    segment(segmentRect) {
}

D2D_RECT_F BitmapSegment::getCenteredRect() const {
	float halfWidth = (segment.right - segment.left) / 2;
	float halfHeight = (segment.bottom - segment.top) / 2;
	return {-halfWidth, -halfHeight, halfWidth, halfHeight};
}

void BitmapSegment::draw(
    D2D_POINT_2F translation, float rotation, float opacity, D2D1_BITMAP_INTERPOLATION_MODE interpolationMode
) const {
	using D2D1::Matrix3x2F;

	Matrix3x2F oldTransform{}, imageTransform{}, finalTransform{};
	bitmap->getTarget()->GetTransform(&oldTransform);
	imageTransform.SetProduct(Matrix3x2F::Rotation(rotation), Matrix3x2F::Translation(translation.x, translation.y));
	finalTransform.SetProduct(imageTransform, oldTransform);

	bitmap->getTarget()->SetTransform(finalTransform);
	bitmap->getTarget()->DrawBitmap(bitmap->getBitmap(), getCenteredRect(), opacity, interpolationMode, segment);
	bitmap->getTarget()->SetTransform(oldTransform);
}
