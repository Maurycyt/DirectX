#pragma once

#include "CollidableObject.h"
#include "DamagableObject.h"

class DamagingObject : virtual public CollidableObject {
	unsigned int damagePoints;

protected:
	explicit DamagingObject(unsigned int damagePoints);

public:
	DamagingObject(float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int damagePoints);

	[[nodiscard]] unsigned int getDamagePoints() const;

	void dealDamage(DamagableObject & victim) const;
};
