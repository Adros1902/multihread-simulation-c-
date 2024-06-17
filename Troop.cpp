#include "Troop.h"
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <random>
#include "Map.h"
#include "ObjectsForMap.h"
#include "mutex"
#include "Simulation.h"
#include <semaphore.h>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <limits.h>



Troop::Troop(Map *mapIn, ObjectsForMap type, int x, int y, std::mt19937& rngIn, std::condition_variable* cv)
{
	this->cv = cv;
	rng = rngIn;
	hasAquiredSemaphore = false;
	map = mapIn;
	mapSize = map->getSize();
	troopType = type;
	id = troopType.id;
	coordinates.first = x;
	coordinates.second = y;
	isAlive.store(true);
	isInFight.store(false);
	damage = 30;
	hp = 100;
	maxHp = hp + 10;
	healingPerSecond = 10;
	enemyAttacking.store(false);
	isGoingToResources = false;
	isGoingToEnemy = false;
	foundEnemy = false;
	foundResource = false;
	haveDestination = false;
	isInOasis.store(false);
	healingProcess = false;
	exitOasis = false;
	firstOasisStep = false;
	exitPoints.push_back(std::make_pair<int, int>(0, 0));
	exitPoints.push_back(std::make_pair<int, int>(0, mapSize-1));
	exitPoints.push_back(std::make_pair<int, int>(mapSize - 1, 0));
	exitPoints.push_back(std::make_pair<int, int>(mapSize - 1, mapSize - 1));
	//enemyAttacinkg = false;
}

Troop::~Troop()
{

}

bool Troop::getEnemyAttacking() 
{

	return enemyAttacking.load();

}

void Troop::lifeOfTroop()
{
	std::uniform_int_distribution<int> isMoveRandomInt(0, 100);
	while (true) {
		if(!isAlive.load()){
			map->deathOfTroop(coordinates.first, coordinates.second);
            break;
		}
		hasMoved = false;
		if (!Simulation::isDay.load()) {
			
			std::unique_lock<std::mutex> lock(Simulation::dayMutex);
			cv->wait(lock, [this] {return Simulation::isDay.load(); });
			
		}

		if (Simulation::endOfSimulation.load())
		{
			break;
		}

		if (isAlive.load())
		{
			if (healingProcess) {
				hp += healingPerSecond;
				if (hp >= maxHp)
				{
					healingProcess = false;
					exitOasis = true;
					int pickedPoint = isMoveRandomInt(rng) % 4;
					exitPoint = exitPoints[pickedPoint];
				}
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			else {

				findEnemy();
				findResources();
				currentEnemy = map->checkForOtherTroops(coordinates.first, coordinates.second, troopType);
				if (currentEnemy != nullptr) {
					if (!currentEnemy->isInFight.load() && !isInOasis.load())
					{
						isGoingToResources = false;
						haveDestination = false;
						foundResource = false;
						fight(currentEnemy);
						if(!isAlive.load()){
								continue;
							}
					}
				}

				//<0,45> - troop, <46, 90> - resource <91,100> - random
				if (hp < maxHp / 2) {
					//oasis - 2
					isGoingToResources = false;
					isGoingToEnemy = false;
					foundEnemy = false;
					foundResource = false;
					haveDestination = false;
					movement(false, 2);
				}
				else {
					if (!haveDestination && !exitOasis ) {

						int isMoveRandom = isMoveRandomInt(rng);
						if ((isMoveRandom < 101 && isMoveRandom > 65 && map->isThereAnyResource()) || closestEnemy == std::make_pair(INT_MAX- 100000, INT_MAX- 100000)) {
							//resources - 1
							isGoingToResources = true;
							isGoingToEnemy = false;
							haveDestination = true;
							movement(false, 1);
						}
						else {
							//enemy - 0
							movement(false, 0);
							haveDestination = true;
							isGoingToEnemy = true;
							isGoingToResources = false;
						}
					}
					if (exitOasis)
					{
						movement(false, 2);
					}
				}

				if (!foundResource) {
					//movement(true);

					if (isGoingToResources) {
						currentResource = map->checkForResource(coordinates.first, coordinates.second);
						if (currentResource != nullptr) {
							foundResource = true;
							haveDestination = false;
						}
						else {
							movement(false, 1);
						}
					}
				}
				else {
					if (currentResource != nullptr)
					{
						getResource(currentResource);
					}
					else {
						haveDestination = false;
						isGoingToResources = false;
						foundResource = false;
					}

				}

				if (isGoingToEnemy && !isInOasis.load()) {
					currentEnemy = map->checkForOtherTroops(coordinates.first, coordinates.second, troopType);
					if (currentEnemy != nullptr) {
						if (!currentEnemy->isInFight.load())
						{
							isGoingToResources = false;
							haveDestination = false;
							foundResource = false;
							fight(currentEnemy);
							if(!isAlive.load()){
								continue;
							}
						}
					}
					else if (closestEnemy == std::make_pair<int, int>(INT_MAX- 100000, INT_MAX- 100000)) {
						//movement(true, 0);
					}
					else {
						movement(false, 0);
					}
				}

				currentEnemy = map->checkForOtherTroops(coordinates.first, coordinates.second, troopType);
				if (currentEnemy != nullptr) {
					if (!currentEnemy->isInFight.load() && !isInOasis.load())
					{
						isGoingToResources = false;
						haveDestination = false;
						foundResource = false;
						fight(currentEnemy);
						if(!isAlive.load()){
								continue;
							}
					}
				}

				std::this_thread::sleep_for(std::chrono::seconds(2));
			}
		}
		else {
			map->deathOfTroop(coordinates.first, coordinates.second);
			break;
		}
	}
}

int Troop::getId()
{
	std::lock_guard<std::mutex> lock(getIdMutex);
	return id;
}

void Troop::getDamage(int enemyDamage)
{
	std::lock_guard<std::mutex> lock(hpMutex);
	hp -= enemyDamage;
	if (hp <= 0) {
		if (currentResource != nullptr) {
			map->setResourceTaken(currentResource);
		}
		if (hasAquiredSemaphore) {
			sem_post(&Simulation::oasisSemaphore);
			Simulation::oasisCounter.fetch_add(1);
			hasAquiredSemaphore = false;
		}
		isAlive.store(false);
	}

}

bool Troop::getIsAlive()
{
	return isAlive.load();
}

void Troop::setEnemyAttacking()
{
	enemyAttacking.store(true);
}

void Troop::findResources()
{
	closestResources = map->closestResource(coordinates.first, coordinates.second);
	if (closestResources == std::make_pair(INT_MAX- 100000, INT_MAX- 100000)) {
		foundResource = false;
		isGoingToResources = false;
		haveDestination = false;
	}
}

void Troop::findEnemy()
{
	closestEnemy = map->getClosestEnemy(coordinates, troopType);
	if (closestEnemy == std::make_pair(INT_MAX- 100000, INT_MAX- 100000)) {
		foundEnemy = false;
		isGoingToEnemy = false;
		haveDestination = false;
	}
}

std::pair<int, int> Troop::pathFinder(std::pair<int,int> currentDest)
{
	/*if (closestResources == std::make_pair(INT_MAX, INT_MAX)) {
		return std::make_pair(INT_MAX, INT_MAX);
	}*/
	int differenceX = currentDest.first - coordinates.first;
	int differenceY = currentDest.second - coordinates.second;
	
	int nextMoveX, nextMoveY;
	if (differenceX > 0 )
	{
		nextMoveX = 1;
	}
	else if (differenceX == 0) {
		nextMoveX = 0;
	}
	else {
		nextMoveX = -1;
	}

	if (differenceY > 0)
	{
		nextMoveY = 1;
	}
	else if (differenceY == 0) {
		nextMoveY = 0;
	}
	else {
		nextMoveY = -1;
	}
	currentDestination = currentDest;
	return std::make_pair(nextMoveX, nextMoveY);
}

void Troop::getResource(std::shared_ptr<Resource> resource)
{
	if (resource != nullptr)
	{
		std::pair<int,int> tempPair = map->getResource(resource);
	if (tempPair != std::make_pair(INT_MAX- 100000, INT_MAX- 100000)) {
		std::lock_guard<std::mutex> lock(hpMutex);
		hp += 10;
		damage += 10;
		if (tempPair.first <= 0 || tempPair.second <= 0)
		{
			map->deleteResource(resource);
			currentResource = nullptr;
			foundResource = false;
			isGoingToResources = false;
			haveDestination = false;
			currentResource = nullptr;
		}
	}
	} else{
			currentResource = nullptr;
			foundResource = false;
			isGoingToResources = false;
			haveDestination = false;
			currentResource = nullptr;
	}
	
	
//caly czas jak ginie to jest problem duzy z semaforem
}

int Troop::calcDistance(std::pair<int, int> destination, std::pair<int, int> tempTroopPosition)
{
	return std::abs(destination.first - tempTroopPosition.first) + std::abs(destination.second - tempTroopPosition.second);
}

void Troop::goToOasis()
{
	
}

void Troop::movement(bool isRandom, char resourceOrTroop)
{
		if (hasMoved) {
			return;
		}
		std::pair<int, int> nextMove = coordinates;
		std::uniform_int_distribution<int> randomMove(0, 7);
		if (isRandom) {
			while (true) {
				
				int randomNextMove = randomMove(rng);
				//std::pair<int, int> nextMove = coordinates;
				switch (randomNextMove)
				{
				case 0:
					nextMove.first++;
					break;
				case 1:
					nextMove.first++;
					nextMove.second++;
					break;
				case 2:
					nextMove.second++;
					break;
				case 3:
					nextMove.first--;
					nextMove.second++;
					break;
				case 4:
					nextMove.first--;
					break;
				case 5:
					nextMove.first--;
					nextMove.second--;
					break;
				case 6:
					nextMove.second--;
					break;
				case 7:
					nextMove.first++;
					nextMove.second--;
					break;
				default:
					break;
				}
				
				if (nextMove.first >= 0 && nextMove.first < mapSize && nextMove.second >= 0 && nextMove.second < mapSize ) {
					if (map->isBlankSpace(nextMove.first, nextMove.second)) {
						break;
					}
				}
			}
		}
		else {
			std::pair<int, int> pathToDest;
			if (!exitOasis) {
				if (resourceOrTroop == 1) {
					pathToDest = pathFinder(closestResources);
				}
				else if (resourceOrTroop == 0) {
					pathToDest = pathFinder(closestEnemy);
				}
				else if (resourceOrTroop == 2) {
					pathToDest = pathFinder(map->getOasisCenter());

					nextMove.first += pathToDest.first;
					nextMove.second += pathToDest.second;

					if (map->isNextStepOasis(nextMove))
					{
						if (!isInOasis.load()) {
							if (sem_trywait(&Simulation::oasisSemaphore) == -1) {
								return;
							}
							hasAquiredSemaphore = true;
							Simulation::oasisCounter.fetch_sub(1);
							isInOasis.store(true);
							firstOasisStep = true;
						}
					} //jesli nie ma juz ruchu zaczyna sie leczyc
					else if ((!map->isNextStepOasis(nextMove) && isInOasis) || coordinates == map->getOasisCenter()) {
						nextMove = coordinates;
						healingProcess = true;
					}
				}
			}
			if (!isInOasis.load()) {
				nextMove.first += pathToDest.first;
				nextMove.second += pathToDest.second;

				if (map->isValid(nextMove.first, nextMove.second)) {
					if (!map->isBlankSpace(nextMove.first, nextMove.second) || nextMove == lastPosition)
					{
						int bestDistance = INT_MAX- 100000;
						std::pair<int, int> bestMove = nextMove;
						for (int i = -1; i <= 1; i++) {
							for (int j = -1; j <= 1; j++)
							{
								if (!(i == 0 && j == 0))
								{
									int newX = coordinates.first + i;
									int newY = coordinates.second + j;
									if (map->isValid(newX, newY) && map->isBlankSpace(newX, newY) && std::make_pair(newX, newY) != lastPosition && std::make_pair(newX, newY) != nextMove)
									{
										int tempDistance = calcDistance(currentDestination, std::make_pair(newX, newY));
										if (tempDistance < bestDistance)
										{
											bestDistance = tempDistance;
											bestMove = std::make_pair(newX, newY);
										}
									}
								}
							}
						}
						nextMove = bestMove; // Aktualizacja nextMove na najlepszy dost�pny ruch
					}
				}
			}//system omijania przeszkod - jesli nie moze znalezc lepszego ruchu, to szuka na sile np zawsze w lewo gdzie ma isc, a zdo znalezienia pierwszego rucho gora - dol (resources nie moga sie spawnowac zaraz obok oazy)
			else if (exitOasis == true && isInOasis.load()) {
				pathToDest = pathFinder(exitPoint);
				nextMove.first += pathToDest.first;
				nextMove.second += pathToDest.second;

				if (map->isValid(nextMove.first, nextMove.second)) {
					if (!map->isBlankSpace(nextMove.first, nextMove.second) && !map->isNextStepOasis(nextMove))
					{
						int bestDistance = INT_MAX- 100000;
						std::pair<int, int> bestMove = nextMove;
						for (int i = -1; i <= 1; i++) {
							for (int j = -1; j <= 1; j++)
							{
								if (!(i == 0 && j == 0))
								{
									int newX = coordinates.first + i;
									int newY = coordinates.second + j;
									if (map->isValid(newX, newY) && (map->isBlankSpace(newX, newY) || map->isNextStepOasis(nextMove)) && std::make_pair(newX, newY) != lastPosition && std::make_pair(newX, newY) != nextMove)
									{
										int tempDistance = calcDistance(currentDestination, std::make_pair(newX, newY));
										if (tempDistance < bestDistance)
										{
											bestDistance = tempDistance;
											bestMove = std::make_pair(newX, newY);
										}
									}
								}
							}
						}
						nextMove = bestMove; // Aktualizacja nextMove na najlepszy dost�pny ruch
					}
				}
			}

		}

		if ((isInOasis.load() && !firstOasisStep)) {

			if (map->isBlankSpace(nextMove.first, nextMove.second)) {

				if (nextMove.first >= 0 && nextMove.first < mapSize && nextMove.second >= 0 && nextMove.second < mapSize) {
					if (map->updateOasisCoords(nextMove, coordinates, troopType)) {
						
						coordinates = nextMove;
						lastPosition = coordinates;
					}
				}
				sem_post(&Simulation::oasisSemaphore);
				hasAquiredSemaphore = false;
				isInOasis.store(false);
				Simulation::oasisCounter.fetch_add(1);
				exitOasis = false;
				//healingProcess = false;
			} else {

			if (nextMove.first >= 0 && nextMove.first < mapSize && nextMove.second >= 0 && nextMove.second < mapSize) {
				if (map->updateOasisCoords(nextMove, coordinates, troopType)) {

					coordinates = nextMove;
					lastPosition = coordinates;
				}
			}
			}
		}
		else {
			if (nextMove.first >= 0 && nextMove.first < mapSize && nextMove.second >= 0 && nextMove.second < mapSize) {
				if (map->updateCoords(nextMove.first, nextMove.second, coordinates.first, coordinates.second, troopType)) {
					//std::cout << "#" << std::this_thread::get_id() << std::endl;
					//std::cout << "Current Troop: " << troopType << std::endl;
					coordinates = nextMove;
					lastPosition = coordinates;
					firstOasisStep = false;
					//std::cout << "X: " << coordinates.first << " Y: " << coordinates.second << std::endl;
				}
			}
		}
		hasMoved = true;
}

void Troop::fight(std::shared_ptr<Troop>  enemy)
{
	isGoingToResources = false;
	haveDestination = false;
	foundResource = false;
	if(enemy != nullptr){
	enemy->setEnemyAttacking();
	}
	if (currentResource != nullptr) {
		map->setResourceTaken(currentResource);
		currentResource = nullptr;
	}
	while (isAlive.load()) {

		if (enemy != nullptr)
		{
			enemy->getDamage(damage);
			if (!enemy->getIsAlive()) {
				currentEnemy = nullptr;
				enemyAttacking.store(false);
				isGoingToEnemy = false;
				foundEnemy = false;
				haveDestination = false;
				isInFight.store(false);
				break;
			}
		} else{
				enemyAttacking.store(false);
				isGoingToEnemy = false;
				foundEnemy = false;
				haveDestination = false;
				isInFight.store(false);
				break;
		}	



		std::this_thread::sleep_for(std::chrono::seconds(1));
	}	
}

//nie potrafia minowac zasobow kiedy chca