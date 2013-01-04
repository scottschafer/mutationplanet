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


/**
 UtilsGenome
 
 Simple genome utility class.
 **/

#include "UtilsGenome.h"
#include "UtilsRandom.h"
#include "InstructionSet.h"
#include "Constants.h"

void UtilsGenome::generateRandomly(int length, char *pOut)
{
    while (length--)
        *pOut++ = InstructionSet::getRandomInstruction();
    *pOut = 0;;
}

void UtilsGenome::mutate(const char *pIn, char *pOut)
{
    char *pResult = pOut;
    
    int mutationType = (int) UtilsRandom::getRangeRandom(0, 2);
    int length = strlen(pIn);

    switch (mutationType)
    {
        default:
        case 0: {
            if (length < (MAX_GENOME_LENGTH-1))
            {
                // insert a new random instruction
                char insertInstruction = InstructionSet::getRandomInstruction();
                int insertPosition = (int) UtilsRandom::getRangeRandom(0,length);
                while (length-- > 0)
                {
                    if (insertPosition-- == 0)
                        *pOut++ = insertInstruction;
                    *pOut++ = *pIn++;
                }
                if (! insertInstruction)
                    *pOut++ = insertInstruction;
                *pOut = 0;
            }
            else
            {
                strcpy(pOut, pIn);
            }
            break; }
            
        case 1: {
            // modify an instruction randomly
            char mutateInstruction = InstructionSet::getRandomInstruction();
            int charPosition = (int) UtilsRandom::getRangeRandom(0,length-1);
            strcpy(pOut, pIn);
            pOut[charPosition] = mutateInstruction;
            break; }
            
        case 2: {
            // delete an instruction randomly
            if (length == 1)
            {
                strcpy(pOut, pIn);
            }
            else
            {
                int charPosition = (int) UtilsRandom::getRangeRandom(0,length-1);
                while (*pIn)
                {
                    if (!charPosition)
                        ++pIn;
                    --charPosition;
                    *pOut++ = *pIn++;
                }
                *pOut = 0;
            }
            break; }
    }
    
    if (strlen(pResult) == 0)
        throw "error";
}
