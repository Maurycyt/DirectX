#include "WinMain.h"
#include <cmath>
#include <numbers>

DirectX2DHelper d2DHelper;
ID2D1SolidColorBrush * brush;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

const float pi = std::numbers::pi_v<float>;
const int NetResolution = 50;
const float ScalingFactor = 250.;
const float BrushWidth = 2.;
struct Point3 {
	float x, y, z;

	[[nodiscard]] Point3 scale(float factor) const {
		return {x * factor, y * factor, z * factor};
	}

	[[nodiscard]] Point3 translate(Point3 vector) const {
		return {x + vector.x, y + vector.y, z + vector.z};
	}

	[[nodiscard]] Point3 rotate(int axis, float angle) const {
		switch (axis) {
		case 0:
			return {x, y * std::cos(angle) - z * std::sin(angle), y * std::sin(angle) + z * std::cos(angle)};
		case 1:
			return {x * std::cos(angle) - z * std::sin(angle), y, x * std::sin(angle) + z * std::cos(angle)};
		default:
			return {x * std::cos(angle) - y * std::sin(angle), x * std::sin(angle) + y * std::cos(angle), z};
		}
	}
} basePoints[NetResolution + 1][NetResolution + 1];

unsigned long long startTickCount;

INT WINAPI wWinMain(
    _In_ [[maybe_unused]] HINSTANCE hInstance,
    _In_opt_ [[maybe_unused]] HINSTANCE hPrevInstance,
    _In_ [[maybe_unused]] PWSTR pCmdLine,
    _In_ [[maybe_unused]] INT nCmdShow
) {

	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = {};

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// Create the window.
	HWND hwnd = CreateWindowEx(
	    0,                   // Optional window styles.
	    CLASS_NAME,          // Window class
	    L"Rotating Net",     // Window text
	    WS_OVERLAPPEDWINDOW, // Window style

	    // Size and position
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,

	    nullptr,   // Parent window
	    nullptr,   // Menu
	    hInstance, // Instance handle
	    nullptr    // Additional application data
	);

	if (hwnd == nullptr) {
		return 0;
	}

	// Get base points
	for (int i = 0; i <= NetResolution; i++) {
		float x = float(i) * 2.f / NetResolution - 1.f;
		for (int j = 0; j <= NetResolution; j++) {
			float y = float(j) * 2.f / NetResolution - 1.f;
			float z = -std::cos(10 * std::sqrt(x * x + y * y)) / 4.f;
			basePoints[i][j] = {x, y, z};
		}
	}

	new (&d2DHelper) DirectX2DHelper(hwnd);
	ShowWindow(hwnd, nCmdShow);

	startTickCount = GetTickCount64();

	UINT_PTR IDT_TIMER1 = 1;
	SetTimer(hwnd, IDT_TIMER1, 15, nullptr);

	// Run the message loop.
	MSG msg = {};

	while (GetMessage(&msg, nullptr, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	float angle1, angle2;
	RECT windowSize;
	//	int decision;
	switch (uMsg) {
	case WM_CLOSE:
		// FIXME: Why does this hang?
		/*decision = MessageBox(
		    hwnd, L"Are you sure you want to quit?", L"Are you sure?", MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1
		);

		if (decision == IDOK) {
		  DestroyWindow(hwnd);
		}*/
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_SIZE:
		d2DHelper.reloadTarget(hwnd);
		return 0;

	case WM_TIMER:
		InvalidateRect(hwnd, nullptr, false);
		return 0;

	case WM_PAINT:
		windowSize = d2DHelper.getWindowSize();

		angle1 = float(float((GetTickCount64() - startTickCount) % 10000) / 10000. * 2 * pi);
		angle2 = float(float((GetTickCount64() - startTickCount) % 13500) / 13500. * 2 * pi);

		d2DHelper.createSolidColorBrush(DirectX2DHelper::white, &brush);

		d2DHelper.beginDraw();
		d2DHelper.clear(DirectX2DHelper::darkBlue);

		for (int i = 0; i <= NetResolution; i++) {
			for (int j = 0; j <= NetResolution; j++) {
				Point3 transformedPoint = basePoints[i][j]
				                              .rotate(2, angle1)
				                              .rotate(0, angle2)
				                              .scale(ScalingFactor)
				                              .translate({float(windowSize.right) / 2.f, float(windowSize.bottom) / 2.f, 0.f});
				float gradientGreen = transformedPoint.x / float(windowSize.right);
				gradientGreen = 2.f * gradientGreen - .5f;
				brush->SetColor(D2D1::ColorF(1., gradientGreen, .0));
				if (i > 0) {
					Point3 transformedOtherPoint =
					    basePoints[i - 1][j]
					        .rotate(2, angle1)
					        .rotate(0, angle2)
					        .scale(ScalingFactor)
					        .translate({float(windowSize.right) / 2.f, float(windowSize.bottom) / 2.f, 0.f});
					d2DHelper.drawLine(
					    {transformedOtherPoint.x, transformedOtherPoint.y},
					    {transformedPoint.x, transformedPoint.y},
					    brush,
					    BrushWidth
					);
				}
				if (j > 0) {
					Point3 transformedOtherPoint =
					    basePoints[i][j - 1]
					        .rotate(2, angle1)
					        .rotate(0, angle2)
					        .scale(ScalingFactor)
					        .translate({float(windowSize.right) / 2.f, float(windowSize.bottom) / 2.f, 0.f});
					d2DHelper.drawLine(
					    {transformedOtherPoint.x, transformedOtherPoint.y},
					    {transformedPoint.x, transformedPoint.y},
					    brush,
					    BrushWidth
					);
				}
			}
		}

		d2DHelper.endDraw();

		brush->Release();
		return 0;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}