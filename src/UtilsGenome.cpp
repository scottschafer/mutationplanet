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


/**
 UtilsGenome
 
 Simple genome utility class.
 **/

#include "UtilsGenome.h"
#include "UtilsRandom.h"
#include "InstructionSet.h"
#include "Constants.h"

eSegmentExecutionType getRandomExecutionType()
{
	int r = rand() % 5;
	switch (r) {
	case 0:
		return eIf;
	case 1:
		return eNotIf;
	default:
		return eAlways;
	}
};

int strlen(const Instruction *pIn)
{
	int result = 0;
	while (pIn[result].instruction)
		++result;
	return result;
}

void strcpy(Instruction *pDest, const Instruction *pSrc)
{
	memcpy (pDest, pSrc, MAX_GENOME_LENGTH * sizeof(Instruction));
}

Instruction randomInstruction()
{
	Instruction result;
	result.instruction = InstructionSet::getRandomInstruction();
	result.executeType = getRandomExecutionType();
	return result;
}

void UtilsGenome::generateRandomly(int length, Instruction *pOut)
{
    while (length--) {
		(*pOut).instruction = InstructionSet::getRandomInstruction();
		(*pOut).executeType = getRandomExecutionType();
		++pOut;
	}
	(*pOut).instruction = 0;
}

void UtilsGenome::mutate(const Instruction *pIn, Instruction *pOut)
{
    Instruction *pResult = pOut;
    
    int mutationType = (int) UtilsRandom::getRangeRandom(0, 2);
    int length = strlen(pIn);

    switch (mutationType)
    {
        default:
        case 0: {
            if (length < (MAX_GENOME_LENGTH-1))
            {
                // insert a new random instruction
                Instruction insertInstruction = randomInstruction();
                int insertPosition = (int) UtilsRandom::getRangeRandom(0,length);
                while (length-- > 0)
                {
                    if (insertPosition-- == 0)
                        *pOut++ = insertInstruction;
                    *pOut++ = *pIn++;
                }
                if (! insertPosition)
                    *pOut++ = insertInstruction;
				(*pOut).instruction = 0;
            }
            else
            {
                strcpy(pOut, pIn);
            }
            break; }
            
        case 1: {
            // modify an instruction randomly
            Instruction mutateInstruction = randomInstruction();
            int charPosition = (int) UtilsRandom::getRangeRandom(0,length-1);
            strcpy(pOut, pIn);
			if (UtilsRandom::getRangeRandom(0, 1))
				pOut[charPosition].instruction = mutateInstruction.instruction;
			else
				pOut[charPosition].executeType = mutateInstruction.executeType;
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
				while ((*pIn).instruction)
                {
                    if (!charPosition)
                        ++pIn;
                    --charPosition;
                    *pOut++ = *pIn++;
                }
				(*pOut).instruction = 0;
            }
            break; }
    }
    
    if (strlen(pResult) == 0)
        throw "error";
}
