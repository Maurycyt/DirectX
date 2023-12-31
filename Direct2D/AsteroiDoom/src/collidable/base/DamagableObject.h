#pragma once

#include "CollidableObject.h"

class DamagableObject : virtual public CollidableObject {
	unsigned int hitPoints;

protected:
	explicit DamagableObject(unsigned int hitPoints);

public:
	DamagableObject(float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints);

	bool takeDamage(unsigned int points);

	[[nodiscard]] unsigned int getHitPoints() const;

	[[nodiscard]] bool destroyed() const;

	unsigned int pointsForDestruction();
};
