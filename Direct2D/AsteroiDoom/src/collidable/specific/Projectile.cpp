#include "Projectile.h"

Projectile::Projectile(float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int damagePoints) :
    CollidableObject(size, movement, bitmapSegment),
    DamagingObject(damagePoints) {
}
