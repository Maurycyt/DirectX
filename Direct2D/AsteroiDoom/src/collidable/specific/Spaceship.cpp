#include "Spaceship.h"

#include "../../utils/AsteroiDoomConstants.h"

#include <cmath>

namespace {
	void rotatePair(float & x, float & y, float angle) {
		angle *= RadiansInDegree;
		float rx = x * cos(angle) - y * sin(angle);
		float ry = x * sin(angle) + y * cos(angle);
		x = rx, y = ry;
	}

	enum class Side {
		Left,
		Right
	};
} // namespace

ThrusterData::ThrusterData() = default;

ThrusterData::ThrusterData(float deceleration, float thrust, float torque) :
    deceleration(deceleration),
    thrust(thrust),
    torque(torque) {
}

Spaceship::Spaceship(
    float size,
    MovementData movement,
    BitmapSegment bitmapSegment,
    unsigned int hitPoints,
    ThrusterData thrusters,
    float gunOffset,
    unsigned long long gunCooldown,
    BitmapSegment projectileBitmapSegment
) :
    CollidableObject(size, movement, bitmapSegment),
    DamagableObject(hitPoints),
    thrusters(thrusters),
    gunOffset(gunOffset),
    gunCooldown(gunCooldown),
    projectileBitmapSegment(projectileBitmapSegment) {
}

void Spaceship::move(unsigned int millis, D2D_RECT_F modulo) {
	float seconds = float(millis) / 1000;
	float boost = 1;

	// decelerate
	float decelerationFactor = std::pow(thrusters.deceleration, seconds);
	movement.velocity.x *= decelerationFactor;
	movement.velocity.y *= decelerationFactor;
	movement.spin *= decelerationFactor;

	// accelerate
	if (GetAsyncKeyState(VK_SHIFT)) {
		boost *= 2;
	}
	if (GetAsyncKeyState('W')) {
		movement.velocity.x += thrusters.thrust * sin(movement.rotation * RadiansInDegree) * seconds * boost;
		movement.velocity.y += thrusters.thrust * -cos(movement.rotation * RadiansInDegree) * seconds * boost;
	}
	if (GetAsyncKeyState('S')) {
		movement.velocity.x -= thrusters.thrust * sin(movement.rotation * RadiansInDegree) * seconds / 2 * boost;
		movement.velocity.y -= thrusters.thrust * -cos(movement.rotation * RadiansInDegree) * seconds / 2 * boost;
	}
	if (GetAsyncKeyState('D')) {
		movement.spin += thrusters.torque * seconds * boost;
	}
	if (GetAsyncKeyState('A')) {
		movement.spin -= thrusters.torque * seconds * boost;
	}

	CollidableObject::move(millis, modulo);
}

MovementData Spaceship::getProjectileSpawnMovement() {
	static Side side{};
	side = Side((int(side) + 1) % 2);

	MovementData result{};

	// Movement relative to the spaceship
	switch (side) {
	case Side::Left:
		result.location.x = -gunOffset;
		result.spin = 60;
		result.velocity.x = 10;
		break;
	case Side::Right:
		result.location.x = gunOffset;
		result.spin = -60;
		result.velocity.x = -10;
		break;
	}
	result.velocity.y = -ProjectileSpeed;
	result.location.y = -size - 10;

	// Apply spaceship rotation, then translation
	rotatePair(result.location.x, result.location.y, movement.rotation);
	rotatePair(result.velocity.x, result.velocity.y, movement.rotation);
	result.rotation += movement.rotation;

	result.location.x += movement.location.x;
	result.location.y += movement.location.y;

	return result;
}

std::unique_ptr<Projectile> Spaceship::shoot(unsigned long long timestamp) {
	if (timestamp - previousShotTimestamp < gunCooldown) {
		return nullptr;
	}
	previousShotTimestamp = timestamp;
	MovementData projectileMovement = getProjectileSpawnMovement();
	return std::make_unique<Projectile>(5, projectileMovement, projectileBitmapSegment, 25);
}
