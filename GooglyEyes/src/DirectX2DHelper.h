#pragma once

#include <d2d1_3.h>
#include <numbers>

class DirectX2DHelper {
	// CONSTANTS
	static constexpr float pi = std::numbers::pi_v<float>;

	static constexpr float bodyGradientRadius = 400.;
	static constexpr float eyeVerticalOffset = 150.;
	static constexpr float eyeHorizontalOffset = 150.;
	static constexpr float eyeRadius = 100.;
	static constexpr float pupilRadius = 30.;

	static constexpr float bodyTop = -300.;
	static constexpr float bodyLeft = -250.;
	static constexpr float bodyRight = 250.;
	static constexpr float bodyBottom = 200.;

	static constexpr float mouthPendulumAmplitude = 15.;

	static constexpr float snoutLeft = -75.;
	static constexpr float snoutRight = 75.;
	static constexpr float snoutTop = -50.;
	static constexpr float snoutBottom = 75.;

	static constexpr float mouthLeft = -100.;
	static constexpr float mouthRight = 100.;
	static constexpr float mouthTop = 100.;
	static constexpr float mouthCenter = 125.;
	static constexpr float mouthBottom = 175.;

	// HELPFUL MEMBERS
	static constexpr int nBodyRadialStops = 2;
	D2D1_GRADIENT_STOP bodyRadialStops[nBodyRadialStops]{
	    {.0, D2D1::ColorF(D2D1::ColorF::Yellow)}, {1., D2D1::ColorF(D2D1::ColorF::Red)}};
	D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES bodyRadialProperties{
	    {0., 0.}, {0., 0.}, bodyGradientRadius, bodyGradientRadius};
	ID2D1GradientStopCollection * bodyRadialStopCollection{nullptr};
	ID2D1RadialGradientBrush * bodyRadialBrush{nullptr};

	static constexpr int nEyeRadialStops = 2;
	D2D1_GRADIENT_STOP eyeRadialStops[nEyeRadialStops]{
	    {.8f, D2D1::ColorF(D2D1::ColorF::White)}, {1.f, D2D1::ColorF(D2D1::ColorF::LightGray)}};
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

	void releasePathGeometries();

public:
	DirectX2DHelper();

	explicit DirectX2DHelper(HWND hwnd);

	~DirectX2DHelper();

	void reloadTarget(HWND hwnd);

	void draw(bool smile, D2D1_POINT_2F mousePosition);
};