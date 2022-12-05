#include "CollidableObject.h"

#include <limits>

namespace {
	float square(float x) {
		return x * x;
	}
} // namespace

MovementData::MovementData() = default;

MovementData::MovementData(D2D_POINT_2F location, float rotation, D2D_POINT_2F velocity, float spin) :
    location(location),
    rotation(rotation),
    velocity(velocity),
    spin(spin) {
}

void MovementData::move(unsigned int millis, D2D_RECT_F modulo) {
	float width = modulo.right - modulo.left;
	float height = modulo.bottom - modulo.top;
	location.x += velocity.x * float(millis) / 1000;
	location.y += velocity.y * float(millis) / 1000;
	rotation += spin * float(millis) / 1000;
	if (location.x < modulo.left) {
		location.x += width;
	} else if (location.x > modulo.right) {
		location.x -= width;
	}
	if (location.y < modulo.top) {
		location.y += height;
	} else if (location.y > modulo.bottom) {
		location.y -= height;
	}
	if (rotation < 0) {
		rotation += 360;
	} else if (rotation > 360) {
		rotation -= 360;
	}
}

CollidableObject::CollidableObject() = default;

CollidableObject::CollidableObject(float size, MovementData movement, BitmapSegment bitmapSegment) :
    size(size),
    movement(movement),
    bitmapSegment(bitmapSegment) {
}

float CollidableObject::squareDistanceFrom(const CollidableObject & other, D2D_RECT_F modulo) const {
	const static float mxFactors[9] = {0, -1, -1, 0, 1, 1, 1, 0, -1};
	const static float myFactors[9] = {0, 0, -1, -1, -1, 0, 1, 1, 1};
	float mx = modulo.right - modulo.left;
	float my = modulo.bottom - modulo.top;
	auto & [x, y] = movement.location;
	auto & [ox, oy] = other.movement.location;

	float result = std::numeric_limits<float>::infinity();

	for (int i = 0; i < 9; i++) {
		result = min(result, square(x + mxFactors[i] * mx - ox) + square(y + myFactors[i] * my - oy));
	}

	return result;
}

bool CollidableObject::collidesWith(const CollidableObject & other, D2D_RECT_F modulo) const {
	return square(size + other.size) >= squareDistanceFrom(other, modulo);
}

bool CollidableObject::isInside(D2D_RECT_F rectangle) const {
	auto & [left, top, right, bottom] = rectangle;
	auto & [x, y] = movement.location;
	return left <= x - size && x + size <= right && top <= y - size && y + size <= bottom;
}

void CollidableObject::draw(D2D_POINT_2F translation, float opacity, D2D1_BITMAP_INTERPOLATION_MODE interpolationMode)
    const {
	bitmapSegment.draw(
	    {movement.location.x + translation.x, movement.location.y + translation.y},
	    movement.rotation,
	    opacity,
	    interpolationMode
	);
}

void CollidableObject::move(unsigned int millis, D2D_RECT_F modulo) {
	movement.move(millis, modulo);
}
