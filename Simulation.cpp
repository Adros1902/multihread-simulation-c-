#include "Simulation.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "Troop.h"
#include <vector>
#include <random>
#include <unordered_map>
#include "ObjectType.h"
#include "Map.h"
#include <semaphore.h>
#include <condition_variable>
#include <atomic>
#include <mutex>

std::vector<char> charsVector;
std::vector <std::pair<int, int>> resourcesCoords;
std::vector<std::thread> troopsThreadsVector;
std::atomic<bool> Simulation::endOfSimulation(false);
std::atomic<bool> Simulation::isDay(true);
std::mutex Simulation::dayMutex;
sem_t Simulation::oasisSemaphore;
std::atomic<int> Simulation::oasisCounter(2);
Simulation::Simulation()
{
	sem_init(&Simulation::oasisSemaphore, 0, 2);
	endOfSimulation.store(false);
	map = new Map();
	for (int i = 0; i < troopsNumber/2; i++)
	{
		charsVector.push_back('A');
	}
	for (int i = 0; i < troopsNumber / 2; i++)
	{
		charsVector.push_back('E');
	}
	//resourcesCoords.push_back(std::make_pair(14, 16));
	//resourcesCoords.push_back(std::make_pair(1, 2));
}

Simulation::~Simulation()
{
	
}



void Simulation::simulation()
{

	std::random_device rd;
	std::mt19937 rng(rd());
	std::condition_variable *dayCV = new std::condition_variable;

	std::uniform_int_distribution<int> randomMapCoords(0, map->getSize() - 1 );
	

	resourceAdder(map, randomMapCoords, rng);

	for (int i = 0; i < troopsNumber; i++)
	{
		troopsIdCOunter++;
		ObjectsForMap troopType;
		troopType.character = charsVector[i];
		troopType.id = i + 1;
		troopType.objectType = TROOP;
		int x, y;
		while (true) {
			x = randomMapCoords(rng);
			y = randomMapCoords(rng);

			if (map->isBlankSpace(x,y))
			{
				break;
			}
		}

		auto troop = std::make_shared<Troop>(map, troopType, x, y, rng, dayCV);
		map->addTroopsToMap(troop);

		std::thread troopThread([troop]() { troop->lifeOfTroop(); });

		troopsThreadsVector.push_back(std::move(troopThread));
	}

	std::thread mapThread(&Map::printMap, map);
	
	std::thread dayAndNightThread(&Simulation::dayAndNight, this, map, dayCV);
	
	while (true)
	{
		resourceAdder(map, randomMapCoords, rng);
		troopsAdder(map, randomMapCoords, rng, dayCV);
		if (map->getTeamACounter() == 0 || Simulation::endOfSimulation.load()) {
			endOfSimulation.store(true);
			for (auto& thread : troopsThreadsVector) {
				if (thread.joinable()) {
					thread.join();
				}
			}
			mapThread.join();
			dayAndNightThread.join();
			break;
		}

		if (map->getTeamBCounter() == 0) {
			endOfSimulation.store(true);
			for (auto& thread : troopsThreadsVector) {
				if (thread.joinable()) {
					thread.join();
				}
			}
			mapThread.join();
			dayAndNightThread.join();
			break;
		}
		
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void Simulation::printMap()
{

}


void Simulation::dayAndNight(Map* map, std::condition_variable* cv)
{
	while(true) {
		std::this_thread::sleep_for(std::chrono::seconds(13));
		isDay.store(false);
		std::this_thread::sleep_for(std::chrono::seconds(7));
		{
			std::lock_guard<std::mutex> dayLock(Simulation::dayMutex);
			isDay.store(true);
			cv->notify_all();
		}
	}
}

void Simulation::resourceAdder(Map* map, std::uniform_int_distribution<int> randomMapCoords, std::mt19937 rng)
{
	int resourcesCurrent = Map::resourcesCounter.load();
	if (resourcesCurrent <= resourcesMinNumber)
	{
		for(int i = 0; i < resourcesMaxNumber - resourcesCurrent; i ++){
		
		ObjectsForMap resourceType;
		resourceType.character = 'R';
		resourceType.objectType = RESOURCE;
		resourceType.id = resourcesIdCounter;
		std::shared_ptr<Resource> resource = std::make_shared<Resource>();
		resource->damage = 20;
		resource->hp = 20;
		resource->metalScraps = 50;
		int x, y;
		while (true) {
			x = randomMapCoords(rng);
			y = randomMapCoords(rng);

			if (map->isBlankSpace(x, y))
			{
				break;
			}
		}
		resource->coordinates = std::make_pair(x, y);
		resource->resourceType = resourceType;
		resource->isTaken = false;
		map->addResourcesToMap(resource);
		resourcesIdCounter++;
		}
	}
	
	
}

void Simulation::troopsAdder(Map *map, std::uniform_int_distribution<int> randomMapCoords, std::mt19937 rng, std::condition_variable *dayCV)
{
	int teamACounter = Map::teamACounterAdder.load();
	int teamBCounter = Map::teamBCounterAdder.load();

	if (teamACounter != 0)
	{
		troopsIdCOunter++;
		Map::teamACounterAdder.fetch_sub(1);
		ObjectsForMap troopType;
		troopType.character = 'A';
		troopType.id = troopsIdCOunter;
		troopType.objectType = TROOP;
		int x, y;
		int cnt = 0;
		while (true && cnt > 3) {
			x = randomMapCoords(rng);
			y = randomMapCoords(rng);
			cnt++;
			if (map->isBlankSpace(x,y))
			{
				break;
			}
		}

		auto troop = std::make_shared<Troop>(map, troopType, x, y, rng, dayCV);
		map->addTroopsToMap(troop);

		std::thread troopThread([troop]() { troop->lifeOfTroop(); });

		troopsThreadsVector.push_back(std::move(troopThread));
		
	}

	if (teamBCounter != 0)
	{
		troopsIdCOunter++;
		Map::teamBCounterAdder.fetch_sub(1);
		ObjectsForMap troopType;
		troopType.character = 'E';
		troopType.id = troopsIdCOunter;
		troopType.objectType = TROOP;
		int x, y;
		while (true) {
			x = randomMapCoords(rng);
			y = randomMapCoords(rng);

			if (map->isBlankSpace(x,y))
			{
				break;
			}
		}

		auto troop = std::make_shared<Troop>(map, troopType, x, y, rng, dayCV);
		map->addTroopsToMap(troop);

		std::thread troopThread([troop]() { troop->lifeOfTroop(); });

		troopsThreadsVector.push_back(std::move(troopThread));
		
	}
	
}


