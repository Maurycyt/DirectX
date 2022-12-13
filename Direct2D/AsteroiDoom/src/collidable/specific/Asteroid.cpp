#include "Asteroid.h"

Asteroid::Asteroid(
    float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints, unsigned int damagePoints
) :
    CollidableObject(size, movement, bitmapSegment),
    DamagableObject(hitPoints),
    DamagingObject(damagePoints) {
}
