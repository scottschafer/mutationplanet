/************************************************************************
 MutationPlanet
 Copyright (C) 2012, Scott Schafer
 
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

    // global
int Parameters :: speed = 10;
int Parameters :: mutationPercent = 5;

    // energy cost / gain
float Parameters :: cycleEnergyCost = 1;
float Parameters :: moveEnergyCost = .5;
float Parameters :: moveAndEatEnergyCost = 7;
float Parameters :: photoSynthesizeEnergyGain = 4;
    
    // spawn energy
float Parameters :: baseSpawnEnergy = 200;
float Parameters :: extraSpawnEnergyPerSegment = 100;
    
    // lifespan
float Parameters :: baseLifespan = 2000;
float Parameters :: extraLifespanPerSegmnet = 100;
    
    // moving
float Parameters :: cellSize = 5;
int Parameters :: cyclesForMove = 2;
bool Parameters:: allowSelfOverlap = true;

    // looking
int Parameters :: lookDistance = 10;
