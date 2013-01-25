/************************************************************************
 MutationPlanet
 Copyright (C) 2012, Scott Schafer, scott.schafer@gmail.com
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/

/*
 Parameters
 All the user tweakable environment variables
 */

#include "Parameters.h"

float Parameters::getMoveDistance()
{
    return .005 + (float) cellSize * .002;
}

void Parameters :: reset()
{
    mutationPercent = 3;
    cycleEnergyCost = 1;
    moveEnergyCost = 1;
    moveAndEatEnergyCost = 7;
    photoSynthesizeEnergyGain = 3.5;
    digestionEfficiency = .8f;
    deadCellDormancy = 1000;
    baseSpawnEnergy = 0;
    extraSpawnEnergyPerSegment = 250;
    sleepTimeAfterBeingSpawned = 200;
    
    baseLifespan = 5000;
    extraLifespanPerSegment = 2500;
    cellSize = 5;
    cyclesForMove = 1;
    allowSelfOverlap = true;
    lookDistance = 10;
    
    sleepTime = 20;
}

// global
int Parameters :: speed = 10;
int Parameters :: mutationPercent;

// energy cost / gain
float Parameters :: cycleEnergyCost;
float Parameters :: moveEnergyCost;
float Parameters :: moveAndEatEnergyCost;
float Parameters :: photoSynthesizeEnergyGain;
float Parameters :: digestionEfficiency;
int Parameters :: deadCellDormancy;

// spawn energy
float Parameters :: baseSpawnEnergy;
float Parameters :: extraSpawnEnergyPerSegment;
int Parameters :: sleepTimeAfterBeingSpawned;

// lifespan
float Parameters :: baseLifespan;
float Parameters :: extraLifespanPerSegment;

// moving
float Parameters :: cellSize;
int Parameters :: cyclesForMove;
bool Parameters:: allowSelfOverlap;

// looking
int Parameters :: lookDistance;

// sleep
int Parameters :: sleepTime;

