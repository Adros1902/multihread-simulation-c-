#include "Map.h"
#include "ObjectsForMap.h"
#include "ObjectType.h"
#include "Troop.h"
#include <vector>
#include "Simulation.h"
#include <ncurses.h>
#include <semaphore.h>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <limits.h>


std::mutex mutex;
std::atomic<int> Map::resourcesCounter;
std::atomic<int> Map::teamACounterAdder;
std::atomic<int> Map::teamBCounterAdder;
std::atomic<bool> Map::isBombed;
Map::Map()
{
	isBombed.store(false);
	blankSpace.character = '-';
	blankSpace.objectType = BLANK_SPACE;
	oasisSpace.character = 'O';
	oasisSpace.objectType = OASIS;
	bombSpace.character = 'X';
	bombSpace.objectType = BOMB;
	//std::cout << "New Map" << std::endl;
	map = new ObjectsForMap* [mapSize];

	for (int i = 0; i < mapSize; i++)
	{
		map[i] = new ObjectsForMap[mapSize];
		for (int j = 0; j < mapSize; j++)
		{
			map[i][j] = blankSpace;
		}
	}
	oasisStartX = (mapSize / 2) - (oasisSize/2);
	oasisStartY = (mapSize / 2) - (oasisSize/2);
	oasisCenter = std::make_pair(mapSize / 2 , mapSize / 2 );



	for (int i = oasisStartX; i < oasisStartX + oasisSize; i++)
	{
		for (int j = oasisStartY; j < oasisStartY + oasisSize; j++)
		{
			map[i][j] = oasisSpace;
		}

	}
	
	resourcesCounter.store(0);
	teamACounterAdder.store(0);
	teamBCounterAdder.store(0);
	teamACounter.store(0);
	teamBCounter.store(0);
}


Map::~Map()
{
	for (int i = 0; i < mapSize; i++)
	{
		delete[] map[i];
	}
	delete[] map;
}

int Map::getSize()
{
	return mapSize;
}

bool Map::updateCoords(int nextX, int nextY, int currentX, int currentY, ObjectsForMap troopType)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (map[nextX][nextY].objectType == BLANK_SPACE || map[nextX][nextY].objectType == OASIS) {
		map[currentX][currentY] = blankSpace;
		map[nextX][nextY] = troopType;
		return true;
	}
	return false;
}

//dodac semafor ktory bedzie zliczal zasoby i pozwoli na ich dodawanie - osobny watek, dodac chodzenie za przeciwnikami, by z nimi walczyc, spawnowanie jednostek przy kliknieciu czegos na klawiaturze

void Map::printMap()
{
	initscr();
	start_color();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);
	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	init_pair(5, COLOR_RED, COLOR_BLACK);
	int range = 0;
	int updateCounter = 5;
	while (true) {
		
		if (Simulation::endOfSimulation.load())
		{
			if(!isBombed.load()){
				updateCounter--;
				mvprintw(0, 0, "KONIEC SYMULACJI ZA: %d", updateCounter);
            refresh();
			if (updateCounter == 0) {
				break;
			}
			} else{
				bombTimer--;
				mvprintw(0, 0, "KONIEC SYMULACJI ZA: %d", bombTimer);
            	refresh();
				if (bombTimer == 0) {
					break;
				}
			}
			
			
		}
		if (Simulation::isDay.load())
		{
			mvprintw(1, 0, "Stan: Jest dzien.");
		}
		else {
			mvprintw(1, 0, "Stan: Jest noc.");
		}
		char inputChar;
		int input = getch();
		if (input == 'a')
		{
			teamACounterAdder.fetch_add(1);
			
		} else if(input == 'e'){
			teamBCounterAdder.fetch_add(1);
		} else if(input == 'b'){
			isBombed.store(true);
			Simulation::endOfSimulation.store(true);
		}
		
		
		mutex.lock();
		troopsVectorMutex.lock();
		resourceMutex.lock();
		if(isBombed.load()){
			if (range <= 10)	{
					int startRow = (mapSize / 2) - range;
        			int endRow = (mapSize / 2) + range;
        			int startCol = (mapSize / 2) - range;
        			int endCol = (mapSize / 2) + range;
					
					for (int col = startCol; col <= endCol; ++col) {
            			if (startRow >= 0 && startRow < mapSize && col >= 0 && col < mapSize) {
            			    map[startRow][col] = bombSpace;
            			}
            			if (endRow >= 0 && endRow < mapSize && col >= 0 && col < mapSize) {
            			    map[endRow][col] = bombSpace;
            			}
        			}

					for (int row = startRow + 1; row < endRow; ++row) { 
            			if (row >= 0 && row < mapSize && startCol >= 0 && startCol < mapSize) {
            			    map[row][startCol] = bombSpace;
            			}
            			if (row >= 0 && row < mapSize && endCol >= 0 && endCol < mapSize) {
            			    map[row][endCol] = bombSpace;
            			}
        			}
				range++;
				}
		}
		for (int i = 0; i < mapSize; i++)
		{
			for (int j = 0; j < mapSize; j++)
   		 	{
				if (Map::map[i][j].objectType == BLANK_SPACE)
        		{
					attron(COLOR_PAIR(2));
        		    mvprintw(i + 3, j * 2, "- ");
					attroff(COLOR_PAIR(2));
        		}
        		else if (Map::map[i][j].objectType == OASIS)
        		{
					attron(COLOR_PAIR(1));
        		    mvprintw(i + 3, j * 2, "O ");
					attroff(COLOR_PAIR(1)); 
        		}
        		else if (Map::map[i][j].objectType == RESOURCE)
        		{
					attron(COLOR_PAIR(5));
        		     mvprintw(i + 3, j * 2, "R ");
					attroff(COLOR_PAIR(5)); 
        		}
        		else if (Map::map[i][j].character == 'A')
        		{
					attron(COLOR_PAIR(3));
					mvprintw(i + 3, j * 2, "A ");
					attroff(COLOR_PAIR(4)); 

        		}
				else if (Map::map[i][j].character == 'E')
        		{
					attron(COLOR_PAIR(4));
					mvprintw(i + 3, j * 2, "E ");
					attroff(COLOR_PAIR(4)); 
        		} else if(Map::map[i][j].objectType == BOMB)
				{
					attron(COLOR_PAIR(4));
			 		mvprintw(i + 3, j * 2, "X ");
			 		attroff(COLOR_PAIR(4)); 
				}
		 
    		}
			 		
			}

		if(!isBombed.load()){

		mvprintw(mapSize+4, 0, "Logs:");
		for (int i = 0; i < troopsVector.size(); i++) {
    		if (troopsVector[i] != nullptr) {
    		    mvprintw(mapSize+5+i + 1, 0, "%d: %c hp: %d coords: X: %d Y: %d resources: X: %d Y: %d IsGoingToEnemy: %d IsGoingToResources: %d Closest Enemy X Y: %d %d",
    		         troopsVector[i]->getId(), troopsVector[i]->troopType.character, troopsVector[i]->hp,
    		         troopsVector[i]->coordinates.first, troopsVector[i]->coordinates.second,
    		         troopsVector[i]->closestResources.first, troopsVector[i]->closestResources.second,
    		         troopsVector[i]->isGoingToEnemy, troopsVector[i]->isGoingToResources,
    		         troopsVector[i]->closestEnemy.first, troopsVector[i]->closestEnemy.second);
    		}
		}

		// Wypisanie informacji o zasobach
		int troopsVectorSize = troopsVector.size();
		for (int i = 0; i < resourcesVector.size(); i++) {
			if(resourcesVector[i] != nullptr){
    			mvprintw(mapSize+ 5 + i + troopsVector.size() + 2, 0, "%d X: %d Y: %d dmg: %d hp: %d is Taken: %d",
             		resourcesVector[i]->resourceType.id, resourcesVector[i]->coordinates.first,
             		resourcesVector[i]->coordinates.second, resourcesVector[i]->damage,
             		resourcesVector[i]->hp, resourcesVector[i]->isTaken);
			 }
			}

		// Wypisanie liczby jednostek na drużynę
		mvprintw(mapSize+ 5 + troopsVector.size() + resourcesVector.size() + 4, 0, "Team A: %d", getTeamACounter());
		mvprintw(mapSize+ 5 + troopsVector.size() + resourcesVector.size() + 5, 0, "Team E: %d", getTeamBCounter());
		mvprintw(mapSize+ 5 + troopsVector.size() + resourcesVector.size() + 6, 0, "Symulacja sie toczy");

		mvprintw(mapSize + 8 + troopsVector.size() + resourcesVector.size(),0,"Spaces in oasis (counting semaphore): %d", Simulation::oasisCounter.load());
		}
		int tempResources = lastResourcesSize - resourcesVector.size();
		int tempTroops = lastTroopsSize - troopsVector.size();
		if (tempResources != lastResourcesSize || tempTroops != lastTroopsSize)
		{
			refresh();
			lastResourcesSize = tempResources;
			lastTroopsSize = tempTroops;
		}
		resourceMutex.unlock();
		troopsVectorMutex.unlock();
		mutex.unlock();
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		clear();
	}	
	endwin();

}


void Map::addTroopsToMap(std::shared_ptr<Troop> troop)
{
	std::lock_guard<std::mutex> lock(mutex);
	std::lock_guard<std::mutex> troopsLock(troopsVectorMutex);

	troopsVector.push_back(troop);
	map[troop->coordinates.first][troop->coordinates.second] = troop->troopType;
	if (troop->troopType.character == 'A')
	{
		teamACounter.fetch_add(1);
	}
	else if(troop->troopType.character == 'E') {
		teamBCounter.fetch_add(1);
	}
	lastTroopsSize = troopsVector.size();

}

std::shared_ptr<Troop> Map::checkForOtherTroops(int x, int y, ObjectsForMap objectType)
{
	std::lock_guard<std::mutex> lock(mutex);
	std::lock_guard<std::mutex> troopsLock(troopsVectorMutex);
	int checkX, checkY;
	for (int i = -1; i < 2; i++)
	{
		for (int j = -1; j < 2; j++) {
			checkX = x + i;
			checkY = y + j;
			if (checkX >= 0 && checkX < mapSize && checkY >= 0 && checkY < mapSize)
			{
				if (map[checkX][checkY].objectType == TROOP && map[checkX][checkY].character != objectType.character)
				{
					for (int t = 0; t < troopsVector.size(); t++)
					{
						if (troopsVector[t]->troopType.id == map[checkX][checkY].id) {
							if (!troopsVector[t]->isInOasis.load())
							{
								return troopsVector[t];
							}
							
						}
					}
				}
			}
		}
	}
	return nullptr;
}

void Map::deathOfTroop(int x, int y)
{
	std::lock_guard<std::mutex> lock(mutex);
	std::lock_guard<std::mutex> troopsLock(troopsVectorMutex);
	int id = map[x][y].id;

	 for (auto it = troopsVector.begin(); it != troopsVector.end(); ++it) {
        if ((*it)->id == id) {
            if ((*it)->troopType.character == 'A') {
                teamACounter--;
            } else {
                teamBCounter--;
            }
            troopsVector.erase(it); 
            break; 
        }
    }
    if (x >= oasisStartX && x < oasisStartX + oasisSize && y >= oasisStartY && y < oasisStartY + oasisSize) {
        map[x][y] = oasisSpace;
    } else {
        map[x][y] = blankSpace;
    }
}


void Map::addResourcesToMap(std::shared_ptr<Resource> resource)
{
	std::lock_guard<std::mutex> lock(mutex);
	std::lock_guard<std::mutex> resourceLock(resourceMutex);
	resourcesVector.push_back(resource);
	lastResourcesSize = resourcesVector.size();
	resourcesCounter.fetch_add(1);
	map[resource->coordinates.first][resource->coordinates.second] = resource->resourceType;
}

std::pair<int,int> Map::closestResource(int x, int y)
{
	std::lock_guard<std::mutex> resourceLock(resourceMutex);
	int closestValue = INT_MAX - 100000;
	std::pair<int, int> closestCoords;
	closestCoords = std::make_pair(INT_MAX- 100000, INT_MAX- 100000);
	for (int i = 0; i < resourcesVector.size(); i++)
	{
		int newValue = (std::abs(resourcesVector[i]->coordinates.first - x) + std::abs(resourcesVector[i]->coordinates.second - y));
		if (closestValue > newValue && !resourcesVector[i]->isTaken)
		{
			closestValue = newValue;
			closestCoords.first = resourcesVector[i]->coordinates.first;
			closestCoords.second = resourcesVector[i]->coordinates.second;
		}
	}

	return closestCoords;
}

bool Map::isBlankSpace(int x, int y)
{

	std::lock_guard<std::mutex> lock(mutex);
	if (map[x][y].objectType == BLANK_SPACE)
	{
		return true;
	}
	else {
		return false;
	}
}

int Map::getTeamACounter()
{
	return teamACounter.load();
}

int Map::getTeamBCounter()
{
	return teamBCounter.load();
}

bool Map::isValid(int x, int y)
{
	if (x < mapSize && x >= 0 && y < mapSize && y > 0) {
		return true;
	}
	return false;
}

std::pair<int, int> Map::getClosestEnemy(std::pair<int, int> troopCoords, ObjectsForMap troopType)
{
	std::lock_guard<std::mutex> troopsLock(troopsVectorMutex);
	int closestValue = INT_MAX- 100000;
	std::pair<int, int> closestCoords;
	closestCoords = std::make_pair(INT_MAX- 100000, INT_MAX- 100000);
	for (int i = 0; i < troopsVector.size(); i++)
	{
		if (troopCoords != troopsVector[i]->coordinates && troopsVector[i]->troopType.character != troopType.character && !troopsVector[i]->isInOasis.load()) {
			int newValue = (std::abs(troopsVector[i]->coordinates.first - troopCoords.first) + std::abs(troopsVector[i]->coordinates.second - troopCoords.second));
			if (closestValue > newValue)
			{
				closestValue = newValue;
				closestCoords.first = troopsVector[i]->coordinates.first;
				closestCoords.second = troopsVector[i]->coordinates.second;
			}
		}
	}
	return closestCoords;
}

std::pair<int,int> Map::getResource(std::shared_ptr<Resource> resource)
{
	std::lock_guard<std::mutex> resourceLock(resourceMutex);
	for (int i = 0; i < resourcesVector.size(); i++)
	{
		if (resourcesVector[i]->coordinates == resource->coordinates) {
			resourcesVector[i]->isTaken = true;
			resourcesVector[i]->damage -= 10;
			resourcesVector[i]->hp -= 10;
			
			return std::make_pair(resourcesVector[i]->damage,resourcesVector[i]->hp);
		}
	}
	return std::make_pair(INT_MAX- 100000, INT_MAX- 100000) ;

}

bool Map::resourceExist(std::shared_ptr<Resource> resource)
{
	std::lock_guard<std::mutex> resourceLock(resourceMutex);
	for (int i = 0; i < resourcesVector.size(); i++)
	{
		if (resourcesVector[i]->coordinates == resource->coordinates)
		{
			return true;
		}
	}
	return false;
}

void Map::setResourceTaken(std::shared_ptr<Resource> resource)
{
	std::lock_guard<std::mutex> resourceLock(resourceMutex);
	for (int i = 0; i < resourcesVector.size(); i++)
	{
		if (resourcesVector[i]->coordinates == resource->coordinates) {
			resourcesVector[i]->isTaken = false;
		}
	}
}

bool Map::isNextStepOasis(std::pair<int, int> troopCoords)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (map[troopCoords.first][troopCoords.second].objectType == OASIS) {
		return true;
	}
	return false;
}

bool Map::updateOasisCoords(std::pair<int, int> nextMove, std::pair<int, int> currentPos, ObjectsForMap troopType)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (map[nextMove.first][nextMove.second].objectType == OASIS || map[nextMove.first][nextMove.second].objectType == BLANK_SPACE) {
		map[currentPos.first][currentPos.second] = oasisSpace;
		map[nextMove.first][nextMove.second] = troopType;
		return true;
	}
	return false;
}

std::pair<int, int> Map::getOasisCenter()
{
	std::lock_guard<std::mutex> oasisLock(oasisMutex);
	return oasisCenter;
}

std::shared_ptr<Resource> Map::checkForResource(int x, int y)
{
	std::lock_guard<std::mutex> lock(mutex);
	std::lock_guard<std::mutex> resourceLock(resourceMutex);
	int checkX, checkY;
	for (int i = -1; i < 2; i++)
	{
		for (int j = -1; j < 2; j++) {
			checkX = x + i;
			checkY = y + j;
			if (checkX >= 0 && checkX < mapSize && checkY >= 0 && checkY < mapSize)
			{
				if (map[checkX][checkY].objectType == RESOURCE)
				{
					for (int t = 0; t < resourcesVector.size(); t++)
					{
						if (resourcesVector[t]->resourceType.id == map[checkX][checkY].id && resourcesVector[t]->isTaken == false) {
							resourcesVector[t]->isTaken = true;
							
							return resourcesVector[t];
						}
					}
				}
			}
		}
	}
	
	return nullptr;
}

void Map::deleteResource(std::shared_ptr<Resource> resource)
{
    std::lock_guard<std::mutex> lock(mutex);
    int id = resource->resourceType.id;  

    for (auto it = resourcesVector.begin(); it != resourcesVector.end(); ++it) {
        if ((*it)->resourceType.id == id) {
            map[resource->coordinates.first][resource->coordinates.second] = blankSpace;
            resourcesVector.erase(it);
			resourcesCounter.fetch_sub(1);
            break; 
        }
    }
}


bool Map::isThereAnyResource()
{
	std::lock_guard<std::mutex> resourceLock(resourceMutex);
	if (resourcesVector.size() != 0) {
		return true;
	}
	return false;
}
