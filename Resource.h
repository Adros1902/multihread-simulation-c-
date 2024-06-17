#pragma once
#include <iostream>
#include "ObjectsForMap.h"
class Resource
{
public:
	int hp;
	int damage;
	int metalScraps;
	std::pair<int, int> coordinates;
	ObjectsForMap resourceType;
	bool isTaken;
};

