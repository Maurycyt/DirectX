#pragma once

#define WIN32_LEAN_AND_MEAN

#include "../../BitmapUtils.h"

#include <d2d1.h>

class MovementData {
public:
	D2D_POINT_2F location{};
	float rotation{};
	D2D_POINT_2F velocity{};
	float spin{};

	MovementData();

	MovementData(D2D_POINT_2F location, float rotation, D2D_POINT_2F velocity, float spin);

	void move(unsigned int millis, D2D_RECT_F modulo);
};

class CollidableObject {
protected:
	const float size{};

	MovementData movement{};

	BitmapSegment bitmapSegment{};

protected:
	CollidableObject();

public:
	CollidableObject(float size, MovementData movement, BitmapSegment bitmapSegment);

	[[nodiscard]] float squareDistanceFrom(const CollidableObject & other, D2D_RECT_F modulo) const;

	[[nodiscard]] bool collidesWith(const CollidableObject & other, D2D_RECT_F modulo) const;

	[[nodiscard]] bool isInside(D2D_RECT_F rectangle) const;

	void draw(
	    D2D_POINT_2F translation = {0, 0},
	    float opacity = 1,
	    D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
	) const;

	void move(unsigned int millis, D2D_RECT_F modulo);
};
