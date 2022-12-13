#include "Arena.h"

#include "../utils/Random.h"
#include "../utils/AsteroiDoomConstants.h"

#include <utility>

using namespace std;

Arena::Arena() = default;

Arena::Arena(float width, float height, float spawnAreaMargin, shared_ptr<Spaceship> spaceship) :
    width(width),
    height(height),
    spawnAreaMargin(spawnAreaMargin),
    arenaRectangle({-width / 2, -height / 2, width / 2, height / 2}),
    spawnRectangle(
        {arenaRectangle.left - spawnAreaMargin,
         arenaRectangle.top - spawnAreaMargin,
         arenaRectangle.right + spawnAreaMargin,
         arenaRectangle.bottom + spawnAreaMargin}
    ),
    spaceship(std::move(spaceship)) {
}

void Arena::addAsteroid(
    float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints, unsigned int damagePoints
) {
	outerAsteroids.emplace(make_unique<Asteroid>(size, movement, bitmapSegment, hitPoints, damagePoints));
}

void Arena::addProjectile(unique_ptr<Projectile> && projectilePointer) {
	if (projectilePointer) {
		projectiles.emplace(std::move(projectilePointer));
	}
}

void Arena::drawInner(D2D_POINT_2F translation) const {
	for (auto & asteroid : innerAsteroids) {
		asteroid->draw(translation);
	}
	for (auto & projectile : projectiles) {
		projectile->draw(translation);
	}
	spaceship->draw(translation);
}

void Arena::draw() const {
	static D2D_POINT_2F translations[9] = {
	    {0, 0},
	    {-width, 0},
	    {-width, -height},
	    {0, -height},
	    {width, -height},
	    {width, 0},
	    {width, height},
	    {0, height},
	    {-width, height}};
	for (auto & asteroid : outerAsteroids) {
		asteroid->draw();
	}
	for (D2D_POINT_2F translation : translations) {
		drawInner(translation);
	}
}

void Arena::move(unsigned long long millis) {
	for (auto & asteroid : innerAsteroids) {
		asteroid->move(millis, arenaRectangle);
	}
	for (auto asteroid = outerAsteroids.begin(); asteroid != outerAsteroids.end();) {
		(*asteroid)->move(millis, spawnRectangle);
		if ((*asteroid)->isInside(arenaRectangle)) {
			auto extracted = outerAsteroids.extract(asteroid++);
			innerAsteroids.emplace(std::move(extracted.value()));
		} else {
			asteroid++;
		}
	}
	for (auto & projectile : projectiles) {
		projectile->move(millis, arenaRectangle);
	}
	spaceship->move(millis, arenaRectangle);
}

unsigned int Arena::checkCollisions() {
	unsigned int score = 0;

	// check projectiles
	for (auto projectile = projectiles.begin(); projectile != projectiles.end();) {
		// check against innerAsteroids
		for (auto asteroid = innerAsteroids.begin(); asteroid != innerAsteroids.end(); asteroid++) {
			if ((*projectile)->collidesWith(**asteroid, arenaRectangle)) {
				(*projectile)->dealDamage(**asteroid);
				if ((*asteroid)->destroyed()) {
					score += (*asteroid)->pointsForDestruction();
					innerAsteroids.erase(asteroid);
				}
				projectiles.erase(projectile++);
				goto nextProjectile;
			}
		}
		// check against outerAsteroids
		for (auto asteroid = outerAsteroids.begin(); asteroid != outerAsteroids.end(); asteroid++) {
			if ((*projectile)->collidesWith(**asteroid, spawnRectangle)) {
				(*projectile)->dealDamage(**asteroid);
				if ((*asteroid)->destroyed()) {
					score += (*asteroid)->pointsForDestruction();
					outerAsteroids.erase(asteroid);
				}
				projectiles.erase(projectile++);
				goto nextProjectile;
			}
		}
		// check against spaceship
		if ((*projectile)->collidesWith(*spaceship, arenaRectangle)) {
			(*projectile)->dealDamage(*spaceship);
			projectiles.erase(projectile++);
			goto nextProjectile;
		}
		projectile++;
	nextProjectile:
		continue;
	}

	// check spaceship
	// check against innerAsteroids
	for (auto asteroid = innerAsteroids.begin(); asteroid != innerAsteroids.end();) {
		if (spaceship->collidesWith(**asteroid, arenaRectangle)) {
			(*asteroid)->dealDamage(*spaceship);
			score += (*asteroid)->pointsForDestruction();
			innerAsteroids.erase(asteroid++);
		} else {
			asteroid++;
		}
	}
	// check against outerAsteroids
	for (auto asteroid = outerAsteroids.begin(); asteroid != outerAsteroids.end();) {
		if (spaceship->collidesWith(**asteroid, spawnRectangle)) {
			(*asteroid)->dealDamage(*spaceship);
			score += (*asteroid)->pointsForDestruction();
			outerAsteroids.erase(asteroid++);
		} else {
			asteroid++;
		}
	}

	return score;
}

void Arena::spawnAsteroid(float size, BitmapSegment bitmapSegment, unsigned int hitPoints, unsigned int damagePoints) {
	MovementData movementData{};
	while (true) {
		movementData.location = {
		    float(Random::next(0, (unsigned int)(spawnRectangle.right - spawnRectangle.left))) + spawnRectangle.left,
		    float(Random::next(0, (unsigned int)(spawnRectangle.bottom - spawnRectangle.top))) + spawnRectangle.top};

		if ((movementData.location.x + size <= arenaRectangle.left || movementData.location.x - size >= arenaRectangle.right
		    ) &&
		    (movementData.location.y + size <= arenaRectangle.top || movementData.location.y - size >= arenaRectangle.bottom
		    )) {
			break;
		}
	}

	movementData.spin = float(Random::next(0, 720)) - 360;

	auto speed = float(Random::next(50, 300));
	auto angle = float(Random::next(0, 360)) * RadiansInDegree;
	movementData.velocity = {speed * cos(angle), speed * sin(angle)};

	addAsteroid(size, movementData, bitmapSegment, hitPoints, damagePoints);
}
