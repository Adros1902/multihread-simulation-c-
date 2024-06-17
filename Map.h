#pragma once
#include <mutex>
#include "ObjectsForMap.h"
#include <vector>
#include "Troop.h"
#include "Resource.h"
#include <semaphore.h>
#include <condition_variable>
#include <atomic>
#include <mutex>

class Map
{
public:
	ObjectsForMap** map;
	ObjectsForMap blankSpace;
	ObjectsForMap oasisSpace;
	ObjectsForMap bombSpace;
	int oasisStartX;
	int oasisStartY;
	int mapSize = 21;
	int oasisSize = 3; //have to be even
	int bombTimer = 15;
	std::pair <int, int> oasisCenter;
	std::vector<std::shared_ptr<Troop>> troopsVector;
	std::mutex troopsVectorMutex;
	std::mutex resourceMutex;
	std::mutex oasisMutex;
	std::vector<std::shared_ptr<Resource>> resourcesVector;
	std::atomic<int> teamACounter;
	std::atomic<int> teamBCounter;
	static std::atomic<bool> isBombed;
	static std::atomic<int> resourcesCounter;
	static std::atomic<int> teamACounterAdder;
	static std::atomic<int> teamBCounterAdder;
	int lastTroopsSize;
	int lastResourcesSize;


	Map();
	~Map();
	int getSize();
	bool updateCoords(int nextX, int nextY, int currentX, int currentY, ObjectsForMap troopType);
	void printMap();
	void addTroopsToMap(std::shared_ptr<Troop> troop);
	std::shared_ptr<Troop> checkForOtherTroops(int x, int y, ObjectsForMap objectType);
	void deathOfTroop(int x, int y);
	void addResourcesToMap(std::shared_ptr<Resource> resource);
	std::pair<int, int> closestResource(int x, int y);
	bool isBlankSpace(int x, int y);
	int getTeamACounter();
	int getTeamBCounter();
	bool isValid(int x, int y);
	std::pair<int, int> getClosestEnemy(std::pair<int,int> troopCoords, ObjectsForMap troopType);
	std::pair<int,int> getResource(std::shared_ptr<Resource> resource);
	bool resourceExist(std::shared_ptr<Resource> resource);
	void setResourceTaken(std::shared_ptr<Resource> resource);
	bool isNextStepOasis(std::pair<int,int> troopCoords);
	bool updateOasisCoords(std::pair<int, int> nextMove, std::pair<int, int> currentPos, ObjectsForMap troopType);
	std::pair<int, int> getOasisCenter();

	std::shared_ptr<Resource> checkForResource(int x, int y);

	void deleteResource(std::shared_ptr<Resource> resource);

	bool isThereAnyResource();
};

