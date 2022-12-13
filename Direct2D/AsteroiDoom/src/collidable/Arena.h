#pragma once

#include "specific/Asteroid.h"
#include "specific/Projectile.h"
#include "specific/Spaceship.h"

#include <memory>
#include <set>

class Arena {
	float width{};
	float height{};
	float spawnAreaMargin{};
	D2D_RECT_F arenaRectangle{};
	D2D_RECT_F spawnRectangle{};

	std::set<std::unique_ptr<Asteroid>> outerAsteroids{};
	std::set<std::unique_ptr<Asteroid>> innerAsteroids{};
	std::set<std::unique_ptr<Projectile>> projectiles{};
	std::shared_ptr<Spaceship> spaceship{};

	void drawInner(D2D_POINT_2F translation = {0, 0}) const;

public:
	Arena();

	Arena(float width, float height, float spawnAreaMargin, std::shared_ptr<Spaceship> spaceship);

	void addAsteroid(
	    float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints, unsigned int damagePoints
	);

	void spawnAsteroid(float size, BitmapSegment bitmapSegment, unsigned int hitPoints, unsigned int damagePoints);

	void addProjectile(std::unique_ptr<Projectile> && projectilePointer);

	void draw() const;

	void move(unsigned long long millis);

	/**
	 * @return the number of points gained as a result of collisions
	 */
	unsigned int checkCollisions();
};
