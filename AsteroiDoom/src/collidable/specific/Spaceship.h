#pragma once

#include "../base/DamagableObject.h"

class Spaceship final : virtual public DamagableObject {
public:
	Spaceship(float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints);
};