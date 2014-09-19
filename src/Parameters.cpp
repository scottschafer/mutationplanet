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

Parameters Parameters :: instance;

Parameters :: Parameters() {
	speed = 10;
	mutationPercent = 15;
	slowSideSpeed = 4;

	reset();
}

float Parameters::getMoveDistance()
{
    return (cellSize + 1) * .002f;
}

float Parameters::getPhotosynthesizeBonus()
{
	return photoSynthesizeEnergyGain - 1.0;
}

void Parameters :: reset()
{
	slowSideSpeed = 4;
    mutationPercent = 25;
    moveEnergyCost = 4;
    moveAndEatEnergyCost = 60;//6;
    photoSynthesizeEnergyGain = 2.0f;//8;
	//photoSynthesizeBonus = 1.0f;
    digestionEfficiency = 1.0f;
	biteStrength = 1.0f;
    deadCellDormancy = 10000; // turns before a "dead" cell (such as left by a critter that starved) turns into a live photo cell
    baseSpawnEnergy = 0;
    extraSpawnEnergyPerSegment = 1000;
    sleepTimeAfterBeingSpawned = 0;//20;
    
    baseLifespan = 0;
    extraLifespanPerSegment = 10000;
    cellSize = 4;
    extraCyclesForMove = 5;
    allowSelfOverlap = false;
    lookDistance = 30;

    sleepTime = 20;
	unexecutedTurnCost = .1f;

	turnToFoodAfterDeath = true;

	mouthSize = 1.0f;
	lookSpread = 1.02f;

	cannibals = 1;
	allowOr = true;
}

/*
// global
int Parameters :: speed = 10;
int Parameters :: mutationPercent;
int Parameters :: slowSideSpeed = 4;

// energy cost / gain
float Parameters :: moveEnergyCost;
float Parameters :: moveAndEatEnergyCost;
float Parameters :: photoSynthesizeEnergyGain;
float Parameters :: unexecutedTurnCost;
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

int Parameters :: turnToFoodAfterDeath;

//float Parameters :: photoSynthesizeBonus;

float Parameters :: mouthSize;
float Parameters :: lookSpread;

int Parameters :: cannibals;

int Parameters :: allowOr;

*/