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
    return (.004f + (float) cellSize * .004f);// * .75f;
}

void Parameters :: reset()
{
    mutationPercent = 8;
    moveEnergyCost = 10;
    moveAndEatEnergyCost = 20;//6;
    photoSynthesizeEnergyGain = 1.2f;//8;
    digestionEfficiency = 1.0f;
	biteStrength = .5f;
    deadCellDormancy = 2000;
    baseSpawnEnergy = 1000;
    extraSpawnEnergyPerSegment = 1000;
    sleepTimeAfterBeingSpawned = 5;
    
    baseLifespan = 5000;
    extraLifespanPerSegment = 5000;
    cellSize = 1;
    extraCyclesForMove = 10;
    allowSelfOverlap = false;
    lookDistance = 30;
    
    sleepTime = 10;
}

// global
int Parameters :: speed = 10;
int Parameters :: mutationPercent;

// energy cost / gain
float Parameters :: moveEnergyCost;
float Parameters :: moveAndEatEnergyCost;
float Parameters :: photoSynthesizeEnergyGain;
float Parameters :: digestionEfficiency;
float Parameters :: biteStrength;
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
int Parameters :: extraCyclesForMove;
bool Parameters:: allowSelfOverlap;

// looking
int Parameters :: lookDistance;

// sleep
int Parameters :: sleepTime;

