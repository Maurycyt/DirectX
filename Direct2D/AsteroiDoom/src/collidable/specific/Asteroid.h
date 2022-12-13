#pragma once

#include "../base/DamagableObject.h"
#include "../base/DamagingObject.h"

class Asteroid final : virtual public DamagableObject, virtual public DamagingObject {
public:
	Asteroid(
	    float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints, unsigned int damagePoints
	);
};
