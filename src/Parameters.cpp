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
	slowSideSpeed = 10;

	reset();
}

float Parameters::getMoveDistance()
{
    return (cellSize + 1) * .002f;
}

float Parameters::getPhotosynthesizeBonus()
{
    return photoSynthesizeEnergyGain;// - 1.0;
}

void Parameters :: reset()
{
	slowSideSpeed = 10;
    mutationPercent = 15;
    moveEnergyCost = 5.0f;//3.5f;
    moveAndEatEnergyCost = 50;
    photoSynthesizeEnergyGain = 2.0f; //3.5f;
    digestionEfficiency = .9f;
    biteStrength = 2.0f;//1.3f;
    deadCellDormancy = 10000; // turns before a "dead" cell (such as left by a critter that starved) turns into a live photo cell
    baseSpawnEnergy = 0;
    extraSpawnEnergyPerSegment = 1500;
    sleepTimeAfterBeingSpawned = 0;
    baseLifespan = 0;
    extraLifespanPerSegment = 10000;
    cellSize = 4;
    extraCyclesForMove = 3;
    allowSelfOverlap = false;
    lookDistance = 40;

    sleepTime = 10;
	unexecutedTurnCost = .1f;

	turnToFoodAfterDeath = true;

	mouthSize = 1.0f;
	lookSpread = 1.02f;

	cannibals = 1;
	allowOr = false;
}