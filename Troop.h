#pragma once
#ifndef TROOP_H
#define TROOP_H

#include <utility>
#include <iostream>
#include <chrono>
#include <random>
#include "ObjectsForMap.h"
#include "mutex"
#include "Resource.h"
#include <semaphore.h>
#include <condition_variable>
#include <atomic>
#include <mutex>

class Map;

class Troop
{
public:
	int id;
	int damage;
	int hp;
	int maxHp;
	bool isGoingToResources;
	bool foundResource;
	bool isGoingToEnemy;
	bool isMoveRandom;
	bool foundEnemy;
	bool haveDestination;
	bool exitOasis;
	bool firstOasisStep;
	bool hasMoved;
	int healingPerSecond;
	std::atomic<bool> isInOasis;
	bool healingProcess;
	bool hasAquiredSemaphore;
	std::atomic<bool> isAlive;
	Map *map;
	std::mt19937 rng;
	int mapSize;
	std::pair<int, int> coordinates;
	std::pair<int, int> lastPosition;
	std::pair<int, int> exitPoint;
	std::vector<std::pair<int, int>> exitPoints;
	ObjectsForMap troopType;
	std::shared_ptr<Resource> currentResource = nullptr;
	std::shared_ptr<Troop> currentEnemy = nullptr;
	std::mutex hpMutex;
	std::mutex enemyAttackingMutex;
	std::mutex getIdMutex;
	std::atomic<bool> isInFight;
	std::atomic<bool> enemyAttacking;
	std::pair<int, int> closestResources;
	std::pair<int, int> closestEnemy;
	std::pair<int, int> currentDestination;
	std::mutex* dayMutex;
	std::condition_variable* cv;
	// x i y
	void movement(bool isRandom, char resourceOrTroop);
	void fight(std::shared_ptr<Troop>  enemy);

	Troop(Map *mapIn, ObjectsForMap type, int x, int y, std::mt19937& rng, std::condition_variable* cv);
	Troop(const Troop&) = delete;

	~Troop();
	bool getEnemyAttacking();
	void lifeOfTroop();
	int getId();
	void getDamage(int damage);
	bool getIsAlive();
	void setEnemyAttacking();
	void findResources();
	void findEnemy();
	std::pair<int, int> pathFinder(std::pair<int, int> currentDest);
	void getResource(std::shared_ptr<Resource> resource);
	int calcDistance(std::pair<int, int> destination, std::pair<int,int> tempTroopPosition);
	void goToOasis();
	void cleanUpOnDeath();
};
#endif //TROOP_H

