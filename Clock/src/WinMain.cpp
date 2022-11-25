#include "WinMain.h"
#include <numbers>

namespace {
	DirectX2DHelper d2DHelper;

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
} // namespace

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
	    L"Clock",            // Window text
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

	new (&d2DHelper) DirectX2DHelper(hwnd);
	ShowWindow(hwnd, nCmdShow);

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

namespace {
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_SIZE:
			d2DHelper.reloadTarget(hwnd);
			return 0;

		case WM_TIMER:
			InvalidateRect(hwnd, nullptr, false);
			return 0;

		case WM_MOUSEMOVE:
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
			return 0;

		case WM_PAINT: {
			d2DHelper.draw();
			ValidateRect(hwnd, nullptr);
			return 0;
		}

		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
} // namespace