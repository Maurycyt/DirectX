#include "Arena.h"

using namespace std;

Arena::Arena(float width, float height, float spawnAreaMargin, shared_ptr<Spaceship> spaceship) :
    width(width),
    height(height),
    spawnAreaMargin(spawnAreaMargin),
    spaceship(std::move(spaceship)),
    arenaRectangle({-width / 2, -height / 2, width / 2, height / 2}),
    spawnRectangle(
        {arenaRectangle.left - spawnAreaMargin,
         arenaRectangle.top - spawnAreaMargin,
         arenaRectangle.right + spawnAreaMargin,
         arenaRectangle.bottom + spawnAreaMargin}
    ) {
}

void Arena::addAsteroid(
    float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int hitPoints, unsigned int damagePoints
) {
	outerAsteroids.emplace(make_unique<Asteroid>(size, movement, bitmapSegment, hitPoints, damagePoints));
}

void Arena::addProjectile(float size, MovementData movement, BitmapSegment bitmapSegment, unsigned int damagePoints) {
	projectiles.emplace(make_unique<Projectile>(size, movement, bitmapSegment, damagePoints));
}

void Arena::drawInner(D2D_POINT_2F translation) const {
	for (auto & asteroid : innerAsteroids) {
		asteroid->draw(translation);
	}
	for (auto & projectile : projectiles) {
		projectile->draw(translation);
	}
	if (spaceship)
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

void Arena::move(unsigned int millis) {
	for (auto & asteroid : innerAsteroids) {
		asteroid->move(millis, arenaRectangle);
	}
	for (auto it = outerAsteroids.begin(); it != outerAsteroids.end();) {
		(*it)->move(millis, spawnRectangle);
		if ((*it)->isInside({0, 0, width, height})) {
			auto extracted = outerAsteroids.extract(it++);
			innerAsteroids.emplace(std::move(extracted.value()));
		} else {
			it++;
		}
	}
	for (auto & projectile : projectiles) {
		projectile->move(millis, arenaRectangle);
	}
	if (spaceship)
		spaceship->move(millis, arenaRectangle);
}
