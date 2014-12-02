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
	speed = 4;
	mutationPercent = 15;

	reset();
}

// The sphere is 2 in diameter (-1 to 1), and the move distance represents the distance a move operation
// will move a critter. This also determines segment size, vision, etc.
float Parameters::getCellSize()
{
    return (cellSize + 1) * .002f;
}

float Parameters::getPhotosynthesizeBonus()
{
    return photoSynthesizeEnergyGain;// - 1.0;
}

void Parameters :: reset()
{
    moveCellSizeFraction = .5f;
    moveEnergyCost = 1.0f;
	moveAndEatEnergyCost = .5f;
    photoSynthesizeEnergyGain = 2.0f;
	digestionEfficiency = .75f;
    biteStrength = 4.0f;
	deadCellDormancy = 10000; // turns before a "dead" cell (such as left by a critter that starved) turns into a live photo cell
    baseSpawnEnergy = 0;
    extraSpawnEnergyPerSegment = 1000;
    sleepTimeAfterBeingSpawned = 0;
    baseLifespan = 500;
    extraLifespanPerSegment = 10000;
    cellSize = 4;
    extraCyclesForMove = 15;
    allowSelfOverlap = false;
    useNaturalMovement = true;
    lookDistance = 30;
	randomFood = 0;

    sleepTime = 20;
	unexecutedTurnCost = .2f;

	turnToFoodAfterDeath = true;

	mouthSize = 1.0f;
	lookSpread = 1.03f;
	cannibals = 1;
	allowOr = false;
}