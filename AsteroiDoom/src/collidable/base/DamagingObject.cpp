#include "DamagingObject.h"

DamagingObject::DamagingObject(unsigned int damagePoints) : damagePoints(damagePoints) {
}

DamagingObject::DamagingObject(
    float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int damagePoints
) :
    CollidableObject(size, movement, bitmapSegment),
    damagePoints(damagePoints) {
}

unsigned int DamagingObject::getDamagePoints() const {
	return damagePoints;
}

void DamagingObject::dealDamage(DamagableObject & victim) const {
	victim.takeDamage(damagePoints);
}
