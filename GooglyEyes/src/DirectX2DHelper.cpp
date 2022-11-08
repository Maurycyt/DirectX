#include "DirectX2DHelper.h"
#include <cmath>

using namespace D2D1;

void DirectX2DHelper::initializeBrushes() {
	target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &solidBrushBlack);
	target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::SaddleBrown), &solidBrushSnout);
	target->CreateGradientStopCollection(bodyRadialStops, nBodyRadialStops, &bodyRadialStopCollection);
	target->CreateRadialGradientBrush(bodyRadialProperties, bodyRadialStopCollection, &bodyRadialBrush);
	target->CreateGradientStopCollection(eyeRadialStops, nEyeRadialStops, &eyeRadialStopCollection);
	target->CreateRadialGradientBrush(eyeRadialProperties, eyeRadialStopCollection, &eyeRadialBrush);
}

void DirectX2DHelper::releaseBrushes() {
	solidBrushBlack->Release();
	solidBrushSnout->Release();
	bodyRadialBrush->Release();
	eyeRadialBrush->Release();
}

void DirectX2DHelper::initializePathGeometries() {
	factory->CreatePathGeometry(&bodyPath);
	bodyPath->Open(&bodyPathSink);
	bodyPathSink->BeginFigure({bodyLeft * .8f, bodyTop * .8f}, D2D1_FIGURE_BEGIN_FILLED);
	bodyPathSink->AddBezier({{bodyLeft * .4f, bodyTop}, {bodyRight * .4f, bodyTop}, {bodyRight * .8f, bodyTop * .8f}});
	bodyPathSink->AddBezier({{bodyRight, bodyTop}, {bodyRight * 1.2f, bodyTop * .8f}, {bodyRight, bodyTop *.5f}});
	bodyPathSink->AddBezier({{bodyRight, bodyBottom * .5f}, {bodyRight * .5f, bodyBottom}, {0., bodyBottom}});
	bodyPathSink->AddBezier({{bodyLeft * .5f, bodyBottom}, {bodyLeft, bodyBottom * .5f}, {bodyLeft, bodyTop * .5f}});
	bodyPathSink->AddBezier({{bodyLeft * 1.2f, bodyTop * .8f}, {bodyLeft, bodyTop}, {bodyLeft * .8f, bodyTop *.8f}});
	bodyPathSink->EndFigure(D2D1_FIGURE_END_OPEN);
	bodyPathSink->Close();
	
	factory->CreatePathGeometry(&snoutPath);
	snoutPath->Open(&snoutPathSink);
	snoutPathSink->BeginFigure({0., snoutTop}, D2D1_FIGURE_BEGIN_FILLED);
	snoutPathSink->AddBezier({{snoutRight, snoutTop}, {snoutRight, snoutTop * .25f}, {snoutRight, .0}});
	snoutPathSink->AddBezier({{snoutRight, snoutBottom * .25f}, {snoutRight * .5f, snoutBottom}, {.0, snoutBottom}});
	snoutPathSink->AddBezier({{snoutLeft * .5f, snoutBottom}, {snoutLeft, snoutBottom * .25f}, {snoutLeft, 0.}});
	snoutPathSink->AddBezier({{snoutLeft, snoutTop * .25f}, {snoutLeft, snoutTop}, {0., snoutTop}});
	snoutPathSink->EndFigure(D2D1_FIGURE_END_CLOSED);
	snoutPathSink->Close();
	
	factory->CreatePathGeometry(&frownPath);
	frownPath->Open(&frownPathSink);
	frownPathSink->BeginFigure({mouthLeft, mouthCenter}, D2D1_FIGURE_BEGIN_HOLLOW);
	frownPathSink->AddBezier({{mouthLeft * .5f, mouthTop}, {mouthRight * .5f, mouthTop}, {mouthRight, mouthCenter}});
	frownPathSink->EndFigure(D2D1_FIGURE_END_OPEN);
	frownPathSink->Close();
	
	factory->CreatePathGeometry(&smilePath);
	smilePath->Open(&smilePathSink);
	smilePathSink->BeginFigure({mouthLeft, mouthCenter}, D2D1_FIGURE_BEGIN_HOLLOW);
	smilePathSink->AddBezier({{mouthLeft * .5f, mouthBottom}, {mouthRight * .5f, mouthBottom}, {mouthRight, mouthCenter}});
	smilePathSink->EndFigure(D2D1_FIGURE_END_OPEN);
	smilePathSink->Close();
}

void DirectX2DHelper::releaseGeometries() {
	snoutPath->Release();
	snoutPathSink->Release();
	frownPath->Release();
	frownPathSink->Release();
	smilePath->Release();
	smilePathSink->Release();
}

DirectX2DHelper::DirectX2DHelper() = default;

DirectX2DHelper::DirectX2DHelper(HWND hwnd) {
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
	GetClientRect(hwnd, &windowSize);
	factory->CreateHwndRenderTarget(
	    RenderTargetProperties(),
	    HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	    &target
	);

	initializeBrushes();
	initializePathGeometries();
}

void DirectX2DHelper::reloadTarget(HWND hwnd) {
	if (target) {
		target->Release();
	}

	GetClientRect(hwnd, &windowSize);
	factory->CreateHwndRenderTarget(
	    RenderTargetProperties(),
	    HwndRenderTargetProperties(hwnd, {UINT32(windowSize.right), UINT32(windowSize.bottom)}),
	    &target
	);

	releaseBrushes();
	initializeBrushes();
}

void DirectX2DHelper::draw(const bool smile, const D2D1_POINT_2F mousePosition) {
	// CONSTANTS
	static D2D1::Matrix3x2F transform{};
	const D2D1::Matrix3x2F translationToCentre{
	    D2D1::Matrix3x2F::Translation(float(windowSize.right) / 2.f, float(windowSize.bottom) / 2.f)};

	// DRAWING
	target->BeginDraw();
	target->SetTransform(translationToCentre);
	target->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));

	// Draw body
	target->FillGeometry(bodyPath, bodyRadialBrush);
	target->DrawGeometry(bodyPath, solidBrushBlack, 4.f);

	// Draw eyes
	// Draw left eye
	D2D1_POINT_2F lookVector;
	float lookVectorLength;
	transform.SetProduct(translationToCentre, D2D1::Matrix3x2F::Translation(-eyeHorizontalOffset, -eyeVerticalOffset));
	target->SetTransform(transform);
	target->FillEllipse(D2D1::Ellipse({0., 0.}, eyeRadius, eyeRadius), eyeRadialBrush);
	target->DrawEllipse(D2D1::Ellipse({0., 0.}, eyeRadius, eyeRadius), solidBrushBlack, 4.);
	lookVector = {
	    mousePosition.x - (float(windowSize.right) / 2.f - eyeHorizontalOffset),
	    mousePosition.y - (float(windowSize.bottom) / 2.f - eyeVerticalOffset)};
	lookVectorLength = float(sqrt(double(lookVector.x * lookVector.x + lookVector.y * lookVector.y)));
	if (lookVectorLength > eyeRadius - pupilRadius) {
		lookVector.x *= (eyeRadius - pupilRadius) / lookVectorLength;
		lookVector.y *= (eyeRadius - pupilRadius) / lookVectorLength;
	}
	target->FillEllipse(D2D1::Ellipse(lookVector, pupilRadius, pupilRadius), solidBrushBlack);

	// Draw right eye
	transform.SetProduct(translationToCentre, D2D1::Matrix3x2F::Translation(eyeHorizontalOffset, -eyeVerticalOffset));
	target->SetTransform(transform);
	target->FillEllipse(D2D1::Ellipse({0., 0.}, eyeRadius, eyeRadius), eyeRadialBrush);
	target->DrawEllipse(D2D1::Ellipse({0., 0.}, eyeRadius, eyeRadius), solidBrushBlack, 4.);
	lookVector = {
	    mousePosition.x - (float(windowSize.right) / 2.f + eyeHorizontalOffset),
	    mousePosition.y - (float(windowSize.bottom) / 2.f - eyeVerticalOffset)};
	lookVectorLength = float(sqrt(double(lookVector.x * lookVector.x + lookVector.y * lookVector.y)));
	if (lookVectorLength > eyeRadius - pupilRadius) {
		lookVector.x *= (eyeRadius - pupilRadius) / lookVectorLength;
		lookVector.y *= (eyeRadius - pupilRadius) / lookVectorLength;
	}
	target->FillEllipse(D2D1::Ellipse(lookVector, pupilRadius, pupilRadius), solidBrushBlack);

	// Draw snout and mouth
	// Rotation setup
	float angle = mouthPendulumAmplitude * std::sin(float(GetTickCount64() % 2000) / 1000.f * pi);
	transform.SetProduct(D2D1::Matrix3x2F::Rotation(angle), translationToCentre);
	target->SetTransform(transform);

	// Draw snout
	target->FillGeometry(snoutPath, solidBrushSnout);
	target->DrawGeometry(snoutPath, solidBrushBlack, 4.);

	// Draw mouth
	if (smile) {
		target->DrawGeometry(smilePath, solidBrushBlack, 4.);
	} else {
		target->DrawGeometry(frownPath, solidBrushBlack, 4.);
	}

	// CLEANUP
	target->EndDraw();
}

DirectX2DHelper::~DirectX2DHelper() {
	releaseBrushes();
	releaseGeometries();
	
	factory->Release();
	target->Release();
}
