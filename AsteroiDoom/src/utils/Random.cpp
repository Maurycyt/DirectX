#include "Random.h"

std::random_device Random::randomDevice{};

std::mt19937 Random::rng{randomDevice()};

unsigned int Random::next(unsigned int a, unsigned int b) {
	return rng() % (b - a) + a;
}
