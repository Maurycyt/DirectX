#pragma once

#include <random>

class Random {
	static std::random_device randomDevice;
	static std::mt19937 rng;

public:
	static unsigned int next(unsigned int a, unsigned int b);
};
