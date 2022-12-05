#include "DamagableObject.h"

DamagableObject::DamagableObject(unsigned int hitPoints) : hitPoints(hitPoints) {
}

DamagableObject::DamagableObject(
    float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints
) :
    CollidableObject(size, movement, bitmapSegment),
    hitPoints(hitPoints) {
}

bool DamagableObject::takeDamage(unsigned int points) {
	points -= min(points, hitPoints);
	return destroyed();
}

bool DamagableObject::destroyed() const {
	return hitPoints == 0;
}
