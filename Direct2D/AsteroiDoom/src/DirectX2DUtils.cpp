#include "DirectX2DUtils.h"

#include <stdexcept>

using namespace D2D1;

DirectX2DHelper::DirectX2DHelper() = default;

DirectX2DHelper::DirectX2DHelper(HWND hwnd) :
    spaceship(std::make_shared<Spaceship>(
        SpaceshipSize,
        MovementData(),
        SpaceshipBitmapSegment,
        SpaceshipHitPoints,
        ThrusterData(SpaceshipDeceleration, SpaceshipThrust, SpaceshipTorque),
        SpaceshipGunOffset,
        SpaceshipGunCooldown,
        ProjectileBitmapSegment
    )),
    arena(ArenaWidth, ArenaHeight, SpawnAreaMargin, spaceship) {
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

	new (&SpaceshipBitmap) BitmapHelper(WICFactory, target, SpaceshipPath);
	new (&ProjectileBitmap) BitmapHelper(WICFactory, target, ProjectilePath);
	new (&Asteroid20Bitmap) BitmapHelper(WICFactory, target, Asteroid20Path);
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

	SpaceshipBitmap.reloadBitmap(target);
	ProjectileBitmap.reloadBitmap(target);
	Asteroid20Bitmap.reloadBitmap(target);
}

void DirectX2DHelper::nextFrame() {
	const static unsigned long long startTimestamp = GetTickCount64();
	static unsigned long long previousTimestamp = startTimestamp;
	static unsigned long long previousAsteroidSpawnTimestamp = startTimestamp;
	unsigned long long timestamp = GetTickCount64();
	unsigned long long timestampDiff = timestamp - previousTimestamp;
	previousTimestamp = timestamp;

	static unsigned long long score = 0;
	static bool gameOver = false;

	// DRAWING
	target->BeginDraw();
	target->Clear(ColorF(ColorF::Black));

	if (!gameOver) {
		target->SetTransform(ArenaTranslation);

		arena.move(timestampDiff);
		if (GetAsyncKeyState(VK_SPACE)) {
			arena.addProjectile(spaceship->shoot(timestamp));
		}
		if (timestamp - previousAsteroidSpawnTimestamp > AsteroidSpawnDelay) {
			arena.spawnAsteroid(20, Asteroid20BitmapSegment, 50, 50);
			previousAsteroidSpawnTimestamp = timestamp;
		}
		arena.draw();
		score += arena.checkCollisions();

		// Handling game over
		if (spaceship->destroyed()) {
			gameOver = true;
			wchar_t message[1024];
			swprintf(message, 1024, L"Your score is %llu.", score);
			int decision = MessageBox(target->GetHwnd(), message, L"Game over!", MB_RETRYCANCEL | MB_ICONEXCLAMATION);
			if (decision == IDRETRY) {
				score = 0;
				spaceship = std::make_shared<Spaceship>(
				    SpaceshipSize,
				    MovementData(),
				    SpaceshipBitmapSegment,
				    SpaceshipHitPoints,
				    ThrusterData(SpaceshipDeceleration, SpaceshipThrust, SpaceshipTorque),
				    SpaceshipGunOffset,
				    SpaceshipGunCooldown,
				    ProjectileBitmapSegment
				);
				arena = Arena(ArenaWidth, ArenaHeight, SpawnAreaMargin, spaceship);
				gameOver = false;
			} else {
				DestroyWindow(target->GetHwnd());
			}
		}
	}

	target->EndDraw();
}

DirectX2DHelper::~DirectX2DHelper() {
	target->Release();
	WICFactory->Release();
	D2DFactory->Release();
}
