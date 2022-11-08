#pragma once

#include <d2d1_3.h>
#include <numbers>

class DirectX2DHelper {
	// CONSTANTS
	const float pi = std::numbers::pi_v<float>;

	const float bodyGradientRadius = 400.;
	const float eyeVerticalOffset = 150.;
	const float eyeHorizontalOffset = 150.;
	const float eyeRadius = 100.;
	const float pupilRadius = 30.;

	const float bodyTop = -300.;
	const float bodyLeft = -250.;
	const float bodyRight = 250.;
	const float bodyBottom = 200.;

	const float mouthPendulumAmplitude = 15.;

	const float snoutLeft = -75.;
	const float snoutRight = 75.;
	const float snoutTop = -50.;
	const float snoutBottom = 75.;

	const float mouthLeft = -100.;
	const float mouthRight = 100.;
	const float mouthTop = 100.;
	const float mouthCenter = 125.;
	const float mouthBottom = 175.;

	// HELPFUL MEMBERS
	static const int nBodyRadialStops = 2;
	D2D1_GRADIENT_STOP bodyRadialStops[nBodyRadialStops]{
	    {.0, D2D1::ColorF(D2D1::ColorF::Yellow)}, {1., D2D1::ColorF(D2D1::ColorF::Red)}};
	D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES bodyRadialProperties{
	    {0., 0.}, {0., 0.}, bodyGradientRadius, bodyGradientRadius};
	ID2D1GradientStopCollection * bodyRadialStopCollection{nullptr};
	ID2D1RadialGradientBrush * bodyRadialBrush{nullptr};

	static const int nEyeRadialStops = 2;
	D2D1_GRADIENT_STOP eyeRadialStops[nEyeRadialStops]{
	    {.8, D2D1::ColorF(D2D1::ColorF::White)}, {1., D2D1::ColorF(D2D1::ColorF::LightGray)}};
	D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES eyeRadialProperties{{0., 0.}, {0., 0.}, eyeRadius, eyeRadius};
	ID2D1GradientStopCollection * eyeRadialStopCollection{nullptr};
	ID2D1RadialGradientBrush * eyeRadialBrush{nullptr};

	ID2D1PathGeometry * bodyPath{nullptr};
	ID2D1GeometrySink * bodyPathSink{nullptr};

	ID2D1SolidColorBrush * solidBrushSnout{nullptr};
	ID2D1PathGeometry * snoutPath{nullptr};
	ID2D1GeometrySink * snoutPathSink{nullptr};

	ID2D1PathGeometry * frownPath{nullptr};
	ID2D1GeometrySink * frownPathSink{nullptr};

	ID2D1PathGeometry * smilePath{nullptr};
	ID2D1GeometrySink * smilePathSink{nullptr};

	// IMPORTANT MEMBERS
	ID2D1Factory * factory{nullptr};
	ID2D1HwndRenderTarget * target{nullptr};
	RECT windowSize{0, 0};

	ID2D1SolidColorBrush * solidBrushBlack{nullptr};

	void initializeBrushes();

	void releaseBrushes();

	void initializePathGeometries();

	void releaseGeometries();

public:
	DirectX2DHelper();

	explicit DirectX2DHelper(HWND hwnd);

	~DirectX2DHelper();

	void reloadTarget(HWND hwnd);

	void draw(bool smile, D2D1_POINT_2F mousePosition);
};