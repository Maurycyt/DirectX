#include "Spaceship.h"

Spaceship::Spaceship(float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints) :
    CollidableObject(size, movement, bitmapSegment),
    DamagableObject(hitPoints) {
}
