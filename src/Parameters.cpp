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
    return .004 + (float) cellSize * .004;
}

void Parameters :: reset()
{
    mutationPercent = 5;
    cycleEnergyCost = 1;
    moveEnergyCost = .5;
    moveAndEatEnergyCost = 10;//6;
    photoSynthesizeEnergyGain = 10;//8;
    digestionEfficiency = 1.0f;
    deadCellDormancy = 5000;
    baseSpawnEnergy = 500;
    extraSpawnEnergyPerSegment = 500;
    sleepTimeAfterBeingSpawned = 0;
    
    baseLifespan = 3000;
    extraLifespanPerSegment = 3000;
    cellSize = 5;
    cyclesForMove = 2;
    allowSelfOverlap = true;
    lookDistance = 7;
    
    sleepTime = 10;
}

// global
int Parameters :: speed = 8;
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

