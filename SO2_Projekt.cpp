#include <iostream>
#include "Simulation.h"
#include <ncurses.h>

int main()
{

    Simulation simulation;
    std::thread simualationAThread(&Simulation::simulation, std::ref(simulation));

    simualationAThread.join();
    
    return 0;
}

