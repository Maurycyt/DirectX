#include "WinMain.h"

namespace {
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	POINT cursorPos;
	RECT desktop;

	DirectX3DHelper d3DHelper;
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
	ThrowIfFailed(!GetClientRect(GetDesktopWindow(), &desktop));
	HWND hwnd = CreateWindowEx(
	    0,                   // Optional window styles.
	    CLASS_NAME,          // Window class
	    L"3DScene",          // Window text
	    WS_POPUP,            // Window style

	    // Size and position
	    0,
	    0,
	    desktop.right,
	    desktop.bottom,

	    nullptr,   // Parent window
	    nullptr,   // Menu
	    hInstance, // Instance handle
	    nullptr    // Additional application data
	);

	if (hwnd == nullptr) {
		return 0;
	}

	new (&d3DHelper) DirectX3DHelper(hwnd);
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
		try {
			switch (uMsg) {
			case WM_CREATE:
				SetCursorPos(desktop.right / 2, desktop.bottom / 2);
				return 0;

			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;

			case WM_TIMER:
				InvalidateRect(hwnd, nullptr, false);
				return 0;

			case WM_MOUSEMOVE:
				SetCursor(nullptr);
				return 0;

			case WM_KEYDOWN:
				if (wParam == VK_ESCAPE) {
					DestroyWindow(hwnd);
				}
				return 0;

			case WM_ACTIVATE:
				if (wParam == WA_INACTIVE) {
					d3DHelper.deactivate();
				} else {
					SetCursorPos(desktop.right / 2, desktop.bottom / 2);
					d3DHelper.activate();
				}
				return 0;

			case WM_PAINT:
				ThrowIfFailed(!GetCursorPos(&cursorPos));
				d3DHelper.draw(cursorPos);
				ValidateRect(hwnd, nullptr);
				return 0;

			default:
				return DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
		} catch (std::exception & e) {
			FatalAppExitA(0, e.what());
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
} // namespace