// WinMain.cpp
#include "WinMain.h"
#include <iostream>

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT WINAPI wWinMain(_In_ [[maybe_unused]] HINSTANCE hInstance,
                    _In_opt_ [[maybe_unused]] HINSTANCE hPrevInstance,
                    _In_ [[maybe_unused]] PWSTR pCmdLine,
                    _In_ [[maybe_unused]] INT nCmdShow) {

  std::cout << "Hello world!\n";

  // Register the window class.
  auto CLASS_NAME = L"Sample Window Class";

  WNDCLASS wc = {};

  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;

  RegisterClass(&wc);

  // Create the window.

  HWND hWnd =
      CreateWindowEx(0,                   // Optional window styles.
                     CLASS_NAME,          // Window class
                     L"Bouncing Balls",   // Window text
                     WS_OVERLAPPEDWINDOW, // Window style

                     // Size and position
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                     nullptr,   // Parent window
                     nullptr,   // Menu
                     hInstance, // Instance handle
                     nullptr    // Additional application data
      );

  if (hWnd == nullptr) {
    return 0;
  }

  // Run the message loop.

  ShowWindow(hWnd, nCmdShow);

  UINT_PTR IDT_TIMER1 = 1;
  SetTimer(hWnd, IDT_TIMER1, 15, (TIMERPROC) nullptr);

  MSG msg = {};
  //	do {
  //		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
  //			if (msg.message != WM_QUIT) {
  //				TranslateMessage(&msg);
  //				DispatchMessage(&msg);
  //			}
  //		}
  //		else {
  //			// Miejsce rysowania kolejnych klatek animacji
  //			InvalidateRect(hWnd, nullptr, false);
  //		}
  //	} while (msg.message != WM_QUIT);

  while (GetMessage(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

double square(double argument) { return argument * argument; }

int parabolicHeight(double period, double maxHeight, double argument) {
  int result = int((-square(argument - period / 2) / square(period / 2) + 1) *
                   maxHeight);
  return result;
}

[[maybe_unused]] void drawCirclesTickCount(HDC hdc, RECT windowSize,
                                           int maxLineHeight,
                                           int maxBounceHeight) {
  const static int period = 2000;
  int time = int(GetTickCount64() % period);

  int numberOfCircles = 12;
  int radius = 40;
  int leftCircleBound = windowSize.right / 2 - numberOfCircles * radius;
  for (int circleID = 0; circleID < numberOfCircles; circleID++) {
    Ellipse(hdc, leftCircleBound + circleID * 2 * radius,
            windowSize.bottom - maxLineHeight - 2 * radius -
                parabolicHeight(period, maxBounceHeight,
                                (time + circleID * 125) % period),
            leftCircleBound + (circleID + 1) * 2 * radius,
            windowSize.bottom - maxLineHeight -
                parabolicHeight(period, maxBounceHeight,
                                (time + circleID * 125) % period));
  }
}

void drawCirclesTimer(HDC hdc, RECT windowSize, int maxLineHeight,
                      int maxBounceHeight) {
  const static int period = 120;
  static int frame = 0;
  frame = (frame + 1) % period;

  int numberOfCircles = 12;
  int radius = 40;
  int leftCircleBound = windowSize.right / 2 - numberOfCircles * radius;
  for (int circleID = 0; circleID < numberOfCircles; circleID++) {
    Ellipse(hdc, leftCircleBound + circleID * 2 * radius,
            windowSize.bottom - maxLineHeight - 2 * radius -
                parabolicHeight(period, maxBounceHeight,
                                (frame + circleID * 8) % period),
            leftCircleBound + (circleID + 1) * 2 * radius,
            windowSize.bottom - maxLineHeight -
                parabolicHeight(period, maxBounceHeight,
                                (frame + circleID * 8) % period));
  }
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  int decision;
  switch (uMsg) {
  case WM_CLOSE:
    decision =
        MessageBox(hWnd, L"Are you sure you want to quit?", L"Are you sure?",
                   MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2);
    if (decision == IDOK) {
      DestroyWindow(hWnd);
    }
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_TIMER:
  case WM_SIZE:
    InvalidateRect(hWnd, nullptr, false);
    return 0;

  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOWFRAME));

    // All painting occurs here, between BeginPaint and EndPaint.
    RECT windowSize;
    GetClientRect(hWnd, &windowSize);

    // Draw lines
    int maxLineHeight = 200;
    int startLineHeight = maxLineHeight / 2;
    for (int height = startLineHeight; true;
         height = maxLineHeight - (maxLineHeight - height) / 2) {
      POINT points[2] = {{windowSize.left, windowSize.bottom - height},
                         {windowSize.right, windowSize.bottom - height}};
      Polyline(hdc, points, 2);
      if (height == maxLineHeight) {
        break;
      }
    }

    int maxBounceHeight = 300;
    //			drawCirclesTickCount(hdc, windowSize, maxLineHeight,
    //maxBounceHeight);
    drawCirclesTimer(hdc, windowSize, maxLineHeight, maxBounceHeight);

    EndPaint(hWnd, &ps);
  }
    return 0;

  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
}
