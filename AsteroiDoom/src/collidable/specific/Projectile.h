#pragma once

#include "../base/DamagingObject.h"

class Projectile final : virtual public DamagingObject {
public:
	Projectile(float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int damagePoints);
};
