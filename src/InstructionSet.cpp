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
 InstructionSet
 ---------
 A utility class for managing all the available instructions.
 
 Individual instructions could be turned on or off.
 */



#include "InstructionSet.h"
#include "UtilsRandom.h"
#include <vector>

static std::vector<char> availableInstructions;
static std::set<char> allAvailableInstructions;

void InstructionSet :: reset()
{
    setInstructionAvailable(eInstructionMove, true);
    setInstructionAvailable(eInstructionMoveAndEat, true);
    
    setInstructionAvailable(eInstructionTurnLeft, true);
    setInstructionAvailable(eInstructionTurnRight, true);
    setInstructionAvailable(eInstructionHardTurnLeft, true);
    setInstructionAvailable(eInstructionHardTurnRight, true);
    setInstructionAvailable(eInstructionSleep, true);
    setInstructionAvailable(eInstructionTestSeeFood, true);
    setInstructionAvailable(eInstructionTestBlocked, true);
    setInstructionAvailable(eInstructionTestNotSeeFood, true);
    setInstructionAvailable(eInstructionTestNotBlocked, true);
    setInstructionAvailable(eInstructionPhotosynthesize, true);
    setInstructionAvailable(eInstructionFakePhotosynthesize, true);
}

char InstructionSet :: getRandomInstruction()
{
    char result = availableInstructions[UtilsRandom::getRangeRandom(0, availableInstructions.size()-1)];
    if (! result)
        throw "argh";
    return result;
}

std::set<char> InstructionSet :: getAllAvailableInstructions()
{
    return allAvailableInstructions;
}

void InstructionSet :: setInstructionAvailable(char instruction, bool isAvailable)
{
    std::vector<char>::iterator i= std::find(availableInstructions.begin(),
                       availableInstructions.end(),
                       instruction);
    bool isInSet = i != availableInstructions.end();
    
    if (isInSet != isAvailable)
    {
        if (isAvailable)
            availableInstructions.push_back(instruction);
        else
            availableInstructions.erase(i);
    }
    
    allAvailableInstructions.insert(instruction);
        
}
