#include "CollidableObject.h"

#include <memory>
#include <unordered_map>

namespace {
	std::unordered_map<unsigned long long, std::unique_ptr<CollidableObject>> collidableObjects{};
}

unsigned long long CollidableObject::nextID() {
	static constinit unsigned long long ID = 0;
	return ++ID;
}

CollidableObject::CollidableObject(
    unsigned int size, D2D_POINT_2F position, D2D_POINT_2F speed, unsigned long long ID
) :
    ID(ID),
    size(size), position(position), speed(speed) {
}

bool CollidableObject::lefter(unsigned long long ID1, unsigned long long ID2) {
	auto & Obj1 = collidableObjects.at(ID1);
	auto & Obj2 = collidableObjects.at(ID2);
	return Obj1->position.x - float(Obj1->size) < Obj2->position.x - float(Obj2->size);
}

bool CollidableObject::higher(unsigned long long ID1, unsigned long long ID2) {
	auto & Obj1 = collidableObjects.at(ID1);
	auto & Obj2 = collidableObjects.at(ID2);
	return Obj1->position.y - float(Obj1->size) < Obj2->position.y - float(Obj2->size);
}

bool CollidableObject::righter(unsigned long long ID1, unsigned long long ID2) {
	auto & Obj1 = collidableObjects.at(ID1);
	auto & Obj2 = collidableObjects.at(ID2);
	return Obj1->position.x + float(Obj1->size) > Obj2->position.x + float(Obj2->size);
}

bool CollidableObject::lower(unsigned long long ID1, unsigned long long ID2) {
	auto & Obj1 = collidableObjects.at(ID1);
	auto & Obj2 = collidableObjects.at(ID2);
	return Obj1->position.y + float(Obj1->size) > Obj2->position.y + float(Obj2->size);
}

bool CollidableObject::lefter(unsigned long long ID, float x) {
	auto & Obj = collidableObjects.at(ID);
	return Obj->position.x - float(Obj->size) < x;
}

bool CollidableObject::higher(unsigned long long ID, float y) {
	auto & Obj = collidableObjects.at(ID);
	return Obj->position.y - float(Obj->size) < y;
}

bool CollidableObject::righter(unsigned long long ID, float x) {
	auto & Obj = collidableObjects.at(ID);
	return Obj->position.x + float(Obj->size) > x;
}

bool CollidableObject::lower(unsigned long long ID, float y) {
	auto & Obj = collidableObjects.at(ID);
	return Obj->position.y + float(Obj->size) > y;
}
