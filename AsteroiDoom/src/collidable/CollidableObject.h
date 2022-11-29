#pragma once

#include <d2d1.h>

class CollidableObject {
	static unsigned long long nextID();

protected:
	unsigned long long ID;

	const unsigned int size;

	D2D_POINT_2F position;
	D2D_POINT_2F speed;

public:
	CollidableObject(unsigned int size, D2D_POINT_2F position, D2D_POINT_2F speed, unsigned long long ID = nextID());

	static bool lefter(unsigned long long ID1, unsigned long long ID2);

	static bool higher(unsigned long long ID1, unsigned long long ID2);

	static bool righter(unsigned long long ID1, unsigned long long ID2);

	static bool lower(unsigned long long ID1, unsigned long long ID2);
	
	static bool lefter(unsigned long long ID, float x);
	
	static bool higher(unsigned long long ID, float y);

	static bool righter(unsigned long long ID, float x);

	static bool lower(unsigned long long ID, float y);
};
