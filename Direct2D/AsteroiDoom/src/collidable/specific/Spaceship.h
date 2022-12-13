#pragma once

#include "../base/DamagableObject.h"
#include "Projectile.h"

#include <memory>

class ThrusterData {
public:
	float deceleration{};
	float thrust{};
	float torque{};

	ThrusterData();

	ThrusterData(float deceleration, float thrust, float torque);
};

class Spaceship final : virtual public DamagableObject {
	ThrusterData thrusters{};
	float gunOffset{};
	unsigned long long gunCooldown{};
	unsigned long long previousShotTimestamp{};
	BitmapSegment projectileBitmapSegment{};

	MovementData getProjectileSpawnMovement();

public:
	Spaceship(
	    float size,
	    MovementData movement,
	    BitmapSegment bitmapSegment,
	    unsigned int hitPoints,
	    ThrusterData thrusters,
	    float gunOffset,
	    unsigned long long gunCooldown,
	    BitmapSegment projectileBitmapSegment
	);

	void move(unsigned int millis, D2D_RECT_F modulo) override;

	std::unique_ptr<Projectile> shoot(unsigned long long timestamp);
};