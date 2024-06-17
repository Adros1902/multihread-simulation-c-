#pragma once
#include "Map.h"
#include <thread>
#include <semaphore.h>
#include <condition_variable>
#include <atomic>
#include <mutex>


class Simulation
{
	Map *map;
	int mapSize = 20;
	int troopsNumber = 12;
	int resourcesMaxNumber = 8;
	int resourcesMinNumber = 3;
	int resourcesIdCounter = 0;
	int troopsIdCOunter = 1;

public:
	static std::atomic<bool> endOfSimulation;
	static std::atomic<bool> isDay;
	static std::atomic<int> oasisCounter;
	static std::mutex dayMutex;
	static sem_t oasisSemaphore;

	Simulation();

	~Simulation();

	void simulation();

	void printMap();

	void dayAndNight(Map* map, std::condition_variable* cv);

	void resourceAdder(Map* map, std::uniform_int_distribution<int> randomMapCoords, std::mt19937 rng);

	void troopsAdder(Map* map, std::uniform_int_distribution<int> randomMapCoords, std::mt19937 rng, std::condition_variable *dayCV);

};

